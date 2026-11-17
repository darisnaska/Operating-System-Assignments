#define _GNU_SOURCE
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "weather.h"

#define INITIAL_CAPACITY 32

weather_t*
weather_new(void)
{
    weather_t *w = calloc(1, sizeof *w);
    if (!w) {
        return NULL;
    }

    w->records = calloc(INITIAL_CAPACITY, sizeof *w->records);
    if (!w->records) {
        free(w);
        return NULL;
    }

    w->capacity = INITIAL_CAPACITY;
    return w;
}

weather_t*
weather_new_from_meteo(void)
{
    json_object *data = meteo_fetch_data();

    if (!data) {
        return NULL;
    }

    weather_t *w = meteo_decode(data);
    json_object_put(data);
    return w;
}

void 
weather_free(weather_t *weather)
{
    if (!weather) {
        return;
    }

    free (weather -> records);
    free (weather); 
}

void
weather_reset(weather_t *weather)
{
    if (weather) {
        weather -> size = 0;
    }
}

void 
weather_append(weather_t *weather, weather_record_t *record)
{
    if (!weather || !record) {
        return;
    }

    if (weather -> size >= weather -> capacity) {
        if (weather -> capacity > SIZE_MAX / 2 / sizeof *weather -> records) {
            fprintf (stderr, "Capacity Overflow");
            return;
        } 
        
        size_t ncap = weather -> capacity * 2;
        weather_record_t *p = realloc(weather -> records, ncap * sizeof *p);
        
        if (!p) {
            fprintf(stderr, "No memory");
            return;
        }
        weather -> records = p;
        weather -> capacity = ncap;
    }

    weather -> records[weather -> size++] = *record;
}

static void
print_value(double v, const char *fmt, const char *unit)
{
    if (isnan (v)) {
        printf(" %s %s", "n/a", unit);
    } else {
        printf(" ");
        printf(fmt, v);
        printf(" %s", unit);
    }
}

void 
weather_print(weather_t *weather)
{
    if (!weather) {
        return;
    }

    time_t now = time(NULL);
    struct tm cur;
    localtime_r(&now, &cur);

    int last_y = -1, last_m = -1, last_d = -1;
    for (size_t i = 0; i < weather -> size; ++i) {
        const weather_record_t *r = &weather -> records[i];
        const struct tm *t = &r -> time; 

                if (t -> tm_year != last_y || t -> tm_mon != last_m || t -> tm_mday != last_d) {
            char day[64];
            if (strftime(day, sizeof day, "%A %Y-%m-%d", t) == 0) {
                snprintf(day, sizeof day, "?");
            }
            printf("* %s\n", day);
            last_y = t -> tm_year;
            last_m = t -> tm_mon;
            last_d = t -> tm_mday;
        }

                int is_now = t -> tm_year == cur.tm_year && t -> tm_mon == cur.tm_mon
            && t -> tm_mday == cur.tm_mday && t -> tm_hour == cur.tm_hour;
        printf("%c %02d:%02d", is_now ? '>' : ' ', t -> tm_hour, t -> tm_min);
        print_value(r -> temperature, "%.1f", "C");
        print_value(r -> humidity, "%.1f", "%");
        print_value(r -> precipitation, "%.1f", "mm");
        print_value(r -> wind_speed, "%.1f", "km/h");
        printf("\n");
    }
}