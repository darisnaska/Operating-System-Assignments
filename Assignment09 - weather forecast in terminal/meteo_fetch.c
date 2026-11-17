#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "weather.h"

#define METEO_URL  "https://api.open-meteo.com/v1/forecast"
#define MAX_RESPONSE (4u * 1024u * 1024u)

static int
read_all (int fd, char **bufp, size_t *lenp)
{
    size_t cap = 8192, len = 0;
    char *buf = malloc(cap);
    if (!buf) {
        return -1;
    }

    for (;;) {
        if (len == cap) {
            if (cap >= MAX_RESPONSE) {
                free(buf);
                return -1;
            }
            cap *= 2;
            char *tmp = realloc(buf, cap);
            if (!tmp) {
                free(buf);
                return -1;
        }
        buf = tmp;
    }
    
        ssize_t n = read(fd, buf + len, cap - len);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("read");
            free(buf); 
            return -1;                         
        }
        if (n == 0) {
            break;
        }
        len += (size_t) n;
    }

    *bufp = buf;
    *lenp = len;
    return 0;
}

json_object*
meteo_fetch_data (void)

{
    int fd[2];
    if (pipe(fd) < 0) {
        return NULL;
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return NULL;
    }

    if (pid == 0) {
        close(fd[0]);
        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            _exit(127);
        }
            close(fd[1]);
            char *argv[] = {
                "curl", "-s", "-S", "-f", "-G",
            "--proto", "=https",
            "--max-time", "20",
            METEO_URL,
            "--data-urlencode", "latitude=52.52",
            "--data-urlencode", "longitude=13.405",
            "--data-urlencode", "timezone=Europe/Berlin",
            "--data-urlencode", "forecast_days=1",
            "--data-urlencode",
            "hourly=temperature_2m,relative_humidity_2m,precipitation,wind_speed_10m",
            NULL
        };
            execvp("curl", argv);
            _exit(127);            
    }
        close(fd[1]);
        char *buf = NULL;
        size_t len = 0;
        int rc = read_all(fd[0], &buf, &len);
        close(fd[0]);
        if (rc<0){
            kill (pid, SIGTERM);
        }

        int status;
        pid_t w;
        do {
            w = waitpid (pid, &status, 0);
        }
        while (w<0 && errno == EINTR);

        if (rc < 0){
            return NULL;
        }

        if (w < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "meteo: curl failed\n");
            free(buf);
            return NULL;
        }

        if (len == 0 || len > INT_MAX) {
            fprintf(stderr, "Invalid response length\n");
            free(buf);
            return NULL;
            }

        struct json_tokener *tok = json_tokener_new_ex(32);
        if (!tok) {
            free(buf);
            return NULL;
        }

        json_object *obj = json_tokener_parse_ex(tok, buf, (int) len);
        enum json_tokener_error jerr = json_tokener_get_error(tok); 
        if (jerr != json_tokener_success) {
            fprintf(stderr, "JSON parsing error: %s\n", json_tokener_error_desc(jerr));
            if (obj) {
                json_object_put(obj);
            }
            obj = NULL;
        }
        json_tokener_free(tok);
        free(buf);
        return obj;
    }

