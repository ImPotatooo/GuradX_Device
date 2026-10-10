/* GuardX RPi B fire-decision core. */
#ifndef SENSOR_PARSER_H
#define SENSOR_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include "guardx_err.h"

#define GUARDX_JSON_MAX 512

typedef struct {
    uint32_t seq;
    int      gas_raw;
    int      spark_raw;
    double   temperature;
    double   humidity;
    double   irtemp_ambient;
    double   irtemp_object;
    bool     gas_valid;
    bool     temphum_valid;
    bool     spark_valid;
    bool     irtemp_valid;
} sensor_msg_t;

typedef struct {
    uint32_t seq;
    uint32_t press_count;
} button_msg_t;

guardx_err_t PARSE_SENSOR_JSON(const char *json, sensor_msg_t *out);
guardx_err_t PARSE_BUTTON_JSON(const char *json, button_msg_t *out);

#endif
