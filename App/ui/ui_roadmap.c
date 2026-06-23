#include "ui_roadmap.h"
#include "Com_Debug.h"
#include "fatfs.h"

typedef struct{
    //用户经纬度
    int world_x;
    int world_y;
    int title_x;
    int title_y;
    int relav_world_x;
    int relav_world_y;


    //屏幕中心点
    int center_world_x;
    int center_world_y;
    int center_title_x;
    int center_title_y;
    int center_relav_world_x;
    int center_relav_world_y;

}TitleStruct;

TitleStruct title;

//存储九宫格图片
lv_obj_t* imgs[9];
//图片路径
char imgPath[9][50];


//存储地图窗口宽高全局变量
uint32_t scr_w = 0, scr_h=0;

//上一次滚动条位置角标
int32_t last_scroll_x = 0 ,last_scroll_y= 0;

//是否用户滚动
bool is_user_scroll = true ;

//上一次缩放等级
int last_zoom = 0;

//用户当前位置标记
static lv_obj_t* userMarker = NULL;
static lv_obj_t* roadmapMapDiv = NULL;
static lv_obj_t* roadmapTileDiv = NULL;
static bool autoFollowUser = true;
static bool ignoreNextScrollEnd = false;

#define USER_MARKER_SIZE 15
#define ROADMAP_DEFAULT_ZOOM 14

static void update_user_marker_by_center(int32_t center_world_x,int32_t center_world_y){
    if(userMarker == NULL) return;

    
    int32_t x = (int32_t)scr_w / 2 + (title.world_x - center_world_x);
    int32_t y = (int32_t)scr_h / 2 + (title.world_y - center_world_y);

    if(x < 0 || x > (int32_t)scr_w || y < 0 || y > (int32_t)scr_h){
        lv_obj_add_flag(userMarker,LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_remove_flag(userMarker,LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(userMarker,x - USER_MARKER_SIZE / 2,y - USER_MARKER_SIZE / 2);
    lv_obj_move_foreground(userMarker);
}

static void update_user_marker(void){
    update_user_marker_by_center(title.center_world_x,title.center_world_y);
}

static void update_user_location(void){
    latLngToWorldPixel(lv_subject_get_float(&lonSubject),lv_subject_get_float(&latSubject),lv_subject_get_int(&zoomSubject),&title.world_x,&title.world_y);

    title.title_x = title.world_x / TILE_SIZE;
    title.title_y = title.world_y / TILE_SIZE;
    title.relav_world_x = title.world_x % TILE_SIZE;
    title.relav_world_y = title.world_y % TILE_SIZE;

    update_user_marker();
}

static void center_map_on_user_location(void){
    title.center_world_x = title.world_x;
    title.center_world_y = title.world_y;
    title.center_relav_world_x = title.relav_world_x;
    title.center_relav_world_y = title.relav_world_y;
    title.center_title_x = title.title_x;
    title.center_title_y = title.title_y;
}

static void scroll_map_to_center(lv_obj_t* parent){
    int32_t target_scroll_x = TILE_SIZE + title.center_relav_world_x - (int32_t)scr_w / 2;
    int32_t target_scroll_y = TILE_SIZE + title.center_relav_world_y - (int32_t)scr_h / 2;
    bool scrollChanged = (lv_obj_get_scroll_x(parent) != target_scroll_x) ||
                         (lv_obj_get_scroll_y(parent) != target_scroll_y);

    last_scroll_x = target_scroll_x;
    last_scroll_y = target_scroll_y;
    is_user_scroll = false;
    if(scrollChanged){
        ignoreNextScrollEnd = true;
    }
    lv_obj_scroll_to(parent,last_scroll_x,last_scroll_y,LV_ANIM_OFF);
    is_user_scroll = true;
    update_user_marker();
}

static void user_location_observer_cb(lv_observer_t* observer,lv_subject_t* subject){
    LV_UNUSED(observer);
    LV_UNUSED(subject);

    update_user_location();
}

static void load_map(lv_obj_t* div,lv_obj_t* parent){
    int zoom = lv_subject_get_int(&zoomSubject);
    float lon = lv_subject_get_float(&lonSubject);
    float lat = lv_subject_get_float(&latSubject);

    COM_DEBUG_LN("roadmap load z=%d lat=%f lon=%f", zoom, lat, lon);
    COM_DEBUG_LN("roadmap center tile=%d,%d user tile=%d,%d",
                 title.center_title_x,
                 title.center_title_y,
                 title.title_x,
                 title.title_y);

    //向九宫格容器添加图片
    for( uint8_t i = 0 ;i<9;i++){
        char imgFsPath[64];
        FRESULT statResult;

        if(imgs[i] == NULL){
            //创建图片组件
            imgs[i] = lv_image_create(div);
            //将子组件添加到网格
            lv_obj_set_grid_cell(imgs[i],LV_GRID_ALIGN_STRETCH,i%3,1,LV_GRID_ALIGN_STRETCH,i /3 ,1);
        }

        //加载图片
        // 计算九宫格的 X 和 Y 瓦片索引（添加括号确保优先级正确）
        int tile_x = title.center_title_x + (i % 3) - 1;  // 列偏移: -1, 0, +1
        int tile_y = title.center_title_y + (i / 3) - 1;  // 行偏移: -1, 0, +1

        snprintf(imgPath[i], sizeof(imgPath[i]), "S:/%d/%d/%d/tile.bmp",
                 zoom,
                 tile_x,
                 tile_y);
        snprintf(imgFsPath, sizeof(imgFsPath), "0:/map/%d/%d/%d/tile.bmp",
                 zoom,
                 tile_x,
                 tile_y);

        statResult = f_stat(imgFsPath, NULL);
        if(i == 4U){
            COM_DEBUG_LN("map center tile res=%d path=%s", statResult, imgFsPath);
        }
        if(statResult != FR_OK){
            COM_DEBUG_LN("map tile missing res=%d path=%s", statResult, imgFsPath);
        }

        lv_image_set_src(imgs[i], imgPath[i]);
    }

    //滚动到用户位置作为屏幕中心点
    scroll_map_to_center(parent);
}

void ui_roadmap_update_user_location(void){
    int oldCenterTileX = title.center_title_x;
    int oldCenterTileY = title.center_title_y;
    bool tileChanged;

    update_user_location();

    if(!autoFollowUser || roadmapMapDiv == NULL || roadmapTileDiv == NULL) return;

    tileChanged = (oldCenterTileX != title.title_x) || (oldCenterTileY != title.title_y);
    center_map_on_user_location();

    if(tileChanged){
        load_map(roadmapTileDiv,roadmapMapDiv);
    }else{
        scroll_map_to_center(roadmapMapDiv);
    }
}

//地图所放事件
void zoom_cb(lv_event_t* e) {
    //根据旧的等级缩放像素角标 计算出新的
    convert_world_pixels(title.center_world_x,title.center_world_y,last_zoom,lv_subject_get_int(&zoomSubject),(int64_t*)&title.center_world_x,(int64_t*)&title.center_world_y);

    //更新屏幕中心其他角标
    title.center_title_x = title.center_world_x /TILE_SIZE;
    title.center_title_y = title.center_world_y /TILE_SIZE;
    title.center_relav_world_x = title.center_world_x % TILE_SIZE;
    title.center_relav_world_y = title.center_world_y % TILE_SIZE;

    //缩放小 再 回到原点，会失去焦点，需要重新计算用户当前缩放等级经纬度所在像素角标
    update_user_location();

    last_zoom = lv_subject_get_int(&zoomSubject);
    //获取转入的参数
    lv_obj_t* mapDiv = (lv_obj_t*) lv_event_get_user_data(e);


    load_map(lv_obj_get_child(mapDiv,0),mapDiv);

}

//回到原点
void return_loc_cb(lv_event_t* e ){
    autoFollowUser = true;

    if(lv_subject_get_int(&zoomSubject) != ROADMAP_DEFAULT_ZOOM){
        lv_subject_set_int(&zoomSubject,ROADMAP_DEFAULT_ZOOM);
    }
    last_zoom = ROADMAP_DEFAULT_ZOOM;
    update_user_location();

    center_map_on_user_location();

    lv_obj_t* mapDiv = (lv_obj_t*)lv_event_get_user_data(e);

    load_map(lv_obj_get_child(mapDiv,0),mapDiv);



}

//地图拖动中实时刷新定位点
void map_scrolling_cb(lv_event_t * e){

    if(!is_user_scroll) return;

    lv_obj_t * obj = lv_event_get_target_obj(e);
    int32_t x = lv_obj_get_scroll_x(obj);
    int32_t y = lv_obj_get_scroll_y(obj);

    int32_t center_world_x = title.center_world_x + (x - last_scroll_x);
    int32_t center_world_y = title.center_world_y + (y - last_scroll_y);

    update_user_marker_by_center(center_world_x,center_world_y);
}

//滚动事件
void map_scroll_cb (lv_event_t * e){

    if(ignoreNextScrollEnd){
        ignoreNextScrollEnd = false;
        return;
    }
    if(!is_user_scroll) return;
    autoFollowUser = false;
    //获取目标组件
    lv_obj_t * obj = lv_event_get_target_obj(e);
    //获取滚动条角标
    int32_t x = lv_obj_get_scroll_x(obj);
    int32_t y = lv_obj_get_scroll_y(obj);

    //计算滚动的距离
    int32_t scroll_x =x - last_scroll_x;
    int32_t scroll_y = y - last_scroll_y;

    //计算出当前屏幕中心点的世界像素角标
    title.center_world_x +=scroll_x;
    title.center_world_y +=scroll_y;
    update_user_marker();

    //计算出当前屏幕中心点所在的图片角标
    int tile_x = title.center_world_x / TILE_SIZE;
    int tile_y = title.center_world_y / TILE_SIZE;
    //更新上一次的滚动位置
    last_scroll_x = x;
    last_scroll_y = y;
    if (tile_x == title.center_title_x && tile_y == title.center_title_y){
        //此时屏幕中心还在中心图片
        return;
    }
    //屏幕中心点移出了中心图片，需要重新加载新的九宫格地图
    //更新屏幕中心的角标
    title.center_title_x = tile_x;
    title.center_title_y = tile_y;
    title.center_relav_world_x = title.center_world_x % TILE_SIZE;
    title.center_relav_world_y = title.center_world_y % TILE_SIZE;

    //刷图
    load_map(lv_obj_get_child(obj,0),obj);


}
//处理地图逻辑
static lv_obj_t* ui_roadMap_processMap(lv_obj_t* parent){

    //创建九宫格网格
    lv_obj_t * div = lv_obj_create(parent);
    lv_obj_set_size(div,TILE_SIZE *3 ,TILE_SIZE *3);
    lv_obj_add_style(div,&whiteBgStyle,0);
    lv_obj_set_layout(div,LV_LAYOUT_GRID);
    //声明网格
    static int32_t column_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};   /* 2 columns with 100- and 400-px width */
    static int32_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1),LV_GRID_TEMPLATE_LAST}; /* 3 100-px tall rows */
    lv_obj_set_grid_dsc_array(div,column_dsc,row_dsc);

    //去掉间隙
    lv_obj_set_style_pad_row(div,0,0);
    lv_obj_set_style_pad_column(div,0,0);


    //默认九宫格中心图片是用户经纬度所在的图片，根据用户经纬度计算出世界像素角标
    float lon = lv_subject_get_float(&lonSubject);
    float lat = lv_subject_get_float(&latSubject);
    int zoom = lv_subject_get_int(&zoomSubject);

    latLngToWorldPixel(lon, lat, zoom, &title.world_x, &title.world_y);



    //计算图片角标
    title.title_x = (int)title.world_x / TILE_SIZE;
    title.title_y = (int)title.world_y / TILE_SIZE;



    title.relav_world_x = title.world_x % TILE_SIZE;
    title.relav_world_y = title.world_y % TILE_SIZE;

    //默认用户经纬度所在的位置就是屏幕中心
    title.center_world_x = title.world_x;
    title.center_world_y = title.world_y;
    title.center_title_x = title.title_x;
    title.center_title_y = title.title_y;
    title.center_relav_world_x = title.relav_world_x;
    title.center_relav_world_y = title.relav_world_y;




    //向九宫格添加图片
    load_map(div,parent);

    return div;
}

void ui_roadmap_create(lv_obj_t* parent){

    //绑定样式
    lv_obj_bind_style(parent,&darkStyle,0,&styleSubject,1);
    lv_obj_bind_style(parent,&whiteBgStyle,0,&styleSubject,0);

    //弹性布局
    //声明布局方式
    lv_obj_set_layout(parent,LV_LAYOUT_FLEX);

    //声明排列方向
    lv_obj_set_flex_flow(parent,LV_FLEX_FLOW_COLUMN);

    //去除间隙
    lv_obj_set_style_pad_all(parent,0,0);



    //4、添加子组件
    lv_obj_t* mapDiv = lv_obj_create(parent);
    lv_obj_set_width(mapDiv,lv_pct(100));
    lv_obj_set_style_pad_all(mapDiv,0,0);
    lv_obj_set_style_border_width(mapDiv,0,0);
    lv_obj_set_flex_grow(mapDiv,9);
    lv_obj_set_style_radius(mapDiv,0,0);


    //取消滚动条
    lv_obj_set_scrollbar_mode(mapDiv,LV_SCROLLBAR_MODE_OFF);

    //获取地图窗口的宽高
    lv_obj_update_layout(mapDiv);
    scr_w = lv_obj_get_width(mapDiv);
    scr_h = lv_obj_get_height(mapDiv);
    //关闭弹性栋梁
    lv_obj_remove_flag(mapDiv,LV_OBJ_FLAG_SCROLL_MOMENTUM);
    //关闭弹性滚动
    lv_obj_remove_flag(mapDiv,LV_OBJ_FLAG_SCROLL_ELASTIC);
    //添加滚动事件
    lv_obj_add_event_cb(mapDiv,map_scrolling_cb,LV_EVENT_SCROLL,NULL);
    lv_obj_add_event_cb(mapDiv,map_scroll_cb,LV_EVENT_SCROLL_END,NULL);

   roadmapMapDiv = mapDiv;
   autoFollowUser = true;

   //加载地图
   roadmapTileDiv = ui_roadMap_processMap(mapDiv);

   //创建用户当前位置标记
   userMarker = lv_obj_create(mapDiv);
   lv_obj_set_size(userMarker,USER_MARKER_SIZE,USER_MARKER_SIZE);
   lv_obj_add_flag(userMarker,LV_OBJ_FLAG_IGNORE_LAYOUT | LV_OBJ_FLAG_FLOATING);
   lv_obj_remove_flag(userMarker,LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
   lv_obj_set_style_radius(userMarker,LV_RADIUS_CIRCLE,0);
   lv_obj_set_style_bg_color(userMarker,lv_palette_main(LV_PALETTE_RED),0);
   lv_obj_set_style_border_width(userMarker,2,0);
   lv_obj_set_style_border_color(userMarker,lv_color_white(),0);
   lv_obj_set_style_pad_all(userMarker,0,0);
   lv_obj_move_foreground(userMarker);
   update_user_marker();
   lv_subject_add_observer_obj(&lonSubject,user_location_observer_cb,mapDiv,NULL);
   lv_subject_add_observer_obj(&latSubject,user_location_observer_cb,mapDiv,NULL);


   //创建缩放滚动条
   lv_obj_t* zoomSlider = lv_slider_create(parent);
   lv_obj_set_size(zoomSlider,8,150);
   lv_obj_add_flag(zoomSlider,LV_OBJ_FLAG_IGNORE_LAYOUT);
   lv_obj_align(zoomSlider,LV_ALIGN_RIGHT_MID,-10,0);
   lv_obj_move_foreground(zoomSlider);
   lv_slider_set_range(zoomSlider,3,ROADMAP_DEFAULT_ZOOM);
   lv_slider_bind_value(zoomSlider, &zoomSubject);
   last_zoom = lv_subject_get_int(&zoomSubject);
   lv_obj_add_event_cb(zoomSlider,zoom_cb, LV_EVENT_VALUE_CHANGED,mapDiv);

   //创建回到原点按钮
   lv_obj_t* pointBt = lv_button_create(parent);
   lv_obj_t* pointLabel =  lv_label_create(pointBt);
   lv_label_set_text(pointLabel,LV_SYMBOL_TINT);
   lv_obj_set_size(pointBt,20,20);
   lv_obj_align(pointBt,LV_ALIGN_BOTTOM_LEFT,6,-35);
   //忽略布局影响
   lv_obj_add_flag(pointBt,LV_OBJ_FLAG_IGNORE_LAYOUT);
   //  移动到最顶层
   lv_obj_move_foreground(pointBt);
   //文字对齐
   lv_obj_align(pointLabel,LV_ALIGN_CENTER,0,0);
   lv_obj_set_style_text_align(pointLabel,LV_TEXT_ALIGN_CENTER,0);

   //添加点击事件
   lv_obj_add_event_cb(pointBt,return_loc_cb,LV_EVENT_CLICKED,mapDiv);




    lv_obj_t* infoDiv= lv_obj_create(parent);
    lv_obj_set_width(infoDiv,lv_pct(100));
    lv_obj_bind_style(infoDiv,&darkStyle,0,&styleSubject,1);
    lv_obj_bind_style(infoDiv,&greyBgStyle,0,&styleSubject,0);
    lv_obj_set_flex_grow(infoDiv,1);

    //显示速度和里程
    lv_obj_t* speedLabel = lv_label_create(infoDiv);
    lv_label_bind_text(speedLabel,&speedSubject,"%.1f km/h");
    lv_obj_align(speedLabel,LV_ALIGN_LEFT_MID,15,0);

    lv_obj_t* distanceLabel = lv_label_create(infoDiv);
    lv_label_bind_text(distanceLabel,&distanceSubject,"%.2f km");
    lv_obj_align(distanceLabel,LV_ALIGN_RIGHT_MID,-15,0);
}
