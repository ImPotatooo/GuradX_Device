/* GuardX RPi B fire-decision core. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "sensor_parser.h"
#include "guardx_protocol.h"

static const char *find_value(const char *json, const char *key)
{
    char quoted[40];
    const char *p;

    snprintf(quoted, sizeof(quoted), "\"%s\"", key);
    p = strstr(json, quoted);
    if (!p)
        return NULL;

    p += strlen(quoted);
    while (*p && isspace((unsigned char)*p))
        p++;
    if (*p != ':')
        return NULL;
    p++;
    while (*p && isspace((unsigned char)*p))
        p++;
    return *p ? p : NULL;
}

static bool get_int(const char *json, const char *key, int *out)
{
    const char *p = find_value(json, key);

    if (!p || (*p != '-' && !isdigit((unsigned char)*p)))
        return false;
    *out = atoi(p);
    return true;
}

static bool get_u32(const char *json, const char *key, uint32_t *out)
{
    const char *p = find_value(json, key);

    if (!p || !isdigit((unsigned char)*p))
        return false;
    *out = (uint32_t)strtoul(p, NULL, 10);
    return true;
}

static bool get_double(const char *json, const char *key, double *out)
{
    const char *p = find_value(json, key);

    if (!p || (*p != '-' && !isdigit((unsigned char)*p)))
        return false;
    *out = strtod(p, NULL);
    return true;
}

static bool get_bool(const char *json, const char *key, bool *out)
{
    const char *p = find_value(json, key);

    if (!p)
        return false;
    if (strncmp(p, "true", 4) == 0)  { *out = true;  return true; }
    if (strncmp(p, "false", 5) == 0) { *out = false; return true; }
    return false;
}

guardx_err_t PARSE_SENSOR_JSON(const char *json, sensor_msg_t *out)
{
    if (json == NULL || out == NULL)
        return GUARDX_ERR_INVALID;

    memset(out, 0, sizeof(*out));

    if (!get_u32(json, GUARDX_JSON_KEY_SEQ, &out->seq))
        return GUARDX_ERR_INVALID;

    if (!get_bool(json, "gas", &out->gas_valid))
        out->gas_valid = false;
    if (!get_bool(json, "temphum", &out->temphum_valid))
        out->temphum_valid = false;
    if (!get_bool(json, "spark", &out->spark_valid))
        out->spark_valid = false;
    if (!get_bool(json, "irtemp", &out->irtemp_valid))
        out->irtemp_valid = false;

    if (!get_int(json, "gas_raw", &out->gas_raw))
        out->gas_valid = false;
    if (!get_int(json, "spark_raw", &out->spark_raw))
        out->spark_valid = false;
    if (!get_double(json, "temperature", &out->temperature) ||
        !get_double(json, "humidity", &out->humidity))
        out->temphum_valid = false;
    if (!get_double(json, "irtemp_ambient", &out->irtemp_ambient) ||
        !get_double(json, "irtemp_object", &out->irtemp_object))
        out->irtemp_valid = false;

    return GUARDX_OK;
}

guardx_err_t PARSE_BUTTON_JSON(const char *json, button_msg_t *out)
{
    if (json == NULL || out == NULL)
        return GUARDX_ERR_INVALID;

    memset(out, 0, sizeof(*out));

    if (!get_u32(json, GUARDX_JSON_KEY_SEQ, &out->seq))
        return GUARDX_ERR_INVALID;
    if (!get_u32(json, "press_count", &out->press_count))
        return GUARDX_ERR_INVALID;

    return GUARDX_OK;
}
