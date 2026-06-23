#include "Inf_Slope.h"

#include <math.h>
#include <stddef.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define INF_SLOPE_NOMINAL_DT_MS 20U
#define INF_SLOPE_MAX_DT_MS 200U
#define INF_SLOPE_ACC_NORM_MIN_MG 650.0f
#define INF_SLOPE_ACC_NORM_MAX_MG 1350.0f
#define INF_SLOPE_CALIBRATION_SAMPLES 25U
#define INF_SLOPE_CALIBRATION_MAX_GYRO_DPS 3.0f
#define INF_SLOPE_FILTER_TAU_SEC 0.75f
#define INF_SLOPE_MAX_ABS_PITCH_DEG 45.0f
#define INF_SLOPE_MAX_ABS_PERCENT 99.9f

#ifndef INF_SLOPE_ACC_FORWARD_SIGN
#define INF_SLOPE_ACC_FORWARD_SIGN 1.0f
#endif

#ifndef INF_SLOPE_GYRO_PITCH_SIGN
#define INF_SLOPE_GYRO_PITCH_SIGN 1.0f
#endif

static InfSlopeState slope_state;
static bool filter_ready = false;
static float zero_offset_deg = 0.0f;
static float calibration_sum_deg = 0.0f;
static uint32_t calibration_count = 0U;

static float clampf_local(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static float get_forward_mg(const AccGyroStruct *imu)
{
    return imu->acc_x * INF_SLOPE_ACC_FORWARD_SIGN;
}

static float get_lateral_mg(const AccGyroStruct *imu)
{
    return imu->acc_y;
}

static float get_up_mg(const AccGyroStruct *imu)
{
    return imu->acc_z;
}

static float get_pitch_rate_dps(const AccGyroStruct *imu)
{
    return (imu->gyro_y / 1000.0f) * INF_SLOPE_GYRO_PITCH_SIGN;
}

static float calc_acc_pitch_deg(float forward_mg, float lateral_mg, float up_mg)
{
    float vertical_plane_mg = sqrtf((lateral_mg * lateral_mg) + (up_mg * up_mg));

    if (vertical_plane_mg < 1.0f) {
        vertical_plane_mg = 1.0f;
    }

    return atan2f(forward_mg, vertical_plane_mg) * (180.0f / (float)M_PI);
}

static float calc_slope_percent(float pitch_deg)
{
    float radians = pitch_deg * ((float)M_PI / 180.0f);
    float percent = tanf(radians) * 100.0f;

    return clampf_local(percent, -INF_SLOPE_MAX_ABS_PERCENT, INF_SLOPE_MAX_ABS_PERCENT);
}

static uint32_t normalize_dt_ms(uint32_t dt_ms)
{
    if ((dt_ms == 0U) || (dt_ms > INF_SLOPE_MAX_DT_MS)) {
        return INF_SLOPE_NOMINAL_DT_MS;
    }

    return dt_ms;
}

void Inf_Slope_Init(void)
{
    slope_state.pitch_deg = 0.0f;
    slope_state.slope_percent = 0.0f;
    slope_state.acc_pitch_deg = 0.0f;
    slope_state.gyro_pitch_dps = 0.0f;
    slope_state.acc_norm_mg = 0.0f;
    slope_state.sample_count = 0U;
    slope_state.invalid_count = 0U;
    slope_state.status = INF_SLOPE_STATUS_CALIBRATING;

    filter_ready = false;
    zero_offset_deg = 0.0f;
    calibration_sum_deg = 0.0f;
    calibration_count = 0U;
}

bool Inf_Slope_Update(const AccGyroStruct *imu, uint32_t dt_ms)
{
    float forward_mg;
    float lateral_mg;
    float up_mg;
    float acc_norm_mg;
    float acc_pitch_deg;
    float pitch_rate_dps;
    float dt_sec;
    float gyro_pitch_deg;
    bool acc_valid;

    if (imu == NULL) {
        slope_state.status = INF_SLOPE_STATUS_INPUT_INVALID;
        slope_state.invalid_count++;
        return false;
    }

    dt_ms = normalize_dt_ms(dt_ms);
    dt_sec = (float)dt_ms / 1000.0f;

    forward_mg = get_forward_mg(imu);
    lateral_mg = get_lateral_mg(imu);
    up_mg = get_up_mg(imu);
    pitch_rate_dps = get_pitch_rate_dps(imu);
    acc_norm_mg = sqrtf((forward_mg * forward_mg) + (lateral_mg * lateral_mg) + (up_mg * up_mg));
    acc_pitch_deg = calc_acc_pitch_deg(forward_mg, lateral_mg, up_mg);
    acc_valid = ((acc_norm_mg >= INF_SLOPE_ACC_NORM_MIN_MG) &&
                 (acc_norm_mg <= INF_SLOPE_ACC_NORM_MAX_MG));

    slope_state.sample_count++;
    slope_state.acc_norm_mg = acc_norm_mg;
    slope_state.acc_pitch_deg = acc_pitch_deg;
    slope_state.gyro_pitch_dps = pitch_rate_dps;

    if (!acc_valid) {
        slope_state.invalid_count++;
        slope_state.status = INF_SLOPE_STATUS_ACCEL_INVALID;

        if (!filter_ready) {
            return false;
        }

        slope_state.pitch_deg = clampf_local(slope_state.pitch_deg + (pitch_rate_dps * dt_sec),
                                             -INF_SLOPE_MAX_ABS_PITCH_DEG,
                                             INF_SLOPE_MAX_ABS_PITCH_DEG);
        slope_state.slope_percent = calc_slope_percent(slope_state.pitch_deg);
        return true;
    }

    if (!filter_ready) {
        if (fabsf(pitch_rate_dps) <= INF_SLOPE_CALIBRATION_MAX_GYRO_DPS) {
            calibration_sum_deg += acc_pitch_deg;
            calibration_count++;
        }

        if (calibration_count < INF_SLOPE_CALIBRATION_SAMPLES) {
            slope_state.status = INF_SLOPE_STATUS_CALIBRATING;
            return false;
        }

        zero_offset_deg = calibration_sum_deg / (float)calibration_count;
        slope_state.pitch_deg = 0.0f;
        slope_state.slope_percent = 0.0f;
        filter_ready = true;
        slope_state.status = INF_SLOPE_STATUS_READY;
        return true;
    }

    gyro_pitch_deg = slope_state.pitch_deg + (pitch_rate_dps * dt_sec);
    {
        float alpha = INF_SLOPE_FILTER_TAU_SEC / (INF_SLOPE_FILTER_TAU_SEC + dt_sec);
        float corrected_acc_pitch_deg = acc_pitch_deg - zero_offset_deg;

        slope_state.pitch_deg = (alpha * gyro_pitch_deg) + ((1.0f - alpha) * corrected_acc_pitch_deg);
    }
    slope_state.pitch_deg = clampf_local(slope_state.pitch_deg,
                                         -INF_SLOPE_MAX_ABS_PITCH_DEG,
                                         INF_SLOPE_MAX_ABS_PITCH_DEG);
    slope_state.slope_percent = calc_slope_percent(slope_state.pitch_deg);
    slope_state.status = INF_SLOPE_STATUS_READY;

    return true;
}

float Inf_Slope_GetPercent(void)
{
    return slope_state.slope_percent;
}

float Inf_Slope_GetPitchDeg(void)
{
    return slope_state.pitch_deg;
}

InfSlopeStatus Inf_Slope_GetStatus(void)
{
    return slope_state.status;
}

const InfSlopeState *Inf_Slope_GetState(void)
{
    return &slope_state;
}

const char *Inf_Slope_StatusName(InfSlopeStatus status)
{
    switch (status) {
    case INF_SLOPE_STATUS_CALIBRATING:
        return "cal";
    case INF_SLOPE_STATUS_READY:
        return "ok";
    case INF_SLOPE_STATUS_ACCEL_INVALID:
        return "acc_bad";
    case INF_SLOPE_STATUS_INPUT_INVALID:
        return "input_bad";
    default:
        return "unknown";
    }
}
