#include "Com_Debug.h"

#include "stdarg.h"

#define COM_DEBUG_BUFFER_SIZE 160U

void Com_Debug_Printf(const char *format, ...)
{
    char buffer[COM_DEBUG_BUFFER_SIZE];
    va_list args;
    int len;
    uint16_t tx_len;

    va_start(args, format);
    len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len <= 0)
    {
        return;
    }

    tx_len = (len < (int)sizeof(buffer)) ? (uint16_t)len : (uint16_t)(sizeof(buffer) - 1U);
    HAL_UART_Transmit(&huart1, (uint8_t *)buffer, tx_len, 1000);
}

int fputc(int ch, FILE *File)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 1000);
    return ch;
}


