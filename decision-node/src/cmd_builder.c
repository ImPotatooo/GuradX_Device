/* GuardX RPi B fire-decision core. */
#include <stdio.h>

#include "cmd_builder.h"
#include "guardx_protocol.h"

static int build_head(char *buf, size_t bufsize,
                      const char *command, const char *action,
                      uint64_t timestamp_ms, uint32_t seq)
{
    return snprintf(buf, bufsize,
        "{"
        "\"node_id\":\"" GUARDX_NODE_RPIB "\","
        "\"timestamp\":%llu,"
        "\"seq\":%u,"
        "\"command\":\"%s\","
        "\"action\":\"%s\"",
        (unsigned long long)timestamp_ms, seq, command, action);
}

static int build_tail(char *buf, size_t bufsize,
                      const char *reason, uint32_t sensor_seq)
{
    if (reason == NULL)
        return snprintf(buf, bufsize, "}");
    return snprintf(buf, bufsize,
                    ",\"reason\":\"%s\",\"sensor_seq\":%u}",
                    reason, sensor_seq);
}

int CREATE_CMD_ACTION_JSON(char *buf, size_t bufsize,
                           const char *command, const char *action,
                           uint64_t timestamp_ms, uint32_t seq,
                           const char *reason, uint32_t sensor_seq)
{
    int n = build_head(buf, bufsize, command, action, timestamp_ms, seq);

    if (n < 0 || (size_t)n >= bufsize)
        return n;
    return n + build_tail(buf + n, bufsize - n, reason, sensor_seq);
}

int CREATE_CMD_SET_JSON(char *buf, size_t bufsize,
                        const char *command, int value,
                        uint64_t timestamp_ms, uint32_t seq,
                        const char *reason, uint32_t sensor_seq)
{
    int n = build_head(buf, bufsize, command, GUARDX_ACTION_SET,
                       timestamp_ms, seq);

    if (n < 0 || (size_t)n >= bufsize)
        return n;
    n += snprintf(buf + n, bufsize - n, ",\"value\":%d", value);
    if ((size_t)n >= bufsize)
        return n;
    return n + build_tail(buf + n, bufsize - n, reason, sensor_seq);
}
