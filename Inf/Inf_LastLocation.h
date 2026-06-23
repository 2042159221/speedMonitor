#ifndef __INF_LAST_LOCATION_H__
#define __INF_LAST_LOCATION_H__

#include "stdbool.h"

typedef struct {
    float lon;
    float lat;
    bool valid;
} Inf_LastLocation;

bool Inf_LastLocation_Load(Inf_LastLocation *location);
bool Inf_LastLocation_Save(float lon, float lat);
bool Inf_LastLocation_IsValid(float lon, float lat);

#endif
