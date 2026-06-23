#ifndef __INF_SLOPE_H__
#define __INF_SLOPE_H__

#include "Inf_LSM6DSM.h"
#include "stdbool.h"
#include "stdint.h"

typedef enum {
    INF_SLOPE_STATUS_CALIBRATING = 0,
    INF_SLOPE_STATUS_READY,
    INF_SLOPE_STATUS_ACCEL_INVALID,
    INF_SLOPE_STATUS_INPUT_INVALID,
} InfSlopeStatus;

typedef struct {
    float pitch_deg;
    float slope_percent;
    float acc_pitch_deg;
    float gyro_pitch_dps;
    float acc_norm_mg;
    uint32_t sample_count;
    uint32_t invalid_count;
    InfSlopeStatus status;
} InfSlopeState;

void Inf_Slope_Init(void);
bool Inf_Slope_Update(const AccGyroStruct *imu, uint32_t dt_ms);
float Inf_Slope_GetPercent(void);
float Inf_Slope_GetPitchDeg(void);
InfSlopeStatus Inf_Slope_GetStatus(void);
const InfSlopeState *Inf_Slope_GetState(void);
const char *Inf_Slope_StatusName(InfSlopeStatus status);

#endif
