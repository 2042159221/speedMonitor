#ifndef __INF_CST816D_H__
#define __INF_CST816D_H__
#include "i2c.h"
#include "stdbool.h"

//用户是否按下手指
bool Inf_CST816D_IsPressed(void);

//用户按下手指所在的角标
//12个有效位，使用uint16 存储
void Inf_CST816D_GetXY(uint16_t* x ,uint16_t * y);
#endif