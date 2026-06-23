#include "ui_common.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

//卫星个数观察者
lv_subject_t saleNumSubject;
//速度
lv_subject_t speedSubject;
//骑行里程
lv_subject_t distanceSubject;
//坡度
lv_subject_t slopeSubject;
//经度
lv_subject_t lonSubject;
//维度
lv_subject_t latSubject;
//骑行时间
lv_subject_t timeSubject;
//缩放等级
lv_subject_t zoomSubject;
//样式观察者
lv_subject_t styleSubject;

//深色样式
lv_style_t darkStyle;
//白色背景浅色样式
lv_style_t whiteBgStyle;
//灰色背景浅色样式
lv_style_t  greyBgStyle;

#define UI_TEXT_ZH_STOPPED  "\xE9\x9D\x99\xE6\xAD\xA2\xE4\xB8\xAD"
#define UI_TEXT_ZH_RUNNING  "\xE8\xBF\x90\xE5\x8A\xA8\xE4\xB8\xAD"
#define UI_TEXT_ZH_LANGUAGE "\xE8\xAF\xAD\xE8\xA8\x80"
#define UI_TEXT_ZH_DARKMODE "\xE6\xB7\xB1\xE8\x89\xB2\xE6\xA8\xA1\xE5\xBC\x8F"
#define UI_TEXT_ZH_ROADMAP  "\xE8\xB7\xAF\xE4\xB9\xA6"
#define UI_TEXT_ZH_GENERAL  "\xE9\x80\x9A\xE7\x94\xA8"
#define UI_TEXT_ZH_SETTINGS "\xE8\xAE\xBE\xE7\xBD\xAE"

const char * languages[] = {"English", "Chinese",  NULL};
const char * tags[] = {"Stopped", "Running", "Language", "DarkMode","Roadmap","General","Settings",  NULL};
const char * translations[] = {
    "Stopped", UI_TEXT_ZH_STOPPED,
    "Running", UI_TEXT_ZH_RUNNING,
    "Language", UI_TEXT_ZH_LANGUAGE,
    "Dark Mode", UI_TEXT_ZH_DARKMODE,
    "Roadmap", UI_TEXT_ZH_ROADMAP,
    "General", UI_TEXT_ZH_GENERAL,
    "Settings", UI_TEXT_ZH_SETTINGS
};

/**
 * @brief 给标签添加语言改变事件
 *
 * @param label
 */
void ui_common_translation(lv_obj_t* label, const char* tag){
    lv_label_set_translation_tag(label, tag);
}

//经纬度转换世界像素角标（墨卡托公式）
void latLngToWorldPixel(float lon, float lat, int zoom, int*x, int*y){
    //计算当前缩放级别下的地图总像素的边长 (256*2^zoom)
    float mapSize = TILE_SIZE * (float)(1 << zoom);

    //经度转 X 像素
    *x = (int)((lon + 180.0f) / 360.0f * mapSize);

    //纬度转Y 像素（Web墨卡托投影）
    if(lat > 85.05112878f) lat = 85.05112878f;
    if(lat < -85.05112878f) lat = -85.05112878f;

    //角度转弧度
    float latRad = lat * M_PI / 180.0f;

    //使用单精度数学函数 sinf 和logf
    float sinLat = sinf(latRad);
    *y = (int)((0.5f - logf((1.0f + sinLat) / (1.0f - sinLat)) / (4.0f * M_PI)) * mapSize);

}
void convert_world_pixels(int64_t old_x , int64_t old_y , int old_zoom, int new_zoom,int64_t * new_x ,int64_t * new_y){
    //边界检查：指针不空，缩放等级在3-14 之间
    if(new_zoom<3 || old_zoom < 3 ||old_zoom > 15 || new_zoom > 15){
        return ;
    }

    if(new_zoom >= old_zoom){
        //放大
        int delta = new_zoom - old_zoom;
        *new_x = old_x <<delta;
        *new_y = old_y <<delta;
    }else{
        //缩小
        int delta = old_zoom - new_zoom;
        *new_x = old_x >>delta;
        *new_y = old_y >>delta;
    }
}
