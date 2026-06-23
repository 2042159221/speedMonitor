#include "Inf_ATGM336H.h"

#include <stdio.h>
#include <string.h>

// 常量定义（使用 float 字面量）
static const float PI = 3.14159265358979324f;
static const float A = 6378245.0f;              // 克拉索夫斯基椭球体长半轴
static const float EE = 0.00669342162296594323f; // 椭球体偏心率平方

#if defined(ATGM336H_USE_FAKE_DATA) && (ATGM336H_USE_FAKE_DATA != 0)
#error "ATGM336H_USE_FAKE_DATA is not allowed in production firmware"
#endif

//接收定位数据的缓冲
uint8_t loc_buff[LOC_BUFF_MAX_SIZE] = {0};
uint8_t loc_frame_buff[LOC_BUFF_MAX_SIZE] = {0};
//缓冲中数据的长度
volatile uint16_t loc_len = 0;

bool Inf_ATGM336H_TakeFrame(char *frame, uint16_t frame_size)
{
    uint16_t size;
    uint32_t primask;

    if (frame == NULL || frame_size == 0U) {
        return false;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    size = loc_len;
    if (size == 0U) {
        if (primask == 0U) {
            __enable_irq();
        }
        frame[0] = '\0';
        return false;
    }

    if (size >= frame_size) {
        size = frame_size - 1U;
    }
    memcpy(frame, loc_frame_buff, size);
    frame[size] = '\0';
    loc_len = 0U;
    if (primask == 0U) {
        __enable_irq();
    }

    return true;
}

void Inf_ATGM336H_Poll(void)
{
    (void)loc_len;
}

static void Inf_ATGM336H_ClearUartRxError(void)
{
    __HAL_UART_CLEAR_OREFLAG(&huart2);
    __HAL_UART_CLEAR_IDLEFLAG(&huart2);
    huart2.ErrorCode = HAL_UART_ERROR_NONE;
}

static void Inf_ATGM336H_StartReceive(void)
{
    HAL_StatusTypeDef status;

    Inf_ATGM336H_ClearUartRxError();
    status = HAL_UARTEx_ReceiveToIdle_IT(&huart2, loc_buff, LOC_BUFF_MAX_SIZE - 1);
    if (status != HAL_OK) {
        HAL_UART_AbortReceive(&huart2);
        Inf_ATGM336H_ClearUartRxError();
        status = HAL_UARTEx_ReceiveToIdle_IT(&huart2, loc_buff, LOC_BUFF_MAX_SIZE - 1);
    }
}

static bool Inf_ATGM336H_HasLocationSentence(const char *frame)
{
    return (strstr(frame, "GGA,") != NULL) || (strstr(frame, "RMC,") != NULL);
}

//串口中断回调，遇到空闲帧会进来
void Inf_ATGM336H_Callback(uint16_t size) {
    if (size >= LOC_BUFF_MAX_SIZE) {
        size = LOC_BUFF_MAX_SIZE - 1;
    }
    loc_buff[size] = '\0';

    if (Inf_ATGM336H_HasLocationSentence((const char *)loc_buff)) {
        memcpy(loc_frame_buff, loc_buff, size);
        loc_frame_buff[size] = '\0';
        loc_len = size;
    }

    //再次开始下一次接收
    Inf_ATGM336H_StartReceive();
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART2) {
        Inf_ATGM336H_Callback(Size);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        Inf_ATGM336H_StartReceive();
    }
}


/**
 * 发送指令
 */
static void Inf_ATGM336H_Send(uint8_t* cmd,uint8_t size){
    uint8_t crccode = 0;
    //计算校验码
    for(uint8_t i=0;i<size ; i++){
        crccode ^= cmd[i];
    }

    //拼接指令
    uint8_t datas[30] = {0};
    sprintf((char*)datas , "$%s*%X\r\n",cmd,crccode);


    //发送
    HAL_UART_Transmit(&huart2,datas,strlen((char*)datas),1000);
}

void Inf_ATGM336H_Init(void){
    loc_len = 0U;
    //开启串口接收
    Inf_ATGM336H_StartReceive();

    //指定GPS芯片模式:BD+GPS双模式
    Inf_ATGM336H_Send("PCAS04,3",8);
    //指定定位频率[1s一次定位数据]
    Inf_ATGM336H_Send("PCAS02,1000",11);

}

/**
 * 判断是否中国国境内
 */

 static bool out_of_china(float lat,float lon){
    if (lon < 72.004f || lon > 137.8347f) return true;
    if (lat < 0.8293f || lat > 55.8271f) return true;
    return false;
 }

 /**
 * 计算纬度偏移量
 */
static float transform_lat(float x, float y) {
    float ret = -100.0f + 2.0f * x + 3.0f * y + 0.2f * y * y + 0.1f * x * y + 0.2f * sqrtf(fabsf(x));
    ret += (20.0f * sinf(6.0f * x * PI) + 20.0f * sinf(2.0f * x * PI)) * 2.0f / 3.0f;
    ret += (20.0f * sinf(y * PI) + 40.0f * sinf(y / 3.0f * PI)) * 2.0f / 3.0f;
    ret += (160.0f * sinf(y / 12.0f * PI) + 320.0f * sinf(y * PI / 30.0f)) * 2.0f / 3.0f;
    return ret;
}

/**
 * 计算经度偏移量
 */
static float transform_lon(float x, float y) {
    float ret = 300.0f + x + 2.0f * y + 0.1f * x * x + 0.1f * x * y + 0.1f * sqrtf(fabsf(x));
    ret += (20.0f * sinf(6.0f * x * PI) + 20.0f * sinf(2.0f * x * PI)) * 2.0f / 3.0f;
    ret += (20.0f * sinf(x * PI) + 40.0f * sinf(x / 3.0f * PI)) * 2.0f / 3.0f;
    ret += (150.0f * sinf(x / 12.0f * PI) + 300.0f * sinf(x / 30.0f * PI)) * 2.0f / 3.0f;
    return ret;
}

/**
 * GPS坐标 (WGS-84) 转 GCJ-02 火星坐标
 * @param wgLat  输入的 WGS-84 纬度 (float)
 * @param wgLon  输入的 WGS-84 经度 (float)
 * @param mgLat  输出的 GCJ-02 纬度 (指针)
 * @param mgLon  输出的 GCJ-02 经度 (指针)
 */
void gps_to_gcj02(float wgLat, float wgLon, float *mgLat, float *mgLon) {
    // 如果在中国境外，直接返回原坐标
    if (out_of_china(wgLat, wgLon)) {
        *mgLat = wgLat;
        *mgLon = wgLon;
        return;
    }

    // 计算偏移量
    float dLat = transform_lat(wgLon - 105.0f, wgLat - 35.0f);
    float dLon = transform_lon(wgLon - 105.0f, wgLat - 35.0f);

    // 弧度转换
    float radLat = wgLat / 180.0f * PI;
    float magic = sinf(radLat);
    magic = 1.0f - EE * magic * magic;
    float sqrtMagic = sqrtf(magic);

    // 将偏移量从"度"转换为"经纬度差值"
    dLat = (dLat * 180.0f) / ((A * (1.0f - EE)) / (magic * sqrtMagic) * PI);
    dLon = (dLon * 180.0f) / (A / sqrtMagic * cosf(radLat) * PI);

    // 输出最终坐标
    *mgLat = wgLat + dLat;
    *mgLon = wgLon + dLon;
}
