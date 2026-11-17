#ifndef _WEATHER_H 
#define _WEATHER_H

#include <json-c/json.h>
#include <time.h>

typedef struct {
    struct tm time;
    double temperature;
    double humidity;
    double wind_speed;
    double precipitation;
} weather_record_t;

typedef struct {
    size_t size;
    size_t capacity;
    weather_record_t *records;
}weather_t;

extern weather_t *weather_new(void);
extern weather_t *weather_new_from_meteo(void);
extern void weather_free(weather_t *weather);
extern void weather_reset(weather_t *weather);
extern void weather_append(weather_t *weather, weather_record_t *record);
extern void weather_print(weather_t *weather);

extern json_object* meteo_fetch_data(void);
extern weather_t* meteo_decode(json_object *data);

#endif