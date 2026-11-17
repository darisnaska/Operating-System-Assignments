#define _GNU_SOURCE /* for sendfile() prototype in <sys/sendfile.h> */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <stdint.h>

static void die(const char *what)
{
    fprintf(stderr, "scat: %s: %s (errno %d)\n", what, strerror(errno), errno);
    exit(EXIT_FAILURE);
}

static void copy_library(void)
{
    int c;

    while ((c = getc(stdin)) != EOF) {
        if (putc(c, stdout) == EOF)
            die("putc");
    }
    if (ferror(stdin))
        die("getc");
    if (fflush(stdout) == EOF)
        die("fflush");
}

static void copy_syscall(void)
{
    unsigned char byte;
    ssize_t n;

    for (;;) {
        n = read(STDIN_FILENO, &byte, 1);
        if (n == -1) {
            if (errno == EINTR)
                continue; /* interrupted by a signal, retry */
            die("read");
        }
        if (n == 0)
            break; /* EOF */

        ssize_t w = write(STDOUT_FILENO, &byte, 1);
        if (w == -1) {
            if (errno == EINTR)
                continue;
            die("write");
        }
        /* w must be 1 here since we only ever ask for 1 byte */
    }
}

static size_t stdio_chunk_size(void)
{
    struct stat st;
    if (fstat(STDIN_FILENO, &st) == 0 && st.st_blksize > 0)
        return (size_t)st.st_blksize;
    return BUFSIZ; /* fallback, mirrors glibc's own fallback */
}

static void copy_sendfile(void)
{
    size_t chunk = stdio_chunk_size();
    ssize_t n;
    do {
        n = sendfile(STDOUT_FILENO, STDIN_FILENO, NULL, chunk);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            /* sendfile() cannot be used on this combination of fds
             * (e.g. in_fd is a regular pipe on very old kernels, or
             * out_fd is not writable in the required way). Report and
             * bail out rather than silently doing nothing. */
            die("sendfile");
        }
    } while (n > 0);
}

static void usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s [-l | -s | -p]\n"
        "  -l  use C library getc()/putc() copy loop (default)\n"
        "  -s  use read()/write() system call copy loop\n"
        "  -p  use sendfile() copy loop\n",
        prog);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int opt;
    char mode = 'l'; /* default: C library loop */

    while ((opt = getopt(argc, argv, "lsp")) != -1) {
        switch (opt) {
        case 'l':
        case 's':
        case 'p':
            mode = (char)opt; /* last option on the command line wins */
            break;
        default:
            usage(argv[0]);
        }
    }

    switch (mode) {
    case 'l': copy_library();  break;
    case 's': copy_syscall();  break;
    case 'p': copy_sendfile(); break;
    }

    return EXIT_SUCCESS;
}