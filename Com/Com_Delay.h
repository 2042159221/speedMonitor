#ifndef __COM_DELAY_H__
#define __COM_DELAY_H__

#include "stdint.h"
#include "main.h"
/// @file Com_Debug.h 当前这个模块主要封装一下延迟功能.利用HAL的延迟函数实现HAL_Delay()
// HAL_Delay,只能实现MS级别延迟,如果项目当中需要us级别的延迟那?只能自己封装

// 1.封装一个ms级别延迟函数
void Com_Delay_ms(uint32_t ms);

// 2.封装一个s级别延迟函数
void Com_Delay_s(uint32_t s);

#endif /* __COM_DELAY_H__ */
