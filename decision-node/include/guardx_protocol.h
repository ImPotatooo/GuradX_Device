#ifndef GUARDX_PROTOCOL_H
#define GUARDX_PROTOCOL_H

/* Public subset of the RPi A -> B -> C message contract. */
#define GUARDX_NODE_RPIB             "rpib"
#define GUARDX_TOPIC_SENSOR_FMT      "guardx/sensor/%s"
#define GUARDX_TOPIC_ACTUATOR_FMT    "guardx/actuator/%s"
#define GUARDX_QOS_SENSOR            0
#define GUARDX_QOS_ACTUATOR          1
#define GUARDX_JSON_KEY_SEQ          "seq"
#define GUARDX_ACTION_ON             "ON"
#define GUARDX_ACTION_SET            "SET"
#define GUARDX_ACTION_CLOSE          "CLOSE"
#define GUARDX_CMD_WATER_PUMP        "water_pump"
#define GUARDX_CMD_SERVO_1           "servo_1"
#define GUARDX_CMD_SHUTTER           "shutter"

#endif
