#define _GNU_SOURCE

#include <ctype.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "chlng.h"

#define FORTUNE_CMD "fortune -s 2>/dev/null"
#define TEXT_MAX    1024

/* Used if fortune is not installed or produces no output. */
static const char *fallback[] = {
    "Is this really happening?",
    "You are standing on my toes.",
    "Are you a turtle?",
    "You'll be sorry...",
    "You should go home.",
    "There is a fly on your nose.",
    "The quick brown fox jumps over the lazy dog.",
    "Time flies like an arrow; fruit flies like a banana.",
};

/*
 * Thread-safe random numbers. Every thread has its own seed and the
 * seed is re-initialised after a fork() so that children do not
 * produce identical sequences.
 */
static unsigned int
rnd(void)
{
    static __thread unsigned int seed;
    static __thread pid_t seeded_pid;
    pid_t pid = getpid();

    if (seeded_pid != pid) {
        seed = (unsigned int) time(NULL) ^ ((unsigned int) pid << 8)
            ^ (unsigned int) (uintptr_t) pthread_self();
        seeded_pid = pid;
    }
    return (unsigned int) rand_r(&seed);
}

/*
 * Turn the text into a single line: control characters and runs of
 * white space become a single blank, leading/trailing blanks vanish.
 * The protocol is line based, so challenges must not contain newlines.
 */
static void
normalise(char *s)
{
    char *start = s, *w = s;
    int pending = 0;

    for (; *s; s++) {
        unsigned char ch = (unsigned char) *s;
        if (isspace(ch) || iscntrl(ch)) {
            pending = 1;
            continue;
        }
        if (pending && w != start) {
            *w++ = ' ';
        }
        pending = 0;
        *w++ = (char) ch;
    }
    *w = '\0';
}

chlng_t *
chlng_new(void)
{
    return calloc(1, sizeof(chlng_t));
}

void
chlng_reset(chlng_t *c)
{
    if (!c) {
        return;
    }
    free(c->text);
    free(c->word);
    c->text = NULL;
    c->word = NULL;
}

void
chlng_del(chlng_t *c)
{
    if (!c) {
        return;
    }
    chlng_reset(c);
    free(c);
}

/*
 * Run 'fortune -s' as a child process and read its output from a
 * pipe. Returns the length of the text or -1 on error.
 */
int
chlng_fetch_text(chlng_t *c)
{
    char buf[TEXT_MAX];
    size_t n = 0, r;
    FILE *f;

    if (!c) {
        return -1;
    }
    chlng_reset(c);

    f = popen(FORTUNE_CMD, "r");
    if (f) {
        while (n < sizeof(buf) - 1
               && (r = fread(buf + n, 1, sizeof(buf) - 1 - n, f)) > 0) {
            n += r;
        }
        (void) pclose(f);
    }
    buf[n] = '\0';
    normalise(buf);

    if (buf[0] == '\0') {
        const char *s = fallback[rnd() % (sizeof(fallback) / sizeof(fallback[0]))];
        (void) snprintf(buf, sizeof(buf), "%s", s);
    }

    c->text = strdup(buf);
    return c->text ? (int) strlen(c->text) : -1;
}

/*
 * Pick a random word (a run of letters), remember it and replace it
 * in the text by underscores. Returns the word length or -1 if the
 * text does not contain a word.
 */
int
chlng_hide_word(chlng_t *c)
{
    size_t len, i, nwords = 0, pick, start = 0, wlen = 0;

    if (!c || !c->text) {
        return -1;
    }
    free(c->word);
    c->word = NULL;

    len = strlen(c->text);
    for (i = 0; i < len; ) {
        if (isalpha((unsigned char) c->text[i])) {
            nwords++;
            while (i < len && isalpha((unsigned char) c->text[i])) {
                i++;
            }
        } else {
            i++;
        }
    }
    if (nwords == 0) {
        return -1;
    }

    pick = rnd() % nwords;
    for (i = 0; i < len; ) {
        if (isalpha((unsigned char) c->text[i])) {
            size_t b = i;
            while (i < len && isalpha((unsigned char) c->text[i])) {
                i++;
            }
            if (pick-- == 0) {
                start = b;
                wlen = i - b;
                break;
            }
        } else {
            i++;
        }
    }

    c->word = strndup(c->text + start, wlen);
    if (!c->word) {
        return -1;
    }
    memset(c->text + start, '_', wlen);
    return (int) wlen;
}
