#include "Inf_LSM6DSM.h"

#define LSM6DSM_I2C_ADDR_W 0xD4U
#define LSM6DSM_I2C_ADDR_R 0xD5U
#define LSM6DSM_I2C_TIMEOUT_MS 1000U
#define LSM6DSM_RESET_TIMEOUT_MS 100U

AccGyroStruct accgyro;

static bool lsm6dsm_ready = false;

static int32_t write_reg(void *handle, uint8_t reg, const uint8_t *buffer, uint16_t len)
{
    return (int32_t)HAL_I2C_Mem_Write((I2C_HandleTypeDef *)handle,
                                      LSM6DSM_I2C_ADDR_W,
                                      reg,
                                      I2C_MEMADD_SIZE_8BIT,
                                      (uint8_t *)buffer,
                                      len,
                                      LSM6DSM_I2C_TIMEOUT_MS);
}

static int32_t read_reg(void *handle, uint8_t reg, uint8_t *buffer, uint16_t len)
{
    return (int32_t)HAL_I2C_Mem_Read((I2C_HandleTypeDef *)handle,
                                     LSM6DSM_I2C_ADDR_R,
                                     reg,
                                     I2C_MEMADD_SIZE_8BIT,
                                     buffer,
                                     len,
                                     LSM6DSM_I2C_TIMEOUT_MS);
}

static void platform_delay(uint32_t millisec)
{
    vTaskDelay(pdMS_TO_TICKS(millisec));
}

static stmdev_ctx_t ctx = {
    .write_reg = write_reg,
    .read_reg = read_reg,
    .handle = &hi2c1,
    .mdelay = platform_delay,
};

uint8_t Inf_LSM6DSM_ReadId(void)
{
    uint8_t id = 0U;

    if (lsm6dsm_device_id_get(&ctx, &id) != 0) {
        return 0U;
    }

    return id;
}

bool Inf_LSM6DSM_Init(void)
{
    uint8_t id = Inf_LSM6DSM_ReadId();
    uint8_t is_resetting = 0U;
    TickType_t start_tick = xTaskGetTickCount();

    lsm6dsm_ready = false;

    if (id != LSM6DSM_ID) {
        return false;
    }

    if (lsm6dsm_reset_set(&ctx, 1U) != 0) {
        return false;
    }

    do {
        if (lsm6dsm_reset_get(&ctx, &is_resetting) != 0) {
            return false;
        }
        platform_delay(1U);
    } while ((is_resetting != 0U) &&
             ((xTaskGetTickCount() - start_tick) < pdMS_TO_TICKS(LSM6DSM_RESET_TIMEOUT_MS)));

    if (is_resetting != 0U) {
        return false;
    }

    if (lsm6dsm_block_data_update_set(&ctx, 1U) != 0) {
        return false;
    }
    if (lsm6dsm_gy_full_scale_set(&ctx, LSM6DSM_2000dps) != 0) {
        return false;
    }
    if (lsm6dsm_xl_full_scale_set(&ctx, LSM6DSM_8g) != 0) {
        return false;
    }
    if (lsm6dsm_gy_data_rate_set(&ctx, LSM6DSM_GY_ODR_52Hz) != 0) {
        return false;
    }
    if (lsm6dsm_xl_data_rate_set(&ctx, LSM6DSM_XL_ODR_52Hz) != 0) {
        return false;
    }

    lsm6dsm_ready = true;
    return true;
}

bool Inf_LSM6DSM_IsReady(void)
{
    return lsm6dsm_ready;
}

bool Inf_LSM6DSM_GetAccGyro(void)
{
    int16_t raw[3] = {0};

    if (!lsm6dsm_ready) {
        return false;
    }

    if (lsm6dsm_angular_rate_raw_get(&ctx, raw) != 0) {
        return false;
    }
    accgyro.gyro_x = lsm6dsm_from_fs2000dps_to_mdps(raw[0]);
    accgyro.gyro_y = lsm6dsm_from_fs2000dps_to_mdps(raw[1]);
    accgyro.gyro_z = lsm6dsm_from_fs2000dps_to_mdps(raw[2]);

    if (lsm6dsm_acceleration_raw_get(&ctx, raw) != 0) {
        return false;
    }
    accgyro.acc_x = lsm6dsm_from_fs8g_to_mg(raw[0]);
    accgyro.acc_y = lsm6dsm_from_fs8g_to_mg(raw[1]);
    accgyro.acc_z = lsm6dsm_from_fs8g_to_mg(raw[2]);

    return true;
}
