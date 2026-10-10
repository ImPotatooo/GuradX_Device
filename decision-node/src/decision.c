/* GuardX RPi B fire-decision core. */
#include <string.h>

#include "decision.h"

void DECISION_ZONE_INIT(decision_zone_t *z)
{
    memset(z, 0, sizeof(*z));
    z->state = DECISION_STATE_NORMAL;
    z->last_cause = DECISION_CAUSE_NONE;
    z->last_score = -1.0f;
}

static fire_config_t cfg = {
    .gas_raw_min    = DEFAULT_GAS_RAW_MIN,    .gas_raw_max    = DEFAULT_GAS_RAW_MAX,
    .spark_raw_safe = DEFAULT_SPARK_RAW_SAFE, .spark_raw_danger = DEFAULT_SPARK_RAW_DANGER,
    .temp_min_c     = DEFAULT_TEMP_MIN_C,     .temp_max_c     = DEFAULT_TEMP_MAX_C,
    .humi_safe_percent   = DEFAULT_HUMI_SAFE_PERCENT,
    .humi_danger_percent = DEFAULT_HUMI_DANGER_PERCENT,
    .irtemp_min_c   = DEFAULT_IRTEMP_MIN_C,   .irtemp_max_c   = DEFAULT_IRTEMP_MAX_C,
    .weight_gas    = DEFAULT_WEIGHT_GAS,   .weight_spark  = DEFAULT_WEIGHT_SPARK,
    .weight_temp   = DEFAULT_WEIGHT_TEMP,  .weight_humi   = DEFAULT_WEIGHT_HUMI,
    .weight_irtemp = DEFAULT_WEIGHT_IRTEMP,
    .fire_score_threshold = DEFAULT_FIRE_SCORE_THRESHOLD,
    .n_confirm = DEFAULT_N_CONFIRM, .n_recover = DEFAULT_N_RECOVER,
    .min_valid_weight = DEFAULT_MIN_VALID_WEIGHT,
    .override_spark_score  = DEFAULT_OVERRIDE_SPARK_SCORE,
    .override_irtemp_score = DEFAULT_OVERRIDE_IRTEMP_SCORE,
};

#define FUSION_CHANNELS 5

enum {
    CH_GAS = 0,
    CH_SPARK,
    CH_TEMP,
    CH_HUMI,
    CH_IRTEMP,
};

static void ch_weights(float w[FUSION_CHANNELS])
{
    w[CH_GAS]    = cfg.weight_gas;
    w[CH_SPARK]  = cfg.weight_spark;
    w[CH_TEMP]   = cfg.weight_temp;
    w[CH_HUMI]   = cfg.weight_humi;
    w[CH_IRTEMP] = cfg.weight_irtemp;
}

static void ch_valid(const sensor_msg_t *msg, bool v[FUSION_CHANNELS])
{
    v[CH_GAS]    = msg->gas_valid;
    v[CH_SPARK]  = msg->spark_valid;
    v[CH_TEMP]   = msg->temphum_valid;
    v[CH_HUMI]   = msg->temphum_valid;
    v[CH_IRTEMP] = msg->irtemp_valid;
}

static const decision_cause_t CH_CAUSE[FUSION_CHANNELS] = {
    [CH_GAS]    = DECISION_CAUSE_GAS,
    [CH_SPARK]  = DECISION_CAUSE_SPARK,
    [CH_TEMP]   = DECISION_CAUSE_TEMP,
    [CH_HUMI]   = DECISION_CAUSE_HUMIDITY,
    [CH_IRTEMP] = DECISION_CAUSE_IRTEMP,
};

static float fuzzify_asc(double value, double min, double max)
{
    if (value <= min)
        return 0.0f;
    if (value >= max)
        return 100.0f;
    return (float)((value - min) / (max - min) * 100.0);
}

static float fuzzify_desc(double value, double safe, double danger)
{
    if (value >= safe)
        return 0.0f;
    if (value <= danger)
        return 100.0f;
    return (float)((safe - value) / (safe - danger) * 100.0);
}

static float composite_score(const sensor_msg_t *msg,
                             float out_scores[FUSION_CHANNELS])
{
    bool valid[FUSION_CHANNELS];
    float w[FUSION_CHANNELS];
    float weighted = 0.0f;
    float wsum = 0.0f;
    int i;

    ch_weights(w);
    ch_valid(msg, valid);

    out_scores[CH_GAS]    = fuzzify_asc(msg->gas_raw,
                                        cfg.gas_raw_min, cfg.gas_raw_max);
    out_scores[CH_SPARK]  = fuzzify_desc(msg->spark_raw,
                                         cfg.spark_raw_safe, cfg.spark_raw_danger);
    out_scores[CH_TEMP]   = fuzzify_asc(msg->temperature,
                                        cfg.temp_min_c, cfg.temp_max_c);
    out_scores[CH_HUMI]   = fuzzify_desc(msg->humidity,
                                         cfg.humi_safe_percent, cfg.humi_danger_percent);
    out_scores[CH_IRTEMP] = fuzzify_asc(msg->irtemp_object,
                                        cfg.irtemp_min_c, cfg.irtemp_max_c);

    for (i = 0; i < FUSION_CHANNELS; i++) {
        if (!valid[i]) {

            out_scores[i] = 0.0f;
            continue;
        }
        weighted += out_scores[i] * w[i];
        wsum     += w[i];
    }

    if (wsum < cfg.min_valid_weight)
        wsum = cfg.min_valid_weight;

    return weighted / wsum;
}

static bool cross_confirm(const float scores[FUSION_CHANNELS])
{
    return scores[CH_SPARK]  >= cfg.override_spark_score &&
           scores[CH_IRTEMP] >= cfg.override_irtemp_score;
}

static bool fire_condition(decision_zone_t *z, const sensor_msg_t *msg,
                           float out_scores[FUSION_CHANNELS])
{
    float total = composite_score(msg, out_scores);

    z->last_score = total;
    return cross_confirm(out_scores) || total >= cfg.fire_score_threshold;
}

static decision_cause_t dominant_cause(const float scores[FUSION_CHANNELS])
{
    float w[FUSION_CHANNELS];
    int best = 0;
    int i;

    ch_weights(w);
    for (i = 1; i < FUSION_CHANNELS; i++) {
        if (scores[i] * w[i] > scores[best] * w[best])
            best = i;
    }
    return CH_CAUSE[best];
}

static decision_event_t feed_normal(decision_zone_t *z, const sensor_msg_t *msg)
{
    float scores[FUSION_CHANNELS];

    z->score_hits = fire_condition(z, msg, scores) ? (z->score_hits + 1) : 0;
    if (z->score_hits < cfg.n_confirm)
        return DECISION_EVENT_NONE;

    z->last_cause = dominant_cause(scores);
    z->state = DECISION_STATE_FIRE;
    z->last_trigger_seq = msg->seq;
    z->score_hits = 0;
    z->recover_hits = 0;
    z->freeze_cycles = 0;
    z->relaxed = false;
    return DECISION_EVENT_FIRE;
}

static decision_event_t feed_fire(decision_zone_t *z, const sensor_msg_t *msg)
{
    float scores[FUSION_CHANNELS];
    bool  valid[FUSION_CHANNELS];
    float w[FUSION_CHANNELS];
    float surviving = 0.0f;
    bool  all_valid = true;
    int   i;

    ch_valid(msg, valid);
    ch_weights(w);
    for (i = 0; i < FUSION_CHANNELS; i++) {
        if (valid[i])
            surviving += w[i];
        else
            all_valid = false;
    }

    if (all_valid) {
        z->freeze_cycles = 0;
        z->relaxed = false;
    } else {
        z->freeze_cycles++;

        if (z->freeze_cycles < cfg.freeze_relax_cycles ||
            surviving < cfg.min_valid_weight)
            return DECISION_EVENT_NONE;

        if (!z->relaxed) {

            z->relaxed = true;
            z->recover_hits = 0;
        }
    }

    z->recover_hits = fire_condition(z, msg, scores) ? 0 : (z->recover_hits + 1);
    if (z->recover_hits > cfg.n_recover)
        z->recover_hits = cfg.n_recover;

    return DECISION_EVENT_NONE;
}

decision_event_t DECISION_FORCE_RECOVER(decision_zone_t *z)
{
    if (z == NULL || z->state != DECISION_STATE_FIRE)
        return DECISION_EVENT_NONE;

    z->state = DECISION_STATE_NORMAL;
    z->recover_hits = 0;
    z->freeze_cycles = 0;
    z->relaxed = false;
    return DECISION_EVENT_RECOVER;
}

bool DECISION_RECOVER_READY(const decision_zone_t *z)
{
    return z != NULL && z->state == DECISION_STATE_FIRE &&
           z->recover_hits >= cfg.n_recover;
}

decision_event_t DECISION_FEED(decision_zone_t *z, const sensor_msg_t *msg)
{
    if (z == NULL || msg == NULL)
        return DECISION_EVENT_NONE;

    z->last_score = -1.0f;
    return (z->state == DECISION_STATE_NORMAL) ? feed_normal(z, msg)
                                               : feed_fire(z, msg);
}

decision_cause_t DECISION_LAST_CAUSE(const decision_zone_t *z)
{
    return z->last_cause;
}

uint32_t DECISION_LAST_TRIGGER_SEQ(const decision_zone_t *z)
{
    return z->last_trigger_seq;
}

const char *DECISION_CAUSE_STR(decision_cause_t c)
{
    switch (c) {
    case DECISION_CAUSE_GAS:      return "gas";
    case DECISION_CAUSE_TEMP:     return "temp";
    case DECISION_CAUSE_SPARK:    return "spark";
    case DECISION_CAUSE_HUMIDITY: return "humidity";
    case DECISION_CAUSE_IRTEMP:   return "irtemp";
    default:                      return "none";
    }
}

decision_state_t DECISION_STATE(const decision_zone_t *z)
{
    return z->state;
}

int DECISION_FREEZE_CYCLES(const decision_zone_t *z)
{
    return z->freeze_cycles;
}

float DECISION_LAST_SCORE(const decision_zone_t *z)
{
    return z->last_score;
}

void DECISION_SET_CONFIG(const fire_config_t *new_cfg)
{
    if (new_cfg)
        cfg = *new_cfg;
}

const fire_config_t *DECISION_GET_CONFIG(void)
{
    return &cfg;
}
