
#define _XOPEN_SOURCE 700

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* environ is not declared in any standard header; POSIX requires the
 * program itself to declare it if it wants to use it directly. */
extern char **environ;

static int verbose = 0;

static void print_environment(void)
{
    for (char **e = environ; *e != NULL; e++) {
        if (printf("%s\n", *e) < 0) {
            perror("env: printf");
            exit(EXIT_FAILURE);
        }
    }
}

/*
 * Decide whether a command line argument has the shape of an
 * environment variable assignment, i.e. "NAME=VALUE".
 *
 * We require:
 *   - at least one '=' character, and
 *   - a non-empty NAME part before the first '='.
 * VALUE may be empty (e.g. "FOO=" is a valid assignment that sets
 * FOO to the empty string).
 */
static int is_assignment(const char *s)
{
    const char *eq = strchr(s, '=');

    if (eq == NULL) {
        return 0;               /* no '=' at all -> not an assignment  */
    }
    if (eq == s) {
        return 0;               /* "=value" -> empty name, reject      */
    }
    return 1;
}

static void remove_variable(const char *name)
{
    if (verbose) {
        fprintf(stderr, "env: removing variable '%s'\n", name);
    }
    if (unsetenv(name) != 0) {
        fprintf(stderr, "env: unsetenv(\"%s\"): %s\n", name, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static void add_assignment(const char *assignment)
{
    char *copy = strdup(assignment);

    if (copy == NULL) {
        fprintf(stderr, "env: out of memory\n");
        exit(EXIT_FAILURE);
    }

    if (verbose) {
        fprintf(stderr, "env: setting '%s'\n", copy);
    }

    if (putenv(copy) != 0) {
        fprintf(stderr, "env: putenv(\"%s\"): %s\n", copy, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static void usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s [-v] [-u name]... [NAME=VALUE]... [COMMAND [ARG]...]\n",
        prog);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "+vu:")) != -1) {
        switch (opt) {
        case 'v':
            verbose = 1;
            break;
        case 'u':
            remove_variable(optarg);
            break;
        case '?':
        default:
            usage(argv[0]);
        }
    }

    int i = optind;

    while (i < argc && is_assignment(argv[i])) {
        add_assignment(argv[i]);
        i++;
    }

    if (i == argc) {
       
        print_environment();
        return EXIT_SUCCESS;
    }

    char **cmd_argv = &argv[i];

    if (verbose) {
        fprintf(stderr, "env: executing '%s'", cmd_argv[0]);
        for (int j = 1; cmd_argv[j] != NULL; j++) {
            fprintf(stderr, " '%s'", cmd_argv[j]);
        }
        fprintf(stderr, "\n");
    }

    execvp(cmd_argv[0], cmd_argv);

    fprintf(stderr, "env: '%s': %s\n", cmd_argv[0], strerror(errno));
    return (errno == ENOENT) ? 127 : 126;
}