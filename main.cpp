#include <stdio.h>
#include <cstring>
#include <sys/epoll.h>
#include <arpa/inet.h>

#include <vector>
#include <unistd.h>
#include <string>

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <netdb.h> // NI_MAXHOST, NI_MAXSERV

#include <vector>
#include <iostream>
#include <algorithm>
#include "util.h"
#include <iostream>
#include "ipv4_addr.h"
#include "poller.h"
#include "util.h"
#include "conn_channel.h"

const std::string_view path_to_resource = "/home/oracle/memo";

void bad_request(int connfd,
    const std::string& cause, const std::string& errnum, const std::string& shortmsg, const std::string& longmsg)
{
  std::string body, buf;

  /* Build the HTTP response body */
  body += "<html><title>useless Error</title>";
  body += "<body bgcolor=""ffffff"">\r\n";
  body += errnum + ": " + shortmsg + "\r\n";
  body += "<p>" + longmsg + ": " + cause + "\r\n";
  body += "<hr><em>The useless Web server</em>\r\n";

  /* Print the HTTP response */
  buf += "HTTP/1.1 " + errnum + " " + shortmsg + "\r\n";
  buf += "Content-type: text/html\r\n";
  buf += "Content-length: " + std::to_string(body.size()) + "\r\n\r\n";
  write(connfd, buf.c_str(), buf.size());
  write(connfd, body.c_str(), body.size());
}

template <std::random_access_iterator I>
bool parse_request(I first, I last, std::string& res, const std::string& default_expand = "index.html")
{
  // very simple server, so just take the first line and ignore the rest
  // assume each "line" ends in CRLF
  // assume only one space between field: so "GET /    HTTP/1.1"  cannot happen
  // assume methods are capitalilzed, so "GET" is valid but "get" is not
  std::string request(first, std::find(first, last, '\r'));
  // so only accept GET method
  if(request.substr(0, 3) != "GET" || request.size() < 5 || request[3] != ' ' || request[4] != '/') return false;

  auto next_ws = std::find(request.begin() + 4, request.end(), ' ');
  if(next_ws == request.end()) return false;
  // check protocol first
  auto prot = request.substr(next_ws + 1 - request.begin(), 8);
  if(prot != "HTTP/1.1" && prot != "HTTP/1.0") return false;

  // now handle resource
  std::string relative_filename(request.begin() + 4, next_ws);
  if(relative_filename == "/") relative_filename += default_expand;
  res = relative_filename;
  return true;
}

std::string get_filetype(const char *filename)
{
  if(strstr(filename, ".html"))     return "text/html";
  else if(strstr(filename, ".gif")) return "image/gif";
  else if(strstr(filename, ".png")) return "image/png";
  else if(strstr(filename, ".jpg")) return "image/jpeg";
  else return "text/plain";
}

void good_request(int connfd, const char* filename, int filesize)
{
  std::string buf;
  int srcfd;

  /* Send response headers to client */
  auto filetype = get_filetype(filename);
  buf += "HTTP/1.0 200 OK\r\n";
  buf += "Server: useless Web Server\r\n";
  buf += "Connection: close\r\n";
  buf += "Content-length: " + std::to_string(filesize) + "\r\n";
  buf += "Content-type: " + filetype + "\r\n\r\n";

  write(connfd, buf.c_str(), buf.size());
  printf("Response headers:\n");
  printf("%s", buf.c_str());

  /* Send response body to client */
  srcfd = open(filename, O_RDONLY | O_CLOEXEC, 0);
  char* srcp = static_cast<char*>(mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0));
  close(srcfd);
  write(connfd, srcp, filesize);
  munmap(srcp, filesize);
}


template <std::random_access_iterator I>
void handle_request(int connfd, I first, I last)
{
  std::string basename;
  if(!parse_request(first, last, basename))
  {
    write(connfd, "HTTP/1.1 400 Bad Request\r\n\r\n", strlen("HTTP/1.1 400 Bad Request\r\n\r\n"));
    return;
  }
  std::string filename{path_to_resource};
  filename += basename;

  // check if file exists
  struct stat sbuf;
  if(stat(filename.c_str(), &sbuf) < 0)
  {
    bad_request(connfd, basename, "404", "Not found", "Your love to me is also not found qwq");
    return;
  }

  // I don't want you to touch files in other dir
  char resolved_path[PATH_MAX]{};
  if(!realpath(filename.c_str(), resolved_path) || strncmp(path_to_resource.data(), resolved_path, path_to_resource.size()))
  {
    bad_request(connfd, basename, "403", "Forbidden", "Don't you look around");
    return;
  }
  filename = resolved_path;

  if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
  {
    bad_request(connfd, basename, "403", "Forbidden", "Don't you look around");
    return;
  }
  good_request(connfd, filename.c_str(), sbuf.st_size);
}

int main()
{
  ipv4_addr ip{8001};
  int listen_socket = create_nonblocking_socket_or_die(ip.domain());
  bind_or_die(listen_socket, ip.to_sockaddr_ptr());
  listen_or_die(listen_socket); 

  poller p;
  p.add_read(listen_socket, &listen_socket);

  while(true)
  {
    using namespace std::literals;
    p.poll(1s);
    for(int ii = 0; ii < p.nfds; ++ii)
    {
      if(*(int*)p.events[ii].data.ptr == listen_socket)
      {
        ipv4_addr connip;
        conn_channel* connptr = new conn_channel(accept_or_die(listen_socket, connip.to_sockaddr_ptr()));
        p.add_read(connptr->fd, connptr);
      }
      else
      {
        printf("into connection socket\n");

        auto conn = static_cast<conn_channel*>(p.events[ii].data.ptr);
        if(p.events[ii].events & EPOLLIN)
        {
          char buf[65536];
          auto cnt = read(conn->fd, buf, sizeof(buf));
          conn->buf += std::string(buf, buf + cnt);
          std::string last_four = std::string(conn->buf.end() - 4, conn->buf.end());
          if(last_four == std::string("\r\n\r\n"))
          {
            handle_request(conn->fd, conn->buf.begin(), conn->buf.end());
            p.del_event(conn->fd);
            delete conn; 
          }
        }

        // need handle close event
      }
    }
  }
}

