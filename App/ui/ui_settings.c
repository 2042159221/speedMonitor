#include "ui_settings.h"

/**
 * @brief 模式切换事件
 *
 * @param e
 */
void changeMode(lv_event_t * e){

    //获取目标对象
    lv_obj_t* obj = lv_event_get_target_obj(e);
    //设置样式观察者的值 改变深浅色模式
    lv_subject_set_int(&styleSubject, lv_obj_has_state(obj,LV_STATE_CHECKED)? 1 : 0);
}
/**
 * @brief 语言选择
 *
 * @param e
 */
void language_change_cb(lv_event_t* e){


    //获取下拉框对象
    lv_obj_t* obj = lv_event_get_target_obj(e);

    //获取选择的文本
    uint32_t index = lv_dropdown_get_selected(obj);
    // lv_dropdown_get_selected_str(obj);

    //设置语言
    lv_translation_set_language(languages[index]);
}

void ui_settings_create(lv_obj_t* parent){
     // 绑定样式
    lv_obj_bind_style(parent, &darkStyle, 0, &styleSubject, 1);
    lv_obj_bind_style(parent, &whiteBgStyle, 0, &styleSubject, 0);
    lv_obj_set_style_pad_top(parent,20,0);
    lv_obj_set_style_pad_hor(parent,25,0);

    lv_obj_t * div = lv_obj_create(parent);
    lv_obj_set_size(div,lv_pct(100),lv_pct(40));
    lv_obj_bind_style(div, &darkStyle,0, &styleSubject,1);
    lv_obj_bind_style(div, &whiteBgStyle,0 , &styleSubject,0);
    //网格布局
    //1、声明布局样式
    lv_obj_set_layout(div, LV_LAYOUT_GRID);

    //2、指定网格
    static int32_t column_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};   /* 2 columns with 100- and 400-px width */
    static int32_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST}; /* 3 100-px tall rows */
    lv_obj_set_grid_dsc_array(div,column_dsc,row_dsc);

    //3.添加子组件到网络
    //语言标签
    lv_obj_t* languageLabel = lv_label_create(div);
    // lv_label_set_text(languageLabel,"Language");
    ui_common_translation(languageLabel,tags[2]);
    lv_obj_set_grid_cell(languageLabel,LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER ,0,1);
    lv_obj_set_style_pad_top(languageLabel,10,0);

    //语言 下拉选项
    lv_obj_t* languageDP = lv_dropdown_create(div);
    lv_obj_bind_style(languageDP, &darkStyle,0, &styleSubject, 1);
    lv_obj_bind_style(languageDP, &whiteBgStyle,0 ,&styleSubject,0);
    //控制边框粗细
    lv_obj_set_style_border_width(languageDP,3,0);
    //控制内边距
    lv_obj_set_style_pad_all(languageDP,6,0);
    //设置空间宽度
    lv_obj_set_width(languageDP,100);
    //设置下来选项
    lv_dropdown_set_options(languageDP, "English\n"
                            "Chinese");


    //添加事件
    lv_obj_add_event_cb(languageDP,language_change_cb,LV_EVENT_VALUE_CHANGED,NULL);

    //获取下拉选框
    lv_obj_t * list = lv_dropdown_get_list(languageDP);
    lv_obj_bind_style(list, &darkStyle,0, &styleSubject ,1);
    lv_obj_bind_style(list, &whiteBgStyle,0, &styleSubject ,0);
    lv_obj_set_style_border_width(list,3,0);
    lv_obj_set_style_pad_all(list,6,0);

    //设置下拉选框在网格中的位置
    lv_obj_set_grid_cell(languageDP, LV_GRID_ALIGN_END,1,1,LV_GRID_ALIGN_END,0,1);

    //模式标签
    lv_obj_t * modeLabel = lv_label_create(div);
    //lv_label_set_text(modeLabel,"Dark Mode");
    ui_common_translation(modeLabel,tags[3]);
    //从上边沿向下挤压10个像素点 【第三个参数是选择器】
    lv_obj_set_style_pad_top(modeLabel,10,0);
    //设置在网格中的布局位置
    lv_obj_set_grid_cell(modeLabel, LV_GRID_ALIGN_START,0,1,LV_GRID_ALIGN_CENTER,1,1);

    //模式开关
    lv_obj_t* modeSW = lv_switch_create(div);
    //设置开关在网格中的布局位置
    lv_obj_set_grid_cell(modeSW, LV_GRID_ALIGN_END,1,1,LV_GRID_ALIGN_END,1,1);

    //添加事件
    lv_obj_add_event_cb(modeSW,changeMode,LV_EVENT_VALUE_CHANGED,NULL);


}
