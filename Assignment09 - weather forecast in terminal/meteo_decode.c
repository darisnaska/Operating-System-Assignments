#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "weather.h"

#define MAX_RECORDS (24 *16)

static json_object*
get_array(json_object *obj, const char *key, size_t *len)
{
    json_object *arr = NULL;
    if (!json_object_object_get_ex(obj, key, &arr) || !json_object_is_type (arr, json_type_array)) {
        fprintf(stderr, "Error: Key '%s' not found in JSON object.\n", key);
        return NULL;
    }

    size_t n = json_object_array_length(arr);
        if (n > MAX_RECORDS) {
            fprintf(stderr, "Error: Array '%s' exceeds maximum length of %d.\n", key, MAX_RECORDS);
            return NULL;
        }
        
        *len = n;
        return arr;
}

static int
get_number (json_object *arr, size_t i, double *out)
{
    json_object *v = json_object_array_get_idx(arr, i);
    if (v == NULL || json_object_is_type(v, json_type_null)) {
        *out = NAN;
        return 0;           
}

if (!json_object_is_type(v, json_type_double) && !json_object_is_type(v, json_type_int)) {
        fprintf(stderr, "Error: Array element at index %zu is not a number.\n", i);
        return -1;
    }

    double d = json_object_get_double(v);
    *out = isfinite(d) ? d : NAN;
    return 0;
}

weather_t*
meteo_decode (json_object *data){
    if (!data || !json_object_is_type(data, json_type_object)) {
        return NULL;
    }

    json_object *hourly;
    if (!json_object_object_get_ex(data, "hourly", &hourly) || !json_object_is_type(hourly, json_type_object)) {
        fprintf(stderr, "meteoL no 'hourly' object\n");
        return NULL;
    }

    size_t n, m;
    json_object *times = get_array(hourly, "time", &n);
    json_object *temp = get_array(hourly, "temperature_2m", &m);
    if (!times || !temp || n != m) {
        return NULL;
    }

    json_object *hum = get_array(hourly, "relative_humidity_2m", &m);
    json_object *prec = get_array(hourly, "precipitation", &m);
    json_object *wind = get_array(hourly, "wind_speed_10m", &m);
    if (!hum ||!prec || !wind) {
        return NULL;
    }

    weather_t *w= weather_new();
    if (!w) {
        return NULL;
    }

    for (size_t i = 0; i < n; i++) {
        weather_record_t r;
        memset(&r, 0, sizeof r);
 
        json_object *t = json_object_array_get_idx(times, i);
        if (!t || !json_object_is_type(t, json_type_string)) {
            goto fail;
        }
        const char *s = json_object_get_string(t);
        const char *end = s ? strptime(s, "%Y-%m-%dT%H:%M", &r.time) : NULL;
        if (!end || *end != '\0') {
            fprintf(stderr, "meteo: bad timestamp\n");
            goto fail;
        }
        r.time.tm_isdst = -1;
        {
            struct tm tmp = r.time;
            if (timegm(&tmp) == (time_t) -1) {   /* normalise, fills tm_wday */
                goto fail;
            }
            r.time = tmp;
        }
 
        if (get_number(temp, i, &r.temperature) < 0
            || get_number(hum, i, &r.humidity) < 0
            || get_number(prec, i, &r.precipitation) < 0
            || get_number(wind, i, &r.wind_speed) < 0) {
            fprintf(stderr, "meteo: bad numeric value\n");
            goto fail;
        }
        weather_append(w, &r);
    }
    return w;
 
fail:
    weather_free(w);
    return NULL;
}
