#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "tcp.h"

int
tcp_listen(const char *port, int fds[], size_t max)
{
    struct addrinfo hints, *res, *ai;
    size_t n = 0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        return -1;
    }

    for (ai = res; ai && n < max; ai = ai->ai_next) {
        int fd, on = 1;

        fd = socket(ai->ai_family, ai->ai_socktype | SOCK_CLOEXEC,
                    ai->ai_protocol);
        if (fd < 0) {
            continue;
        }
        (void) setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        if (ai->ai_family == AF_INET6) {
            /* separate sockets for IPv4 and IPv6 */
            (void) setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));
        }
        if (bind(fd, ai->ai_addr, ai->ai_addrlen) < 0
            || listen(fd, SOMAXCONN) < 0) {
            (void) close(fd);
            continue;
        }
        fds[n++] = fd;
    }
    freeaddrinfo(res);

    return n > 0 ? (int) n : -1;
}

int
tcp_write_all(int fd, const char *buf, size_t len)
{
    while (len > 0) {
        ssize_t w = write(fd, buf, len);
        if (w < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        buf += w;
        len -= (size_t) w;
    }
    return 0;
}
