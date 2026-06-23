#include "ui_general.h"

void ui_general_create(lv_obj_t* parent){

    //绑定样式
    lv_obj_bind_style(parent,&darkStyle,0,&styleSubject,1);
    lv_obj_bind_style(parent,&whiteBgStyle,0,&styleSubject,0);

    //内边距
    lv_obj_set_style_pad_hor(parent,8,0);
    lv_obj_set_style_pad_ver(parent,12,0);

    //网格布局
    //声明布局方式
    lv_obj_set_layout(parent,LV_LAYOUT_GRID);

    //2、声明网格
    static int32_t column_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};   /* 2 columns with 100- and 400-px width */
    static int32_t row_dsc[] = {LV_GRID_FR(5), LV_GRID_FR(3), LV_GRID_FR(3),LV_GRID_TEMPLATE_LAST}; /* 3 100-px tall rows */
    lv_obj_set_grid_dsc_array(parent,column_dsc,row_dsc);

    //添加子组件
    //速度
    lv_obj_t* speedLabel = lv_label_create(parent);
    lv_label_bind_text(speedLabel,&speedSubject,"%.1f km/h");
    lv_obj_set_grid_cell(speedLabel,LV_GRID_ALIGN_STRETCH,0,2,LV_GRID_ALIGN_STRETCH,0,1);
    lv_obj_set_style_border_width(speedLabel,4,0);
    lv_obj_set_style_border_color(speedLabel,lv_palette_main(LV_PALETTE_BLUE),0);
    lv_obj_set_style_radius(speedLabel,10,0);
    lv_obj_set_style_text_align(speedLabel,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_font(speedLabel,&lv_font_montserrat_30,0);
    lv_obj_set_style_pad_ver(speedLabel,25,0);

    //3.2、距离
    lv_obj_t* distanceLabel = lv_label_create(parent);
    lv_label_bind_text(distanceLabel,&distanceSubject,"%.2f km");
    lv_obj_set_grid_cell(distanceLabel,LV_GRID_ALIGN_STRETCH,0,1,LV_GRID_ALIGN_STRETCH,1,1);
    lv_obj_set_style_border_width(distanceLabel,4,0);
    lv_obj_set_style_border_color(distanceLabel,lv_palette_main(LV_PALETTE_BLUE),0);
    lv_obj_set_style_radius(distanceLabel,10,0);
    lv_obj_set_style_text_align(distanceLabel,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_font(distanceLabel,&lv_font_montserrat_20,0);
    lv_obj_set_style_pad_ver(distanceLabel,15,0);
    //3.3、坡度
    lv_obj_t* slopeLabel = lv_label_create(parent);
    lv_label_bind_text(slopeLabel,&slopeSubject,"%.1f %%");
    lv_obj_set_grid_cell(slopeLabel,LV_GRID_ALIGN_STRETCH,1,1,LV_GRID_ALIGN_STRETCH,1,1);
    lv_obj_set_style_border_width(slopeLabel,4,0);
    lv_obj_set_style_border_color(slopeLabel,lv_palette_main(LV_PALETTE_BLUE),0);
    lv_obj_set_style_radius(slopeLabel,10,0);
    lv_obj_set_style_text_align(slopeLabel,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_font(slopeLabel,&lv_font_montserrat_20,0);
    lv_obj_set_style_pad_ver(slopeLabel,15,0);
    //3.4、经纬度
    lv_obj_t* div = lv_obj_create(parent);
    lv_obj_bind_style(div,&darkStyle,0,&styleSubject,1);
    lv_obj_bind_style(div,&whiteBgStyle,0,&styleSubject,0);
    lv_obj_set_grid_cell(div,LV_GRID_ALIGN_STRETCH,0,1,LV_GRID_ALIGN_STRETCH,2,1);
    lv_obj_set_style_border_width(div,4,0);
    lv_obj_set_style_border_color(div,lv_palette_main(LV_PALETTE_BLUE),0);
    lv_obj_set_style_radius(div,10,0);
    lv_obj_set_style_pad_ver(div,3,0);
    //经度
    lv_obj_t* lonLabel = lv_label_create(div);
    lv_obj_set_size(lonLabel, lv_pct(100),lv_pct(50));
    lv_label_bind_text(lonLabel,&lonSubject,"%f");
    lv_obj_align(lonLabel,LV_ALIGN_TOP_MID,0,0);
    lv_obj_set_style_text_align(lonLabel,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_font(lonLabel,&lv_font_montserrat_16,0);
    //维度
    lv_obj_t* latLabel = lv_label_create(div);
    lv_obj_set_size(latLabel, lv_pct(100),lv_pct(50));
    lv_label_bind_text(latLabel,&latSubject,"%f");
    lv_obj_align(latLabel,LV_ALIGN_BOTTOM_MID,0,0);
    lv_obj_set_style_text_align(latLabel,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_font(latLabel,&lv_font_montserrat_16,0);
    //3.5、骑行时间
    lv_obj_t* timeLabel = lv_label_create(parent);
    lv_label_bind_text(timeLabel,&timeSubject,"%s");
    lv_obj_set_grid_cell(timeLabel,LV_GRID_ALIGN_STRETCH,1,1,LV_GRID_ALIGN_STRETCH,2,1);
    lv_obj_set_style_border_width(timeLabel,4,0);
    lv_obj_set_style_border_color(timeLabel,lv_palette_main(LV_PALETTE_BLUE),0);
    lv_obj_set_style_radius(timeLabel,10,0);
    lv_obj_set_style_text_align(timeLabel,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_font(timeLabel,&lv_font_montserrat_20,0);
    lv_obj_set_style_pad_ver(timeLabel,15,0);

}
