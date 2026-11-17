#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include "weather.h"

int 
main (void)
{
    setenv("TZ", "Europe/Berlin",1);
    tzset();

    weather_t *weather = weather_new_from_meteo();
    if (!weather) {
        fprintf(stderr, "Failed to get weather data\n");
        return EXIT_FAILURE;
    }

    weather_print(weather);
    weather_free(weather);
    return EXIT_SUCCESS;
}