#ifndef __INF_ATGM336H_H__
#define __INF_ATGM336H_H__
#include "usart.h"
#include "Com_Debug.h"
#include "math.h"
#include "stdbool.h"

//初始化 ATGM
void Inf_ATGM336H_Init(void);
void Inf_ATGM336H_Poll(void);
bool Inf_ATGM336H_TakeFrame(char *frame, uint16_t frame_size);
void Inf_ATGM336H_Callback(uint16_t size);

void gps_to_gcj02(float wgLat ,float wgLon,float *mgLat, float *mgLon);

#define LOC_BUFF_MAX_SIZE 1024
//接收定位数据的缓冲
extern uint8_t loc_buff[LOC_BUFF_MAX_SIZE];
extern uint8_t loc_frame_buff[LOC_BUFF_MAX_SIZE];

//缓冲中数据的长度
extern volatile uint16_t loc_len;

#endif
