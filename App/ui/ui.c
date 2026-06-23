#include "ui.h"

// mmm : ss\0
// 用于存储骑行时间的缓冲
char timeBuff[9];
// 用于存储上一次骑行时间的缓冲
char beforetimeBuff[9];
lv_obj_t *batteryLabel;
lv_obj_t *runStateLabel;
lv_obj_t *powerMessageOverlay;
lv_obj_t *powerMessageLabel;

static char batteryText[20];

#define UI_DEFAULT_LON 0.0f
#define UI_DEFAULT_LAT 0.0f

static float initialLon = UI_DEFAULT_LON;
static float initialLat = UI_DEFAULT_LAT;



static void ui_style_init(void){
    lv_style_init(&darkStyle);
    //黑色背景
    lv_style_set_bg_color(&darkStyle, lv_color_black());
    //白色字体
    lv_style_set_text_color(&darkStyle, lv_color_white());
    //去掉内边距
    lv_style_set_pad_all(&darkStyle,0);
    //去掉边框
    lv_style_set_border_width(&darkStyle,0);
    //去掉圆角
    lv_style_set_radius(&darkStyle,0);

    lv_style_init(&whiteBgStyle);
    // 白色背景
    lv_style_set_bg_color(&whiteBgStyle, lv_color_white());
    // 黑色字
    lv_style_set_text_color(&whiteBgStyle, lv_color_black());
    // 去掉内边距
    lv_style_set_pad_all(&whiteBgStyle, 0);
    // 去掉边框
    lv_style_set_border_width(&whiteBgStyle, 0);
    // 去掉圆角
    lv_style_set_radius(&whiteBgStyle, 0);

    lv_style_init(&greyBgStyle);
    // 灰色背景
    lv_style_set_bg_color(&greyBgStyle, lv_palette_main(LV_PALETTE_GREY));
    // 黑色字
    lv_style_set_text_color(&greyBgStyle, lv_color_black());
    // 去掉内边距
    lv_style_set_pad_all(&greyBgStyle, 0);
    // 去掉边框
    lv_style_set_border_width(&greyBgStyle, 0);
    // 去掉圆角
    lv_style_set_radius(&greyBgStyle, 0);


}

/**
 * @brief 初始化观察者
 *
 */
static void ui_subject_init(void){
    //赋予初始默认值
    lv_subject_init_int(&saleNumSubject, 0);
    lv_subject_init_float(&speedSubject, 0.0f);
    lv_subject_init_float(&distanceSubject, 0.0f);
    lv_subject_init_float(&slopeSubject, 0.0f);
    lv_subject_init_float(&lonSubject, initialLon);
    lv_subject_init_float(&latSubject, initialLat);
    lv_subject_init_string(&timeSubject, timeBuff, beforetimeBuff, 9, "00 : 00");
    lv_subject_init_int(&zoomSubject, 14);
    lv_subject_init_int(&styleSubject, 0);

}

/**
 * @brief 创建标题
 *
 * @param title
 */
static void ui_create_title(lv_obj_t* title){
    //创建卫星个数
    lv_obj_t* saleLable = lv_label_create(title);

    lv_label_bind_text(saleLable, &saleNumSubject, LV_SYMBOL_GPS " %d");
    lv_obj_align(saleLable, LV_ALIGN_LEFT_MID, 10,0);

    // 运动状态
    runStateLabel = lv_label_create(title);
    ui_common_translation(runStateLabel, tags[0]);
    lv_obj_align(runStateLabel, LV_ALIGN_CENTER,-5,0);

    //电池电量
    batteryLabel = lv_label_create(title);
    lv_label_set_text(batteryLabel, LV_SYMBOL_BATTERY_FULL " 100%");
    lv_obj_align(batteryLabel, LV_ALIGN_RIGHT_MID, -10,0);
}

static void ui_create_power_message_overlay(void){
    lv_obj_t* scr = lv_scr_act();

    powerMessageOverlay = lv_obj_create(scr);
    lv_obj_set_size(powerMessageOverlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(powerMessageOverlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(powerMessageOverlay, LV_OPA_80, 0);
    lv_obj_set_style_border_width(powerMessageOverlay, 0, 0);
    lv_obj_set_style_radius(powerMessageOverlay, 0, 0);
    lv_obj_set_style_pad_all(powerMessageOverlay, 0, 0);
    lv_obj_add_flag(powerMessageOverlay, LV_OBJ_FLAG_HIDDEN);

    powerMessageLabel = lv_label_create(powerMessageOverlay);
    lv_obj_set_style_text_color(powerMessageLabel, lv_color_white(), 0);
    lv_label_set_text(powerMessageLabel, "Shutting down");
    lv_obj_center(powerMessageLabel);
}

/**
 * @brief 创建内容区域
 *
 * @param content
 */
static void  ui_create_content(lv_obj_t* content){
    //创建标签试图实现页面切换
    lv_obj_t* tabview = lv_tabview_create(content);
    lv_obj_bind_style(tabview, &darkStyle, 0 , &styleSubject,1);
    lv_obj_bind_style(tabview, &whiteBgStyle, 0 ,&styleSubject ,0);

    // 添加标签栏
    lv_obj_t *Roadmap = lv_tabview_add_tab(tabview, "Roadmap");
    lv_obj_t *General = lv_tabview_add_tab(tabview, "General");
    lv_obj_t *Settings = lv_tabview_add_tab(tabview, "Settings");

    //设置标签栏显示位置
    lv_tabview_set_tab_bar_position(tabview,LV_DIR_BOTTOM);
    //设置标签栏大小
    lv_tabview_set_tab_bar_size(tabview,25);
    //设置透明度为不透明
    lv_obj_set_style_bg_opa(Roadmap,LV_OPA_COVER,0);
    lv_obj_set_style_bg_opa(General,LV_OPA_COVER,0);
    lv_obj_set_style_bg_opa(Settings,LV_OPA_COVER,0);

    // 获取标签栏容器
    lv_obj_t* tabBar = lv_tabview_get_tab_bar(tabview);

    //设置tab bar里面的标签翻译
    ui_common_translation( lv_obj_get_child(lv_obj_get_child(tabBar,0) , 0 ), tags[4] );
    ui_common_translation( lv_obj_get_child(lv_obj_get_child(tabBar,1) , 0 ), tags[5] );
    ui_common_translation( lv_obj_get_child(lv_obj_get_child(tabBar,2) , 0 ), tags[6] );
    // 获取tabview 内容容器
    lv_obj_t* tabContent = lv_tabview_get_content(tabview);
    lv_obj_bind_style(tabBar, &darkStyle, 0, &styleSubject, 1);
    lv_obj_bind_style(tabBar, &whiteBgStyle, 0, &styleSubject, 0);


    //去除滚动翻页
    lv_obj_remove_flag(lv_tabview_get_content(tabview),LV_OBJ_FLAG_SCROLLABLE);
    //创建设置页面
    ui_settings_create(Settings);
    //创建骑行显示页面
    ui_general_create(General);
    //创建路书页面
    ui_roadmap_create(Roadmap);
    // lv_obj_set_style_bg_color(tab1,lv_palette_main(LV_PALETTE_BLUE),0);
    // lv_obj_set_style_bg_color(tab2,lv_palette_main(LV_PALETTE_RED),0);
    // lv_obj_set_style_bg_color(tab3,lv_palette_main(LV_PALETTE_GREEN),0);
}

/**
 * @brief 画页面
 *
 */
static void ui_create_page(void){
    //1、获取活动屏幕
    lv_obj_t* scr = lv_scr_act();

    //2、创建页面容器
    lv_obj_t* page = lv_obj_create(scr);
    //绑定样式
    lv_obj_bind_style(page, &darkStyle, 0, &styleSubject, 1);
    lv_obj_bind_style(page, &whiteBgStyle, 0, &styleSubject, 0);
    lv_obj_set_size(page,lv_pct(100),lv_pct(100));
    //3、弹性布局
    //3.1、声明布局方式
    lv_obj_set_layout(page, LV_LAYOUT_FLEX);
    //3.2、指定布局方向
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    //3.3、去掉间隙
    lv_obj_set_style_pad_row(page,0,0);
    lv_obj_set_style_pad_column(page,0,0);
    //3.4、添加子元素
    //标题
    lv_obj_t* title = lv_obj_create(page);
    lv_obj_set_width(title,lv_pct(100));
    lv_obj_bind_style(title, &darkStyle, 0, &styleSubject, 1);
    lv_obj_bind_style(title, &greyBgStyle, 0, &styleSubject, 0);
    // lv_obj_set_style_bg_color(title,lv_palette_main(LV_PALETTE_BLUE),0);
    //设置占比
    lv_obj_set_flex_grow(title,1);

    ui_create_title(title);
    //内容
    lv_obj_t* content = lv_obj_create(page);
    lv_obj_set_width(content,lv_pct(100));
    lv_obj_bind_style(content, &darkStyle, 0, &styleSubject, 1);
    lv_obj_bind_style(content, &whiteBgStyle, 0, &styleSubject, 0);
    // lv_obj_set_style_bg_color(content,lv_palette_main(LV_PALETTE_RED),0);
    lv_obj_set_flex_grow(content,9);

    ui_create_content(content);

}

void ui_create(void){
    // 初始化样式
    ui_style_init();

    // 初始化观察者
    ui_subject_init();
    //添加静态翻译
    lv_translation_add_static(languages, tags, translations);
    lv_translation_set_language(languages[0]);

    // 创建ui
    ui_create_page();
    ui_create_power_message_overlay();

}

void ui_setInitialLocation(float lon, float lat)
{
    if ((lon < -180.0f) || (lon > 180.0f) || (lat < -90.0f) || (lat > 90.0f)) {
        return;
    }

    initialLon = lon;
    initialLat = lat;
}


void ui_updateBattery(bool isCharge,float v){
    uint8_t socPercent = 0U;

    if(v >= 4.1f){
        socPercent = 100U;
    }else if(v > 3.95f){
        socPercent = 75U;
    }else if(v > 3.75f){
        socPercent = 50U;
    }else if(v > 3.55f){
        socPercent = 25U;
    }

    ui_updatePower(isCharge, v, socPercent, false, false);
}

void ui_updatePower(bool isCharge, float v, uint8_t socPercent, bool isLow, bool isCritical){
    const char *batteryIcon;

    if(batteryLabel == NULL){
        return;
    }

    if(isCharge){
        lv_snprintf(batteryText, sizeof(batteryText), LV_SYMBOL_CHARGE " %u%%", (unsigned int)socPercent);
        lv_label_set_text(batteryLabel, batteryText);
        return;
    }

    if(isCritical){
        lv_snprintf(batteryText, sizeof(batteryText), LV_SYMBOL_WARNING " CRIT %u%%", (unsigned int)socPercent);
        lv_label_set_text(batteryLabel, batteryText);
        return;
    }

    if(isLow){
        lv_snprintf(batteryText, sizeof(batteryText), LV_SYMBOL_WARNING " LOW %u%%", (unsigned int)socPercent);
        lv_label_set_text(batteryLabel, batteryText);
        return;
    }

    if(socPercent >= 90U){
        batteryIcon = LV_SYMBOL_BATTERY_FULL;
    }else if (socPercent >= 65U){
        batteryIcon = LV_SYMBOL_BATTERY_3;
    }else if (socPercent >= 40U){
        batteryIcon = LV_SYMBOL_BATTERY_2;
    }else if (socPercent >= 15U){
        batteryIcon = LV_SYMBOL_BATTERY_1;
    }else{
        batteryIcon = LV_SYMBOL_BATTERY_EMPTY;
    }

    (void)v;
    lv_snprintf(batteryText, sizeof(batteryText), "%s %u%%", batteryIcon, (unsigned int)socPercent);
    lv_label_set_text(batteryLabel, batteryText);
}

void ui_showPowerMessage(const char *message){
    if(powerMessageOverlay == NULL || powerMessageLabel == NULL){
        return;
    }

    if(message == NULL){
        message = "Shutting down";
    }

    lv_label_set_text(powerMessageLabel, message);
    lv_obj_remove_flag(powerMessageOverlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(powerMessageOverlay);
}

void ui_upateBikeState(float speed){
    if(runStateLabel == NULL){
        return;
    }

    if(speed > 0.5f){
        ui_common_translation(runStateLabel, tags[1]);
    }else{
        ui_common_translation(runStateLabel, tags[0]);
    }
}
