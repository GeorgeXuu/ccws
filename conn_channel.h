
struct conn_channel
{
  int fd;
  using buffer = std::string;
  buffer buf;

  conn_channel(int fd) : fd(fd), buf() { }
};

