#include "Inf_power.h"
#include "Com_Debug.h"

#define POWER_KEY_DEBOUNCE_MS              10U
#define POWER_KEY_BOOT_RELEASE_STABLE_MS  500U
#define POWER_KEY_SHUTDOWN_HOLD_MS       2000U
#define POWER_KEY_POLL_MS                  10U

#define POWER_ADC_REF_MV                 3300U
#define POWER_ADC_MAX                    4095U
#define POWER_BAT_DIVIDER_NUM               2U
#define POWER_BAT_DIVIDER_DEN               1U
#define POWER_ADC_POLL_TIMEOUT_MS           2U
#define POWER_FILTER_SHIFT                  3U

#define POWER_SOC_FULL_MV                4100U
#define POWER_SOC_75_MV                  3950U
#define POWER_SOC_50_MV                  3750U
#define POWER_SOC_25_MV                  3550U
#define POWER_SOC_EMPTY_MV               3350U
#define POWER_LOW_ASSERT_MV              3450U
#define POWER_LOW_RELEASE_MV             3550U
#define POWER_CRITICAL_ASSERT_MV         3350U
#define POWER_CRITICAL_RELEASE_MV        3450U

static bool powerKeyShutdownArmed = false;
static bool powerShutdownRequested = false;
static uint32_t powerKeyHoldMs = 0U;
static uint32_t powerFilteredMvX8 = 0U;
static bool powerFilterReady = false;
static bool powerLowLatched = false;
static bool powerCriticalLatched = false;

static Inf_Power_BatteryState powerBattery = {0};

static bool Inf_Power_KeyIsLow(void)
{
    return HAL_GPIO_ReadPin(PWR_KEY_DET_GPIO_Port, PWR_KEY_DET_Pin) == GPIO_PIN_RESET;
}

static bool Inf_Power_ReadRawAdc(uint16_t *rawAdc)
{
    if (HAL_ADC_PollForConversion(&hadc1, POWER_ADC_POLL_TIMEOUT_MS) != HAL_OK)
    {
        return false;
    }

    *rawAdc = (uint16_t)HAL_ADC_GetValue(&hadc1);
    return true;
}

static uint16_t Inf_Power_RawToBatteryMv(uint16_t rawAdc)
{
    uint32_t adcMv = ((uint32_t)rawAdc * POWER_ADC_REF_MV + (POWER_ADC_MAX / 2U)) / POWER_ADC_MAX;
    uint32_t batteryMv = (adcMv * POWER_BAT_DIVIDER_NUM) / POWER_BAT_DIVIDER_DEN;

    if (batteryMv > 65535U)
    {
        batteryMv = 65535U;
    }

    return (uint16_t)batteryMv;
}

static uint16_t Inf_Power_FilterMv(uint16_t sampleMv)
{
    if (!powerFilterReady)
    {
        powerFilteredMvX8 = ((uint32_t)sampleMv) << POWER_FILTER_SHIFT;
        powerFilterReady = true;
    }
    else
    {
        powerFilteredMvX8 = powerFilteredMvX8 - (powerFilteredMvX8 >> POWER_FILTER_SHIFT) + sampleMv;
    }

    return (uint16_t)(powerFilteredMvX8 >> POWER_FILTER_SHIFT);
}

static uint8_t Inf_Power_SocFromMv(uint16_t mv)
{
    if (mv >= POWER_SOC_FULL_MV)
    {
        return 100U;
    }
    if (mv >= POWER_SOC_75_MV)
    {
        return (uint8_t)(75U + (((uint32_t)(mv - POWER_SOC_75_MV) * 25U) /
                               (POWER_SOC_FULL_MV - POWER_SOC_75_MV)));
    }
    if (mv >= POWER_SOC_50_MV)
    {
        return (uint8_t)(50U + (((uint32_t)(mv - POWER_SOC_50_MV) * 25U) /
                               (POWER_SOC_75_MV - POWER_SOC_50_MV)));
    }
    if (mv >= POWER_SOC_25_MV)
    {
        return (uint8_t)(25U + (((uint32_t)(mv - POWER_SOC_25_MV) * 25U) /
                               (POWER_SOC_50_MV - POWER_SOC_25_MV)));
    }
    if (mv >= POWER_SOC_EMPTY_MV)
    {
        return (uint8_t)(((uint32_t)(mv - POWER_SOC_EMPTY_MV) * 25U) /
                         (POWER_SOC_25_MV - POWER_SOC_EMPTY_MV));
    }
    return 0U;
}

static bool Inf_Power_UpdateLatch(bool latched, uint16_t mv, uint16_t assertMv, uint16_t releaseMv)
{
    if (latched)
    {
        return mv < releaseMv;
    }
    return mv <= assertMv;
}

void Inf_Power_Init(void){
    HAL_GPIO_WritePin(PWR_EN_GPIO_Port, PWR_EN_Pin, GPIO_PIN_SET);

    //启动ADC
    HAL_ADC_Start(&hadc1);

    COM_DEBUG_LN("power init: PWR_EN=1 key=%d charge=%d",
                 HAL_GPIO_ReadPin(PWR_KEY_DET_GPIO_Port, PWR_KEY_DET_Pin),
                 HAL_GPIO_ReadPin(BAT_CHG_DET_GPIO_Port, BAT_CHG_DET_Pin));

    if (Inf_Power_KeyIsLow())
    {
        COM_DEBUG_LN("power key held during boot, wait release before arming shutdown");
        while (Inf_Power_KeyIsLow())
        {
            vTaskDelay(POWER_KEY_POLL_MS);
        }
    }

    vTaskDelay(POWER_KEY_BOOT_RELEASE_STABLE_MS);
    powerKeyShutdownArmed = true;
    COM_DEBUG_LN("power key shutdown armed");
}

void Inf_Power_Update(void)
{
    uint16_t rawAdc = 0U;
    uint16_t sampleMv;
    uint16_t filteredMv;

    powerBattery.isCharging = Inf_Power_IsCharge();

    if (!Inf_Power_ReadRawAdc(&rawAdc))
    {
        return;
    }

    sampleMv = Inf_Power_RawToBatteryMv(rawAdc);
    filteredMv = Inf_Power_FilterMv(sampleMv);

    powerLowLatched = Inf_Power_UpdateLatch(powerLowLatched,
                                            filteredMv,
                                            POWER_LOW_ASSERT_MV,
                                            POWER_LOW_RELEASE_MV);
    powerCriticalLatched = Inf_Power_UpdateLatch(powerCriticalLatched,
                                                 filteredMv,
                                                 POWER_CRITICAL_ASSERT_MV,
                                                 POWER_CRITICAL_RELEASE_MV);

    powerBattery.rawAdc = rawAdc;
    powerBattery.voltageMv = filteredMv;
    powerBattery.socPercent = Inf_Power_SocFromMv(filteredMv);
    powerBattery.isLow = powerLowLatched;
    powerBattery.isCritical = powerCriticalLatched;
    powerBattery.isValid = true;
}

const Inf_Power_BatteryState *Inf_Power_GetBatteryState(void)
{
    return &powerBattery;
}

void Inf_Power_KeyScan(uint32_t elapsedMs)
{
    if (!powerKeyShutdownArmed || powerShutdownRequested)
    {
        return;
    }

    if (Inf_Power_KeyIsLow())
    {
        if (powerKeyHoldMs == 0U)
        {
            COM_DEBUG_LN("power key press detected");
        }

        if (powerKeyHoldMs < (0xFFFFFFFFU - elapsedMs))
        {
            powerKeyHoldMs += elapsedMs;
        }

        if (powerKeyHoldMs >= POWER_KEY_SHUTDOWN_HOLD_MS)
        {
            powerShutdownRequested = true;
            COM_DEBUG_LN("power key long press shutdown request");
        }
    }
    else
    {
        powerKeyHoldMs = 0U;
    }
}

bool Inf_Power_ShutdownRequested(void)
{
    return powerShutdownRequested;
}

void Inf_Power_ClearShutdownRequest(void)
{
    powerShutdownRequested = false;
    powerKeyHoldMs = 0U;
}

//获取ADC测量的电压值
float Inf_Power_GetV(void){
    return ((float)powerBattery.voltageMv) / 1000.0f;
}

uint16_t Inf_Power_GetVoltageMv(void)
{
    return powerBattery.voltageMv;
}

//电池是否充电中
bool Inf_Power_IsCharge(void){
    return HAL_GPIO_ReadPin(BAT_CHG_DET_GPIO_Port,BAT_CHG_DET_Pin) ==GPIO_PIN_RESET;

}


//电源按键是否按下 
bool Inf_Power_PowerKeyIsPressed(void){
    if (Inf_Power_KeyIsLow())
    {
        vTaskDelay(POWER_KEY_DEBOUNCE_MS);
        if (!Inf_Power_KeyIsLow())
        {
            return false;
        }
    }
    return Inf_Power_ShutdownRequested();
}

//关机
void Inf_Power_PowerOFF(void){
    COM_DEBUG_LN("power off: PWR_EN=0");
    HAL_GPIO_WritePin(PWR_EN_GPIO_Port,PWR_EN_Pin,GPIO_PIN_RESET);
}
