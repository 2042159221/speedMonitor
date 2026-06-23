
#ifndef __UI_COMMON_H__
#define __UI_COMMON_H__
#include "lvgl.h"
#include <stdio.h>
#include <math.h>

//卫星个数观察者
extern lv_subject_t saleNumSubject;
//速度
extern lv_subject_t speedSubject;
//骑行里程
extern lv_subject_t distanceSubject;
//坡度
extern lv_subject_t slopeSubject;
//经度
extern lv_subject_t lonSubject;
//维度
extern lv_subject_t latSubject;
//骑行时间
extern lv_subject_t timeSubject;
//缩放等级
extern lv_subject_t zoomSubject;
//样式观察者
extern lv_subject_t styleSubject;

//深色样式
extern lv_style_t darkStyle;
//白色背景浅色样式
extern lv_style_t whiteBgStyle;
//灰色背景浅色样式
extern lv_style_t  greyBgStyle;

extern const char * languages[];
extern const char * tags[];
extern const char * translations[];

#define TILE_SIZE 256

void ui_common_translation(lv_obj_t *label, const char *tag);
void latLngToWorldPixel(float lon, float lat, int zoom, int*x, int*y);
void convert_world_pixels(int64_t old_x , int64_t old_y , int old_zoom, int new_zoom,int64_t * new_x ,int64_t * new_y);
#endif
