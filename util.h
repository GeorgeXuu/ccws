#ifndef UTIL_H
#define UTIL_H

int create_nonblocking_socket_or_die(int domain = AF_INET)
{
  int fd = socket(domain, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if(fd < 0) exit(EXIT_FAILURE);
  return fd;
}

void bind_or_die(int sockfd, const struct sockaddr* addr)
{
  if(bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr))) < 0) exit(EXIT_FAILURE);
}

void listen_or_die(int sockfd)
{
  int ret = ::listen(sockfd, SOMAXCONN);
  if(ret < 0) exit(EXIT_FAILURE);
}

int accept_or_die(int sockfd, struct sockaddr* addr)
{
  socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
  int connfd = ::accept4(sockfd, addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if(connfd < 0) exit(EXIT_FAILURE);
  return connfd;
}

#endif // UTIL_h
