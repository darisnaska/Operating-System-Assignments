#define _GNU_SOURCE

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "player.h"

#define MAX_FETCH_TRIES 10

player_t *
player_new(void)
{
    player_t *p = calloc(1, sizeof(player_t));

    if (!p) {
        return NULL;
    }
    p->chlng = chlng_new();
    if (!p->chlng) {
        free(p);
        return NULL;
    }
    return p;
}

void
player_reset(player_t *p)
{
    if (!p) {
        return;
    }
    p->solved = 0;
    p->total = 0;
    p->finished = false;
    chlng_reset(p->chlng);
}

void
player_del(player_t *p)
{
    if (!p) {
        return;
    }
    chlng_del(p->chlng);
    free(p);
}

/* Fetch a text that contains at least one word we can hide. */
int
player_fetch_chlng(player_t *p)
{
    int i;

    if (!p || !p->chlng) {
        return -1;
    }
    for (i = 0; i < MAX_FETCH_TRIES; i++) {
        if (chlng_fetch_text(p->chlng) > 0 && chlng_hide_word(p->chlng) > 0) {
            return 0;
        }
    }
    return -1;
}

int
player_get_greeting(player_t *p, char **msg)
{
    int rc;

    *msg = NULL;
    if (!p) {
        return -1;
    }
    rc = asprintf(msg, "M: Guess the missing ____!\r\n"
                       "M: Send your guess in the form 'R: word\\r\\n'.\r\n");
    if (rc < 0) {
        *msg = NULL;
    }
    return rc;
}

/*
 * Return the current challenge, fetching a new one if necessary. If
 * the client did not answer properly, the same challenge is repeated.
 */
int
player_get_challenge(player_t *p, char **msg)
{
    int rc;

    *msg = NULL;
    if (!p) {
        return -1;
    }
    if (!p->chlng->text || !p->chlng->word) {
        if (player_fetch_chlng(p) != 0) {
            p->finished = true;
            return asprintf(msg, "M: Cannot create a challenge. Good bye!\r\n");
        }
    }
    rc = asprintf(msg, "C: %s\r\n", p->chlng->text);
    if (rc < 0) {
        *msg = NULL;
    }
    return rc;
}

/* Strip leading and trailing white space in place. */
static char *
trim(char *s)
{
    char *e;

    while (isspace((unsigned char) *s)) {
        s++;
    }
    e = s + strlen(s);
    while (e > s && isspace((unsigned char) e[-1])) {
        *--e = '\0';
    }
    return s;
}

int
player_post_challenge(player_t *p, char *msg, char **resp)
{
    char *line, *guess;
    int rc;

    *resp = NULL;
    if (!p || !msg) {
        return -1;
    }

    line = strdup(msg);
    if (!line) {
        return -1;
    }
    guess = trim(line);

    if (strncmp(guess, "Q:", 2) == 0) {
        p->finished = true;
        rc = asprintf(resp, "M: You mastered %d/%d challenges. Good bye!\r\n",
                      p->solved, p->total);
    } else if (strncmp(guess, "R:", 2) == 0) {
        guess = trim(guess + 2);
        if (!p->chlng->word) {
            rc = asprintf(resp, "M: No challenge pending.\r\n");
        } else if (*guess == '\0') {
            rc = asprintf(resp, "M: Missing word - use 'R: word\\r\\n'.\r\n");
        } else {
            p->total++;
            if (strcasecmp(guess, p->chlng->word) == 0) {
                p->solved++;
                rc = asprintf(resp, "O: Congratulation - challenge passed!\r\n");
            } else {
                rc = asprintf(resp, "F: Wrong guess '%s' - expected '%s'\r\n",
                              guess, p->chlng->word);
            }
            chlng_reset(p->chlng);   /* next call fetches a new challenge */
        }
    } else {
        rc = asprintf(resp, "M: Unknown message - use 'R: word\\r\\n' "
                            "or 'Q:\\r\\n'.\r\n");
    }

    free(line);
    if (rc < 0) {
        *resp = NULL;
    }
    return rc;
}
