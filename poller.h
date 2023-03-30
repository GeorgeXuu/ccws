#ifndef POLLER_H
#define POLLER_H

#include <chrono>
#include <vector>
#include <unistd.h> // close
#include <sys/epoll.h>
class poller
{
public:
  poller() noexcept
  : epollfd(epoll_create1(EPOLL_CLOEXEC)), nfds(0), now(), events(16)
  {
    if(epollfd == -1) exit(EXIT_FAILURE);
  }

private:
  void event_change_(int fd, int op, int event, void* baggage) const noexcept
  {
    struct epoll_event e;
    memset(&e, 0, sizeof(e));
    e.data.ptr = baggage;
    e.events = event;
    if(epoll_ctl(epollfd, op, fd, &e) == -1) exit(EXIT_FAILURE);
  }

public:
  // beware! read_write_add() is NOT the same as read_add; write_add;
  void add_read(int fd, void* baggage) { event_change_(fd, EPOLL_CTL_ADD, EPOLLIN | EPOLLPRI, baggage); }
  void add_write(int fd, void* baggage) { event_change_(fd, EPOLL_CTL_ADD, EPOLLOUT, baggage); }
  void add_read_write(int fd, void* baggage) { event_change_(fd, EPOLL_CTL_ADD, EPOLLIN | EPOLLPRI | EPOLLOUT, baggage); }

  void mod_read(int fd, void* baggage) { event_change_(fd, EPOLL_CTL_MOD, EPOLLIN | EPOLLPRI, baggage); }
  void mod_write(int fd, void* baggage) { event_change_(fd, EPOLL_CTL_MOD, EPOLLOUT, baggage); }
  void mod_read_write(int fd, void* baggage) { event_change_(fd, EPOLL_CTL_MOD, EPOLLIN | EPOLLPRI | EPOLLOUT, baggage); }

  // added chap 3
  void add_event(int fd, int events, void* baggage) { event_change_(fd, EPOLL_CTL_ADD, events, baggage); }
  void mod_event(int fd, int events, void* baggage) { event_change_(fd, EPOLL_CTL_MOD, events, baggage); }

  void del_event(int fd) { if(epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) == -1) exit(EXIT_FAILURE); }

  // return { now: the time, last: the point where input ends}
  // user needs to guarantee there are enough space between first and a sentinel
  void poll(std::chrono::milliseconds timeout) noexcept
  {
    nfds = epoll_wait(epollfd, &events.front(), static_cast<int>(events.size()), timeout.count());
    if(nfds == -1) exit(EXIT_FAILURE);
    now = std::chrono::system_clock::now();
    if(nfds == static_cast<int>(events.size())) events.resize(events.size() * 2);
  }

  ~poller() { ::close(epollfd); }

private:
  int epollfd;
public: // read only by user
  int nfds;
  std::chrono::time_point<std::chrono::system_clock> now;
  std::vector<struct epoll_event> events;
};


#endif // POLLER_H
