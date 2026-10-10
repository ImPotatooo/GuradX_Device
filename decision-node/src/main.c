#define _POSIX_C_SOURCE 200809L
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "cmd_builder.h"
#include "decision.h"
#include "guardx_protocol.h"
#include "mqtt_bus.h"
#include "sensor_parser.h"

/* A focused, single-zone demonstration of the original RPi B pipeline. */
static volatile sig_atomic_t running = 1;
static decision_zone_t zone;
static uint32_t command_seq;
static int live_mode;

static void stop_running(int signum)
{
    (void)signum;
    running = 0;
}

static uint64_t now_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
        return 0;
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static void emit_command(const char *command, const char *action, int value,
                         int has_value, uint32_t sensor_seq)
{
    char json[GUARDX_JSON_MAX];
    int len;
    if (has_value)
        len = CREATE_CMD_SET_JSON(json, sizeof(json), command, value,
                                  now_ms(), command_seq++, "fire", sensor_seq);
    else
        len = CREATE_CMD_ACTION_JSON(json, sizeof(json), command, action,
                                     now_ms(), command_seq++, "fire", sensor_seq);
    if (len < 0 || (size_t)len >= sizeof(json)) {
        fprintf(stderr, "command serialization failed\n");
        return;
    }
    if (live_mode) {
        if (mqtt_bus_publish_actuator("rpic", json, len) != 0)
            fprintf(stderr, "actuator publication failed: %s\n", command);
    } else {
        printf("dry-run -> guardx/actuator/rpic %s\n", json);
    }
}

static void on_sensor(const char *node_id, const char *json, int len)
{
    sensor_msg_t sensor;
    decision_event_t event;
    (void)len;
    if (strcmp(node_id, "rpia") != 0 ||
        PARSE_SENSOR_JSON(json, &sensor) != GUARDX_OK)
        return;
    event = DECISION_FEED(&zone, &sensor);
    printf("sensor seq=%" PRIu32 " score=%.1f state=%s\n", sensor.seq,
           DECISION_LAST_SCORE(&zone),
           DECISION_STATE(&zone) == DECISION_STATE_FIRE ? "FIRE" : "NORMAL");
    if (event != DECISION_EVENT_FIRE)
        return;
    printf("fire confirmed: cause=%s sensor_seq=%" PRIu32 "\n",
           DECISION_CAUSE_STR(DECISION_LAST_CAUSE(&zone)), sensor.seq);
    emit_command(GUARDX_CMD_SERVO_1, NULL, 90, 1, sensor.seq);
    emit_command(GUARDX_CMD_SHUTTER, GUARDX_ACTION_CLOSE, 0, 0, sensor.seq);
    emit_command(GUARDX_CMD_WATER_PUMP, GUARDX_ACTION_ON, 0, 0, sensor.seq);
}

int main(int argc, char **argv)
{
    char line[GUARDX_JSON_MAX + 2];
    live_mode = argc == 2 && strcmp(argv[1], "--mqtt") == 0;
    if (argc > 2 || (argc == 2 && !live_mode)) {
        fprintf(stderr, "usage: %s [--mqtt]\n", argv[0]);
        return 2;
    }
    DECISION_ZONE_INIT(&zone);
    if (!live_mode) {
        puts("dry-run: enter one RPi A sensor JSON message per line");
        while (fgets(line, sizeof(line), stdin)) {
            if (!strchr(line, '\n') && !feof(stdin)) {
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) {}
                fprintf(stderr, "sensor message too long\n");
                continue;
            }
            on_sensor("rpia", line, (int)strlen(line));
        }
        return 0;
    }
    signal(SIGINT, stop_running);
    signal(SIGTERM, stop_running);
    if (mqtt_bus_init(on_sensor) != 0) {
        fprintf(stderr, "cannot connect to local MQTT broker\n");
        return 1;
    }
    puts("listening on guardx/sensor/+; Ctrl+C to stop");
    while (running)
        sleep(1);
    mqtt_bus_cleanup();
    return 0;
}
