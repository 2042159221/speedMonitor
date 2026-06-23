#ifndef __INF_PWR_H__
#define __INF_PWR_H__
#include "gpio.h"
#include "stdbool.h"
#include "stdint.h"
#include "FreeRTOS.h"
#include "task.h"
#include "adc.h"

typedef struct {
    uint16_t rawAdc;
    uint16_t voltageMv;
    uint8_t socPercent;
    bool isCharging;
    bool isLow;
    bool isCritical;
    bool isValid;
} Inf_Power_BatteryState;

//初始化
void Inf_Power_Init(void);

//周期更新电池采样和充电状态
void Inf_Power_Update(void);

//获取电池状态快照
const Inf_Power_BatteryState *Inf_Power_GetBatteryState(void);

//周期扫描电源按键
void Inf_Power_KeyScan(uint32_t elapsedMs);

//是否已经请求关机
bool Inf_Power_ShutdownRequested(void);

//清除关机请求
void Inf_Power_ClearShutdownRequest(void);

//电源按键是否按下
bool Inf_Power_PowerKeyIsPressed(void);

//关机
void Inf_Power_PowerOFF(void);

//获取adc 测量电压值
float Inf_Power_GetV(void);

//获取adc 测量电压值，单位mV
uint16_t Inf_Power_GetVoltageMv(void);

//电池是否充电中
bool Inf_Power_IsCharge(void);
#endif
