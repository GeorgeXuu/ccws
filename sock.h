#ifndef SOCK_H
#define SOCK_H

#include "utility.h"
#include "ipv4_addr.h"

class sock
{
public:
  sock(int sockfd) : fd(sockfd) { }

  void listen() const noexcept { listen_or_die(fd); }
  void bind(const struct sockaddr* addr) const noexcept { bind_or_die(fd, addr); }

  [[nodiscard]]
  int accept(ipv4_addr* peeraddr) const noexcept
  {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    int connfd = accept_smart(fd, &addr);
    if (connfd >= 0) *peeraddr = ipv4_addr(std::move(addr));
    return connfd;
  }

  void reuse_addr() const noexcept
  {
    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
  }

  void reuse_port() const noexcept
  {
    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
  }

  constexpr
  operator int() const noexcept { return fd; }

  sock(const sock&) = delete;
  sock& operator=(const sock&) = delete;

  ~sock() { ::close(fd); }
private:
  const int fd;
};


#endif // SOCK_H
