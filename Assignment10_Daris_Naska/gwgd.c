/*
 * gwgd.c --
 *
 * Word guessing game daemon.
 *
 *   gwgd [-t] [-f] [-p port]
 *
 *   -t   handle every client in a separate thread
 *   -f   handle every client in a separate child process
 *   -p   port to listen on (default 1234)
 *
 * Without -t and -f only one client is served at a time, further
 * clients wait in the listen queue until the current one is done.
 *
 * The game logic (player.c, chlng.c) is shared by all three modes; the
 * only mode specific code is in serve().
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "player.h"
#include "tcp.h"

#define DEFAULT_PORT "1234"
#define MAX_LISTEN   8
#define LINE_SIZE    512

typedef enum { MODE_BLOCK, MODE_FORK, MODE_THREAD } srv_mode_t;

/* ------------------------------------------------------------------ */
/* Game play on one connection (the same for all server modes).        */
/* ------------------------------------------------------------------ */

/* Send a message produced by the player object and free it. */
static int
send_reply(int fd, int len, char *msg)
{
    int rc = 0;

    if (len > 0 && msg) {
        rc = tcp_write_all(fd, msg, (size_t) len);
    } else if (len < 0) {
        rc = -1;
    }
    free(msg);
    return rc;
}

/* Read one line; overly long lines are truncated. 0 on EOF/error. */
static int
read_line(FILE *in, char *buf, size_t size)
{
    if (!fgets(buf, (int) size, in)) {
        return 0;
    }
    if (!strchr(buf, '\n')) {
        int ch;
        while ((ch = fgetc(in)) != EOF && ch != '\n') {
            /* discard the rest of the line */
        }
    }
    return 1;
}

/* Play the game with the client on fd. Takes ownership of fd. */
static void
play(int fd)
{
    player_t *p = player_new();
    FILE *in = fdopen(fd, "r");
    char line[LINE_SIZE];
    char *msg;
    int len;

    if (!p || !in) {
        player_del(p);
        if (in) {
            (void) fclose(in);
        } else {
            (void) close(fd);
        }
        return;
    }

    len = player_get_greeting(p, &msg);
    if (send_reply(fd, len, msg) < 0) {
        goto done;
    }

    while (!p->finished) {
        len = player_get_challenge(p, &msg);
        if (send_reply(fd, len, msg) < 0 || p->finished) {
            break;
        }
        if (!read_line(in, line, sizeof(line))) {
            (void) snprintf(line, sizeof(line), "Q:");   /* client left */
        }
        len = player_post_challenge(p, line, &msg);
        if (send_reply(fd, len, msg) < 0) {
            break;
        }
    }

done:
    player_del(p);
    (void) fclose(in);   /* also closes fd */
}

/* ------------------------------------------------------------------ */
/* Server modes.                                                       */
/* ------------------------------------------------------------------ */

static void
reap_children(int sig)
{
    int saved = errno;

    (void) sig;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        /* collect all terminated children, no zombies */
    }
    errno = saved;
}

static void *
client_thread(void *arg)
{
    int fd = *(int *) arg;

    free(arg);
    play(fd);
    return NULL;
}

static void
serve(srv_mode_t mode, int fd, const int lfds[], int nl)
{
    pid_t pid;
    pthread_t tid;
    pthread_attr_t attr;
    int *arg, i;

    switch (mode) {
    case MODE_BLOCK:
        play(fd);
        break;

    case MODE_FORK:
        pid = fork();
        if (pid == 0) {
            for (i = 0; i < nl; i++) {
                (void) close(lfds[i]);
            }
            (void) signal(SIGCHLD, SIG_DFL);   /* popen()/pclose() need it */
            play(fd);
            _exit(EXIT_SUCCESS);
        }
        if (pid < 0) {
            perror("fork");
        }
        (void) close(fd);
        break;

    case MODE_THREAD:
        arg = malloc(sizeof(*arg));
        if (!arg) {
            (void) close(fd);
            break;
        }
        *arg = fd;
        (void) pthread_attr_init(&attr);
        (void) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&tid, &attr, client_thread, arg) != 0) {
            perror("pthread_create");
            free(arg);
            (void) close(fd);
        }
        (void) pthread_attr_destroy(&attr);
        break;
    }
}

/* ------------------------------------------------------------------ */

static void
usage(void)
{
    (void) fprintf(stderr, "Usage: gwgd [-t] [-f] [-p port]\n");
    exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
    srv_mode_t mode = MODE_BLOCK;
    const char *port = DEFAULT_PORT;
    int lfds[MAX_LISTEN], nl, i, c;
    int use_t = 0, use_f = 0;

    while ((c = getopt(argc, argv, "tfp:")) != -1) {
        switch (c) {
        case 't':
            use_t = 1;
            break;
        case 'f':
            use_f = 1;
            break;
        case 'p':
            port = optarg;
            break;
        default:
            usage();
        }
    }
    if (optind != argc || (use_t && use_f)) {
        usage();
    }
    mode = use_t ? MODE_THREAD : (use_f ? MODE_FORK : MODE_BLOCK);

    /* A client that disconnects must not kill the server. */
    (void) signal(SIGPIPE, SIG_IGN);

    if (mode == MODE_FORK) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = reap_children;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
        if (sigaction(SIGCHLD, &sa, NULL) < 0) {
            perror("sigaction");
            return EXIT_FAILURE;
        }
    }

    nl = tcp_listen(port, lfds, MAX_LISTEN);
    if (nl < 0) {
        (void) fprintf(stderr, "gwgd: cannot listen on port %s\n", port);
        return EXIT_FAILURE;
    }

    for (;;) {
        fd_set rfds;
        int maxfd = -1;

        FD_ZERO(&rfds);
        for (i = 0; i < nl; i++) {
            FD_SET(lfds[i], &rfds);
            if (lfds[i] > maxfd) {
                maxfd = lfds[i];
            }
        }
        if (select(maxfd + 1, &rfds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }
        for (i = 0; i < nl; i++) {
            int fd;

            if (!FD_ISSET(lfds[i], &rfds)) {
                continue;
            }
            fd = accept(lfds[i], NULL, NULL);
            if (fd < 0) {
                if (errno != EINTR && errno != ECONNABORTED
                    && errno != EAGAIN) {
                    perror("accept");
                }
                continue;
            }
            /* keep the client socket away from popen()ed fortune */
            (void) fcntl(fd, F_SETFD, FD_CLOEXEC);
            serve(mode, fd, lfds, nl);
        }
    }

    for (i = 0; i < nl; i++) {
        (void) close(lfds[i]);
    }
    return EXIT_FAILURE;
}