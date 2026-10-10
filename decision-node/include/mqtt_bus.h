#ifndef MQTT_BUS_H
#define MQTT_BUS_H

/* This public adapter connects only to a broker on the local loopback. */
typedef void (*mqtt_sensor_handler_t)(const char *node_id,
                                      const char *json, int len);

int mqtt_bus_init(mqtt_sensor_handler_t on_sensor);
int mqtt_bus_publish_actuator(const char *node_id, const char *json, int len);
void mqtt_bus_cleanup(void);

#endif
