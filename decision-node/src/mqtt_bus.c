#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <mosquitto.h>

#include "guardx_protocol.h"
#include "mqtt_bus.h"
#include "sensor_parser.h"

#define BROKER_HOST "127.0.0.1"
#define BROKER_PORT 1883
#define TOPIC_PREFIX "guardx/sensor/"

static struct mosquitto *client;
static mqtt_sensor_handler_t sensor_handler;

static void on_connect(struct mosquitto *mosq, void *userdata, int rc)
{
    int result;
    (void)userdata;
    if (rc != 0) {
        fprintf(stderr, "mqtt: broker connection failed (%d)\n", rc);
        return;
    }
    result = mosquitto_subscribe(mosq, NULL, "guardx/sensor/+",
                                 GUARDX_QOS_SENSOR);
    if (result != MOSQ_ERR_SUCCESS)
        fprintf(stderr, "mqtt: subscription failed (%d)\n", result);
}

static void on_message(struct mosquitto *mosq, void *userdata,
                       const struct mosquitto_message *message)
{
    const char *node;
    size_t length;
    char node_id[64];
    char json[GUARDX_JSON_MAX + 1];
    (void)mosq;
    (void)userdata;
    if (!message || !message->topic || !message->payload ||
        message->payloadlen <= 0 || message->payloadlen > GUARDX_JSON_MAX)
        return;
    if (strncmp(message->topic, TOPIC_PREFIX, sizeof(TOPIC_PREFIX) - 1) != 0)
        return;
    node = message->topic + sizeof(TOPIC_PREFIX) - 1;
    length = strlen(node);
    if (length == 0 || length >= sizeof(node_id) || strchr(node, '/'))
        return;
    memcpy(node_id, node, length + 1);
    memcpy(json, message->payload, (size_t)message->payloadlen);
    json[message->payloadlen] = '\0';
    if (sensor_handler)
        sensor_handler(node_id, json, message->payloadlen);
}

int mqtt_bus_init(mqtt_sensor_handler_t on_sensor)
{
    int rc;
    if (!on_sensor)
        return -1;
    sensor_handler = on_sensor;
    rc = mosquitto_lib_init();
    if (rc != MOSQ_ERR_SUCCESS)
        return -1;
    client = mosquitto_new("guardx-rpib-public-subset", true, NULL);
    if (!client)
        goto fail_library;
    mosquitto_connect_callback_set(client, on_connect);
    mosquitto_message_callback_set(client, on_message);
    rc = mosquitto_connect(client, BROKER_HOST, BROKER_PORT, 60);
    if (rc != MOSQ_ERR_SUCCESS)
        goto fail_client;
    rc = mosquitto_loop_start(client);
    if (rc != MOSQ_ERR_SUCCESS)
        goto fail_client;
    return 0;

fail_client:
    mosquitto_destroy(client);
    client = NULL;
fail_library:
    mosquitto_lib_cleanup();
    return -1;
}

int mqtt_bus_publish_actuator(const char *node_id, const char *json, int len)
{
    char topic[128];
    int size;
    if (!client || !node_id || !*node_id || strchr(node_id, '/') ||
        !json || len <= 0)
        return -1;
    size = snprintf(topic, sizeof(topic), GUARDX_TOPIC_ACTUATOR_FMT, node_id);
    if (size < 0 || (size_t)size >= sizeof(topic))
        return -1;
    return mosquitto_publish(client, NULL, topic, len, json,
                             GUARDX_QOS_ACTUATOR, false) == MOSQ_ERR_SUCCESS
               ? 0 : -1;
}

void mqtt_bus_cleanup(void)
{
    if (client) {
        mosquitto_disconnect(client);
        mosquitto_loop_stop(client, false);
        mosquitto_destroy(client);
        client = NULL;
    }
    mosquitto_lib_cleanup();
}
