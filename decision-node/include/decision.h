/* GuardX RPi B fire-decision core. */
#ifndef DECISION_H
#define DECISION_H

#include <stdbool.h>
#include "sensor_parser.h"

typedef struct {
    float gas_raw_min,    gas_raw_max;
    float spark_raw_safe, spark_raw_danger;

    float temp_min_c,     temp_max_c;
    float humi_safe_percent, humi_danger_percent;
    float irtemp_min_c,   irtemp_max_c;

    float weight_gas, weight_spark, weight_temp, weight_humi, weight_irtemp;

    float fire_score_threshold;
    int   n_confirm;
    int   n_recover;
    int   freeze_relax_cycles;

    float min_valid_weight;
    float override_spark_score;
    float override_irtemp_score;
} fire_config_t;

#define DEFAULT_GAS_RAW_MIN          650.0f
#define DEFAULT_GAS_RAW_MAX          800.0f

#define DEFAULT_SPARK_RAW_SAFE       850.0f
#define DEFAULT_SPARK_RAW_DANGER     30.0f
#define DEFAULT_TEMP_MIN_C           35.0f
#define DEFAULT_TEMP_MAX_C           60.0f
#define DEFAULT_HUMI_SAFE_PERCENT    50.0f
#define DEFAULT_HUMI_DANGER_PERCENT  15.0f
#define DEFAULT_IRTEMP_MIN_C         40.0f
#define DEFAULT_IRTEMP_MAX_C         80.0f

#define DEFAULT_WEIGHT_SPARK   0.35f
#define DEFAULT_WEIGHT_IRTEMP  0.25f
#define DEFAULT_WEIGHT_GAS     0.20f
#define DEFAULT_WEIGHT_TEMP    0.15f
#define DEFAULT_WEIGHT_HUMI    0.05f

#define DEFAULT_FIRE_SCORE_THRESHOLD  65.0f
#define DEFAULT_N_CONFIRM             3
#define DEFAULT_N_RECOVER             10

#define DEFAULT_FREEZE_RELAX_CYCLES   60

#define DEFAULT_MIN_VALID_WEIGHT      0.50f

#define DEFAULT_OVERRIDE_SPARK_SCORE   70.0f
#define DEFAULT_OVERRIDE_IRTEMP_SCORE  70.0f

typedef enum {
    DECISION_STATE_NORMAL = 0,
    DECISION_STATE_FIRE   = 1,
} decision_state_t;

typedef enum {
    DECISION_CAUSE_NONE     = 0,
    DECISION_CAUSE_GAS      = 1,
    DECISION_CAUSE_TEMP     = 2,
    DECISION_CAUSE_SPARK    = 3,
    DECISION_CAUSE_HUMIDITY = 4,
    DECISION_CAUSE_IRTEMP   = 5,
} decision_cause_t;

typedef struct {
    decision_state_t state;
    decision_cause_t last_cause;
    uint32_t         last_trigger_seq;

    int   score_hits;
    int   recover_hits;
    int   freeze_cycles;
    bool  relaxed;
    float last_score;
} decision_zone_t;

void DECISION_ZONE_INIT(decision_zone_t *z);

typedef enum {
    DECISION_EVENT_NONE    = 0,
    DECISION_EVENT_FIRE    = 1,
    DECISION_EVENT_RECOVER = 2,
} decision_event_t;

decision_event_t DECISION_FEED(decision_zone_t *z, const sensor_msg_t *msg);

decision_event_t DECISION_FORCE_RECOVER(decision_zone_t *z);

bool DECISION_RECOVER_READY(const decision_zone_t *z);

decision_cause_t DECISION_LAST_CAUSE(const decision_zone_t *z);
uint32_t         DECISION_LAST_TRIGGER_SEQ(const decision_zone_t *z);
const char      *DECISION_CAUSE_STR(decision_cause_t c);

decision_state_t DECISION_STATE(const decision_zone_t *z);

int              DECISION_FREEZE_CYCLES(const decision_zone_t *z);

float            DECISION_LAST_SCORE(const decision_zone_t *z);

void                    DECISION_SET_CONFIG(const fire_config_t *cfg);
const fire_config_t   *DECISION_GET_CONFIG(void);

#endif
