#ifndef IPV4_ADDR_H 
#define IPV4_ADDR_H

#include <arpa/inet.h> // struct sockaddr_in
#include <cstring>     // memset
#include <string>      // std::string


template <class I> concept charptr = std::is_same_v<std::decay_t<I>, char*> || std::is_same_v<std::decay_t<I>, const char*>;
template <class I> concept string_like = requires(I str) { str.c_str(); };
template <class I> concept charptr_or_string_like = string_like<I> || charptr<I>;

#include <stdio.h>
struct ipv4_addr
{
  constexpr
  ipv4_addr() noexcept
  {
    memset(&addr, 0, sizeof(addr));
  }

  explicit constexpr
  ipv4_addr(uint16_t port, bool loopback_only = false) noexcept
  {
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(loopback_only ? INADDR_LOOPBACK : INADDR_ANY); // see getaddrinfo(3) on def of consts
  }

  // assume ip is in numbers and dot notation(E.g. 127.0.0.1), so inet_aton(2) cannot fail
  template <charptr_or_string_like I> constexpr 
  ipv4_addr(I ip, uint16_t port) noexcept 
  {
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if constexpr(charptr<I>) inet_aton(ip, &addr.sin_addr); 
    else inet_aton(ip.c_str(), &addr.sin_addr);
  }

  explicit constexpr
  ipv4_addr(const sockaddr_in& x) noexcept : addr(x) { }

  [[nodiscard]] constexpr inline
  auto domain() const noexcept { return addr.sin_family; }

  [[nodiscard]] constexpr inline
  uint32_t ip_net_endian() const noexcept { return addr.sin_addr.s_addr; }

  [[nodiscard]] inline
  std::string ip() const noexcept { return inet_ntoa(addr.sin_addr); }

  [[nodiscard]] inline
  uint16_t port_net_endian() const noexcept { return addr.sin_port; }

  [[nodiscard]] inline
  uint16_t port() const noexcept { return ntohs(port_net_endian()); }

  [[nodiscard]] inline
  std::string ip_port() const noexcept { return ip() + ':' + std::to_string(port()); }

  [[nodiscard]] inline
  const auto to_sockaddr_ptr() const noexcept { return reinterpret_cast<const sockaddr*>(&addr); }

  [[nodiscard]] inline
  auto to_sockaddr_ptr() noexcept { return reinterpret_cast<sockaddr*>(&addr); }

  struct sockaddr_in addr;
};

#endif // IPV4_ADDR_H
