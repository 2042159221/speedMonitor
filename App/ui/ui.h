#ifndef __UI_H__
#define __UI_H__
#include <stdint.h>
#include "ui_common.h"
#include "ui_settings.h"
#include "ui_general.h"
#include "ui_roadmap.h"
void ui_create(void);
void ui_setInitialLocation(float lon, float lat);
void ui_updateBattery(bool isCharge, float v);
void ui_updatePower(bool isCharge, float v, uint8_t socPercent, bool isLow, bool isCritical);
void ui_showPowerMessage(const char *message);
void ui_upateBikeState(float speed);
#endif
