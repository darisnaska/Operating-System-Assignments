
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define N_PRISONERS 100   /* prisoners == drawers == numbers 0..99          */
#define N_TRIES      50   /* each prisoner may open at most 50 drawers      */
#define DEFAULT_GAMES 100

/* ---------------------------------------------------------------------- */
/* shared game state                                                       */
/* ---------------------------------------------------------------------- */

static int drawers[N_PRISONERS];                 /* drawers[i] = number inside */
static pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t drawer_lock[N_PRISONERS]; /* one mutex per drawer       */

typedef struct {
    pthread_t    tid;
    int          id;     /* the prisoner's number (0..99)                   */
    unsigned int seed;   /* private RNG state for rand_r() (thread safe)    */
    int          found;  /* result: 1 if this prisoner found his number     */
} prisoner_t;

static prisoner_t prisoners[N_PRISONERS];

static void
init_drawers(unsigned int *seed)
{
    for (int i = 0; i < N_PRISONERS; i++) {
        drawers[i] = i;
    }
    for (int i = N_PRISONERS - 1; i > 0; i--) {
        int j = (int) (rand_r(seed) % (unsigned) (i + 1));
        int t = drawers[i];
        drawers[i] = drawers[j];
        drawers[j] = t;
    }
}

/* Open drawer without locking (caller already holds the global lock). */
static inline int
open_unlocked(int d)
{
    return drawers[d];
}

/* Open drawer while holding only that drawer's lock, then close it again. */
static inline int
open_locked(int d)
{
    pthread_mutex_lock(&drawer_lock[d]);
    int v = drawers[d];
    pthread_mutex_unlock(&drawer_lock[d]);
    return v;
}

static void
pick_random_drawers(prisoner_t *p, int *order)
{
    for (int i = 0; i < N_PRISONERS; i++) {
        order[i] = i;
    }
    for (int k = 0; k < N_TRIES; k++) {
        int j = k + (int) (rand_r(&p->seed) % (unsigned) (N_PRISONERS - k));
        int t = order[k];
        order[k] = order[j];
        order[j] = t;
    }
}

/* ---------------------------------------------------------------------- */
/* the four strategies (thread entry points)                               */
/* ---------------------------------------------------------------------- */

static void *
random_global(void *arg)
{
    prisoner_t *p = arg;
    int order[N_PRISONERS];

    pick_random_drawers(p, order);          /* private work, no lock needed */

    pthread_mutex_lock(&global_lock);       /* whole search under one lock  */
    p->found = 0;
    for (int k = 0; k < N_TRIES; k++) {
        if (open_unlocked(order[k]) == p->id) {
            p->found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&global_lock);
    return NULL;
}

static void *
random_drawer(void *arg)
{
    prisoner_t *p = arg;
    int order[N_PRISONERS];

    pick_random_drawers(p, order);

    p->found = 0;
    for (int k = 0; k < N_TRIES; k++) {
        if (open_locked(order[k]) == p->id) {   /* lock/unlock per drawer */
            p->found = 1;
            break;
        }
    }
    return NULL;
}

static void *
smart_global(void *arg)
{
    prisoner_t *p = arg;

    pthread_mutex_lock(&global_lock);
    p->found = 0;
    int d = p->id;                          /* start at drawer with own number */
    for (int k = 0; k < N_TRIES; k++) {
        int v = open_unlocked(d);
        if (v == p->id) {
            p->found = 1;
            break;
        }
        d = v;                              /* follow the number found        */
    }
    pthread_mutex_unlock(&global_lock);
    return NULL;
}

static void *
smart_drawer(void *arg)
{
    prisoner_t *p = arg;

    p->found = 0;
    int d = p->id;
    for (int k = 0; k < N_TRIES; k++) {
        int v = open_locked(d);             /* at most ONE lock held at a time */
        if (v == p->id) {
            p->found = 1;
            break;
        }
        d = v;
    }
    return NULL;
}

/* Create n threads that all run proc, then join them. */
static void
run_threads(int n, void *(*proc)(void *))
{
    for (int i = 0; i < n; i++) {
        int rc = pthread_create(&prisoners[i].tid, NULL, proc, &prisoners[i]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < n; i++) {
        int rc = pthread_join(prisoners[i].tid, NULL);
        if (rc != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(rc));
            exit(EXIT_FAILURE);
        }
    }
}

static double
now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static double
timeit(int games, void *(*proc)(void *), unsigned int *seed, int *wins)
{
    double total = 0.0;
    *wins = 0;

    for (int g = 0; g < games; g++) {
        init_drawers(seed);
        for (int i = 0; i < N_PRISONERS; i++) {
            prisoners[i].id    = i;
            prisoners[i].seed  = rand_r(seed);  /* independent per-thread seed */
            prisoners[i].found = 0;
        }

        double t1 = now_ms();
        run_threads(N_PRISONERS, proc);
        double t2 = now_ms();
        total += t2 - t1;

        int all = 1;                            /* all must succeed to win    */
        for (int i = 0; i < N_PRISONERS; i++) {
            if (!prisoners[i].found) {
                all = 0;
                break;
            }
        }
        *wins += all;
    }
    return total;
}

int
main(int argc, char *argv[])
{
    int games = DEFAULT_GAMES;
    int nonstatic_seed = 0;
    int opt;

    while ((opt = getopt(argc, argv, "n:s")) != -1) {
        switch (opt) {
        case 'n':
            games = atoi(optarg);
            if (games <= 0) {
                fprintf(stderr, "invalid number of games: %s\n", optarg);
                return EXIT_FAILURE;
            }
            break;
        case 's':
            nonstatic_seed = 1;
            break;
        default:
            fprintf(stderr, "usage: %s [-n games] [-s]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    unsigned int seed = 42;                     /* static, reproducible seed  */
    if (nonstatic_seed) {
        seed = (unsigned int) time(NULL) ^ ((unsigned int) getpid() << 16);
    }

    for (int i = 0; i < N_PRISONERS; i++) {
        pthread_mutex_init(&drawer_lock[i], NULL);
    }

    static const struct {
        const char *name;
        void *(*proc)(void *);
    } methods[] = {
        { "random_global", random_global },
        { "random_drawer", random_drawer },
        { "smart_global",  smart_global  },
        { "smart_drawer",  smart_drawer  },
    };

    for (size_t m = 0; m < sizeof(methods) / sizeof(methods[0]); m++) {
        int wins;
        double total = timeit(games, methods[m].proc, &seed, &wins);
        printf("%-14s %d/%d wins/games = %.2f%% %.3f/%d ms = %.3f ms\n",
               methods[m].name, wins, games, 100.0 * wins / games,
               total, games, total / games);
    }

    for (int i = 0; i < N_PRISONERS; i++) {
        pthread_mutex_destroy(&drawer_lock[i]);
    }
    return EXIT_SUCCESS;
}