#ifndef TCP_H
#define TCP_H

#include <stddef.h>

extern int tcp_listen(const char *port, int fds[], size_t max);

extern int tcp_write_all(int fd, const char *buf, size_t len);

#endif
