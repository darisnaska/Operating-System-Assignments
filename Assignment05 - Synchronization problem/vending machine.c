#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define MAX_RUNOUTS 3

static struct {
    pthread_mutex_t lock;
    pthread_cond_t  started_cv;  /* main -> everybody: all threads exist   */
    pthread_cond_t  ready_cv;    /* machine -> students: next customer     */
    pthread_cond_t  coin_cv;     /* student -> machine: all coins inserted */
    pthread_cond_t  drink_cv;    /* machine -> student: drink dispensed    */
    pthread_cond_t  pickup_cv;   /* student -> machine: drink picked up    */
    pthread_cond_t  empty_cv;    /* machine -> supplier: please refill     */
    pthread_cond_t  refill_cv;   /* supplier -> machine: refilled          */

    int  capacity, price;
    int  drinks, coins, inserted;
    int  runouts;
    bool started, ready, drink_ready, picked_up, needs_refill, off, done;
} E = {
    .lock       = PTHREAD_MUTEX_INITIALIZER,
    .started_cv = PTHREAD_COND_INITIALIZER,
    .ready_cv   = PTHREAD_COND_INITIALIZER,
    .coin_cv    = PTHREAD_COND_INITIALIZER,
    .drink_cv   = PTHREAD_COND_INITIALIZER,
    .pickup_cv  = PTHREAD_COND_INITIALIZER,
    .empty_cv   = PTHREAD_COND_INITIALIZER,
    .refill_cv  = PTHREAD_COND_INITIALIZER,
};

static void say(const char *fmt, ...)
{
    va_list ap;
    printf("energy: [%d/%d drinks, %d coins, %d inserted] ",
           E.drinks, E.capacity, E.coins, E.inserted);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
    fflush(stdout);
}

static void wait_for_start(void)
{
    while (!E.started)
        pthread_cond_wait(&E.started_cv, &E.lock);
}

static void *student(void *arg)
{
    int id = (int)(long)arg;
    int last = 0;                      
    pthread_mutex_lock(&E.lock);
    say("student %d established", id);
    wait_for_start();
    say("student %d requires an energy drink", id);

    /* Wait until the machine invites us (idle + has a drink). */
    while (!E.ready && !E.off) {
        int now = (E.drinks == 0) ? 1 : 2;
        if (now != last) {
            if (now == 1)
                say("student %d waiting for machine to be refilled", id);
            else
                say("student %d waiting for the machine", id);
            last = now;
        }
        pthread_cond_wait(&E.ready_cv, &E.lock);
    }
    if (!E.ready) {                     //machines is off//
        say("student %d gives up, machine is out of order", id);
        pthread_mutex_unlock(&E.lock);
        return NULL;
    }

    E.ready = false;
    say("student %d is next to be served", id);
    for (int k = 0; k < E.price; k++) {
        E.inserted++;
        say("student %d inserted another coin", id);
    }
    pthread_cond_signal(&E.coin_cv);

    say("student %d waiting for drink to arrive", id);
    while (!E.drink_ready)
        pthread_cond_wait(&E.drink_cv, &E.lock);

    E.drink_ready = false;
    E.picked_up = true;
    say("student %d picked up a drink", id);
    pthread_cond_signal(&E.pickup_cv);
    say("student %d enjoying an energy drink", id);
    pthread_mutex_unlock(&E.lock);
    return NULL;
}

static void *machine(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&E.lock);
    wait_for_start();
    say("machine booting up");

    for (;;) {
        /* Empty? Ask the supplier and wait for the refill. */
        if (E.drinks == 0) {
            if (E.runouts > 0)
                say("machine out of drinks, waiting for supplier");
            E.needs_refill = true;
            pthread_cond_signal(&E.empty_cv);
            while (E.drinks == 0 && !E.done)
                pthread_cond_wait(&E.refill_cv, &E.lock);
        }
        if (E.done)
            break;

        /* Invite exactly one student, then wait for her coins. */
        E.ready = true;
        say("machine waiting for more coins");
        pthread_cond_broadcast(&E.ready_cv);
        while (E.inserted < E.price && !E.done)
            pthread_cond_wait(&E.coin_cv, &E.lock);
        if (E.done)
            break;

        /* Dispense. */
        E.drinks--;
        E.coins += E.price;
        E.inserted = 0;
        say("machine dispensing drink");
        E.drink_ready = true;
        pthread_cond_signal(&E.drink_cv);

        /* The next transaction may only start after the pickup. */
        say("machine waiting for pickup of drink");
        while (!E.picked_up)
            pthread_cond_wait(&E.pickup_cv, &E.lock);
        E.picked_up = false;

        if (E.drinks == 0 && ++E.runouts == MAX_RUNOUTS) {
            E.off = true;
            say("machine ran out of drinks %d times, shutting off permanently",
                MAX_RUNOUTS);
            pthread_cond_broadcast(&E.ready_cv);   /* release waiters */
            pthread_mutex_unlock(&E.lock);
            return NULL;
        }
    }
    say("machine shutting down");
    pthread_mutex_unlock(&E.lock);
    return NULL;
}

static void *supplier(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&E.lock);
    say("supplier established");
    wait_for_start();

    for (;;) {
        while (!E.needs_refill && !E.done)
            pthread_cond_wait(&E.empty_cv, &E.lock);
        if (E.done)
            break;

        pthread_mutex_unlock(&E.lock);      /* travelling to the machine */
        nanosleep(&(struct timespec){ .tv_nsec = 20 * 1000 * 1000 }, NULL);
        pthread_mutex_lock(&E.lock);
        if (E.done)
            break;

        say("supplier arriving");
        int n = E.capacity - E.drinks;
        say("supplier loading %d drinks", n);
        E.drinks += n;
        int c = E.coins;
        E.coins = 0;
        say("supplier collected %d coins", c);
        E.needs_refill = false;
        say("supplier leaving");
        pthread_cond_broadcast(&E.refill_cv);
    }
    pthread_mutex_unlock(&E.lock);
    return NULL;
}

static void usage(const char *prog)
{
    fprintf(stderr, "usage: %s [-n students] [-c capacity] [-p price]\n", prog);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int n = 2;
    E.capacity = 4;
    E.price = 5;

    int opt;
    while ((opt = getopt(argc, argv, "n:c:p:")) != -1) {
        switch (opt) {
        case 'n': n = atoi(optarg);          break;
        case 'c': E.capacity = atoi(optarg); break;
        case 'p': E.price = atoi(optarg);    break;
        default:  usage(argv[0]);
        }
    }
    if (n < 1 || E.capacity < 1 || E.price < 1)
        usage(argv[0]);

    pthread_t *st = calloc((size_t)n, sizeof *st);
    pthread_t sup, mach;
    if (!st) { perror("calloc"); return EXIT_FAILURE; }

    for (long i = 0; i < n; i++)
        if (pthread_create(&st[i], NULL, student, (void *)i)) {
            perror("pthread_create"); return EXIT_FAILURE;
        }
    if (pthread_create(&sup, NULL, supplier, NULL) ||
        pthread_create(&mach, NULL, machine, NULL)) {
        perror("pthread_create"); return EXIT_FAILURE;
    }

    /* Let everybody run. */
    pthread_mutex_lock(&E.lock);
    E.started = true;
    pthread_cond_broadcast(&E.started_cv);
    pthread_mutex_unlock(&E.lock);

    /* Every student either got a drink or gave up. */
    for (int i = 0; i < n; i++)
        pthread_join(st[i], NULL);

    /* Tell machine and supplier to go home. */
    pthread_mutex_lock(&E.lock);
    E.done = true;
    pthread_cond_broadcast(&E.coin_cv);
    pthread_cond_broadcast(&E.refill_cv);
    pthread_cond_broadcast(&E.empty_cv);
    pthread_mutex_unlock(&E.lock);

    pthread_join(sup, NULL);
    pthread_join(mach, NULL);
    free(st);
    return EXIT_SUCCESS;
}