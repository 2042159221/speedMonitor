#include "Com_Delay.h"

// 1.封装一个ms级别延迟函数
void Com_Delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

// 2.封装一个s级别延迟函数
void Com_Delay_s(uint32_t s)
{
    while (s--)
    {
        Com_Delay_ms(1000);
    }
}
