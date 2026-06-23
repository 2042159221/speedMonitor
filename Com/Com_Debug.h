#ifndef __COM_DEBUG_H__
#define __COM_DEBUG_H__
#include "usart.h"
#include "stdio.h"
#include "string.h"

#define DEVELOPMENT
// 开发阶段
#ifdef DEVELOPMENT
// 获取当前文件名字,前面相对路径不需要
#define FILENAME strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__
// 不带换行功能
#define COM_DEBUG(format, ...) Com_Debug_Printf("[%s:%d]" format, FILENAME, __LINE__, ##__VA_ARGS__)
// 带换行功能
#define COM_DEBUG_LN(format, ...) Com_Debug_Printf("[%s:%d]" format "\r\n", FILENAME, __LINE__, ##__VA_ARGS__)
#else
// 生产阶段
// 不带换行功能
#define COM_DEBUG(format, ...) 
// 带换行功能
#define COM_DEBUG_LN(format, ...) 
#endif // DEBUG

// 重写fputc方法
void Com_Debug_Printf(const char *format, ...);
int fputc(int ch, FILE *File);

#endif /* __COM_DEBUG_H__ */

/**
 * @brief 串口调试功能,在开发项目时候,很多地方多频繁使用,因此放在通用层! 需要重写fputc方法
 *
 * 1.串口调试何时使用?
 * 项目开发:开发阶段,测试阶段,生产阶段(代码搞完了,硬件也没问题,产品开始售卖)!
 * 生产阶段测试代码不应该出现,影响到二进制文件打包体积,串口调试功能也会影响到代码时序问题.
 * 在生产阶段调试code就应该消失!
 *
 *
 * 在宏定义当中...,可以变参数
 * 函数当中需要使用#__VA_ARGS__表示可变参数,可变参数只能放在函数最后面
 * ##__VA_ARGS__,最前面的#表示含义,当可变参数没有情况下,把前面逗号干掉
 *
 */
