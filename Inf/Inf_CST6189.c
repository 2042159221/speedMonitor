#include "Inf_CST6189.h"

//根据手册 i2c读写地址
#define DEV_ADDR_R 0x2B
#define DEV_ADDR_W 0x2A
#define TOUCH_HOR_RES 240U
#define TOUCH_VER_RES 320U
//是否按下手指
static bool is_pressed = false;

//用户点按触摸屏中断回调
void Inf_CST816_Callback(void){
    is_pressed = true;
}


//读取寄存器
//Param:地址，数据，长度
void Inf_CST816_ReadRegister(uint8_t reg ,uint8_t* datas, uint8_t len){
    HAL_I2C_Mem_Read(&hi2c2,DEV_ADDR_R,reg,I2C_MEMADD_SIZE_8BIT,datas,len,2000);

}

//用户是否按下
bool Inf_CST816D_IsPressed(void){
    if(is_pressed){
        is_pressed = false;
        return true;
    }
    return false;
}


//按下屏幕所在角标
void Inf_CST816D_GetXY(uint16_t* x, uint16_t* y){
    //读取角标
    uint8_t datas[4] = {0};
    uint16_t raw_x;
    uint16_t raw_y;
    Inf_CST816_ReadRegister(0x03,datas,4);

    //向左移动 8 位，腾出低端的 8 个空位
    raw_x = (uint16_t)(((datas[0] & 0xF) << 8 ) | datas[1]);
    //通过 & 0xF 可以将无用的高 4 位状态位清零，低 4 位才属于坐标
    raw_y = (uint16_t)(((datas[2] &0xF) << 8) | datas[3]);

    if (raw_x >= TOUCH_HOR_RES) {
        raw_x = TOUCH_HOR_RES - 1U;
    }
    if (raw_y >= TOUCH_VER_RES) {
        raw_y = TOUCH_VER_RES - 1U;
    }

    //因为屏幕x角标是反的，所以使用最大有效坐标 239 调换x角标
    *x = (uint16_t)((TOUCH_HOR_RES - 1U) - raw_x);
    *y = raw_y;

}
