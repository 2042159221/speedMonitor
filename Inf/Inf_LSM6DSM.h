#ifndef __INF_LSM6DSM_H__
#define __INF_LSM6DSM_H__

#include "FreeRTOS.h"
#include "i2c.h"
#include "lsm6dsm_reg.h"
#include "stdbool.h"
#include "stdint.h"
#include "task.h"

typedef struct {
    float acc_x;   // mg
    float acc_y;   // mg
    float acc_z;   // mg
    float gyro_x;  // mdps
    float gyro_y;  // mdps
    float gyro_z;  // mdps
} AccGyroStruct;

extern AccGyroStruct accgyro;

uint8_t Inf_LSM6DSM_ReadId(void);
bool Inf_LSM6DSM_Init(void);
bool Inf_LSM6DSM_IsReady(void);
bool Inf_LSM6DSM_GetAccGyro(void);

#endif
