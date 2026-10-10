/* GuardX RPi B fire-decision core. */
#ifndef CMD_BUILDER_H
#define CMD_BUILDER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "sensor_parser.h"

int CREATE_CMD_ACTION_JSON(char *buf, size_t bufsize,
                           const char *command, const char *action,
                           uint64_t timestamp_ms, uint32_t seq,
                           const char *reason, uint32_t sensor_seq);

int CREATE_CMD_SET_JSON(char *buf, size_t bufsize,
                        const char *command, int value,
                        uint64_t timestamp_ms, uint32_t seq,
                        const char *reason, uint32_t sensor_seq);

#endif
