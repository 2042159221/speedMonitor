/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Com_Debug.h"
#include "Inf_CST6189.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "ui.h"
#include "lv_demos.h"
#include "fatfs.h"
#include "Inf_power.h"
#include "Inf_ATGM336H.h"
#include "Inf_LSM6DSM.h"
#include "Inf_Slope.h"
#include "Inf_LastLocation.h"
#include <math.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define GPS_TASK_POLL_MS 200U
#define GPS_MAX_FIX_INTERVAL_MS 5000U
#define GPS_MOVING_SPEED_THRESHOLD_KMH 0.5f
#define GPS_LAST_LOCATION_SAVE_INTERVAL_MS 10000U
#define GPS_MIN_DISTANCE_SATELLITES 4U
#define GPS_MIN_MOVE_DISTANCE_M 0.5f
#define GPS_MAX_REASONABLE_SPEED_KMH 120.0f
#define GPS_EARTH_RADIUS_M 6371000.0f
#define GPS_DEG_TO_RAD 0.017453292519943295f
#define GPS_DEBUG_VIRTUAL_LOCATION 0
#define GPS_COORDINATE_USE_GCJ02 0
#define GPS_DEBUG_FRAME_LOG 1
#define GPS_DEBUG_VIRTUAL_LON 113.84f
#define GPS_DEBUG_VIRTUAL_LAT 22.63f
#define GPS_DEBUG_VIRTUAL_SATELLITES 8U
#define GPS_DEBUG_VIRTUAL_UPDATE_MS 1000U
#define POWER_DEBUG_DISABLE_SHUTDOWN GPS_DEBUG_VIRTUAL_LOCATION
#define POWER_TASK_POLL_MS 50U
#define POWER_UI_UPDATE_MS 1000U
#define POWER_DEBUG_LOG_MS 1000U
#define POWER_SHUTDOWN_NOTICE_MS 700U
#define POWER_SHUTDOWN_DELAY_MS 150U

// 判断电池是否在充电
bool charge = false ;
// 获取电池电压值
float v = 0.0f;
uint8_t batterySocPercent = 0U;
bool batteryLow = false;
bool batteryCritical = false;
uint8_t gpsSaleNum = 0;
float lon = 0.0f, lat = 0.0f;
float speed = 0.0f, distance = 0.0f;
float slope = 0.0f;
uint32_t rideTimeMs = 0U;
int rideTimeSec = 0;
bool bikeMoving = false;


/**
 * 更新lvgl标题电池图标
 */
void update_battery_callback(void *args){
  ui_updatePower(charge, v, batterySocPercent, batteryLow, batteryCritical);
}

void show_power_message_callback(void *args){
  ui_showPowerMessage((const char *)args);
}

static void power_shutdown_sequence(const char *message)
{
  COM_DEBUG_LN("shutdown start: %s", message);
  lv_async_call(show_power_message_callback, (void *)message);
  osDelay(POWER_SHUTDOWN_NOTICE_MS);
  (void)f_mount(NULL, "", 1);
  HAL_GPIO_WritePin(ST_BLK_GPIO_Port, ST_BLK_Pin, GPIO_PIN_RESET);
  osDelay(POWER_SHUTDOWN_DELAY_MS);
  Inf_Power_PowerOFF();
  for (;;)
  {
    osDelay(1000);
  }
}

void update_gps_satellite_callback(void *args){
  lv_subject_set_int(&saleNumSubject, gpsSaleNum);
}

//更新码表数据
void update_bike_callback(void *args){
  lv_subject_set_float(&lonSubject, lon);
  lv_subject_set_float(&latSubject, lat);
  lv_subject_set_float(&speedSubject, speed);
  lv_subject_set_float(&distanceSubject, distance);
  lv_subject_snprintf(&timeSubject, "%02d : %02d", rideTimeSec / 60, rideTimeSec % 60);
  ui_roadmap_update_user_location();
  ui_upateBikeState(bikeMoving ? speed : 0.0f);
}

void update_slope_callback(void *args){
  (void)args;
  lv_subject_set_float(&slopeSubject, slope);
}

static bool parse_gga_satellite_num(const char *gga, uint8_t *sat_num)
{
  const char *p = gga;
  uint8_t comma_count = 0;
  uint16_t value = 0;

  while (*p != '\0' && comma_count < 7)
  {
    if (*p == ',')
    {
      comma_count++;
    }
    p++;
  }

  if (comma_count != 7 || *p < '0' || *p > '9')
  {
    return false;
  }

  while (*p >= '0' && *p <= '9')
  {
    value = (uint16_t)(value * 10U + (uint16_t)(*p - '0'));
    if (value > 99U)
    {
      return false;
    }
    p++;
  }

  *sat_num = (uint8_t)value;
  return true;
}

static float nmea_to_degree(float nmea_value)
{
  int degree = (int)(nmea_value / 100.0f);
  float minute = nmea_value - (float)(degree * 100);

  return (float)degree + minute / 60.0f;
}

static bool move_after_next_comma(const char **cursor)
{
  const char *comma = strchr(*cursor, ',');

  if (comma == NULL) {
    return false;
  }

  *cursor = comma + 1;
  return true;
}

static bool parse_nmea_float_field(const char *field, float *value, const char **end)
{
  bool hasDigit = false;
  float integer = 0.0f;
  float fraction = 0.0f;
  float scale = 1.0f;
  float sign = 1.0f;
  const char *p = field;

  if (*p == '-') {
    sign = -1.0f;
    p++;
  } else if (*p == '+') {
    p++;
  }

  while (*p >= '0' && *p <= '9') {
    hasDigit = true;
    integer = integer * 10.0f + (float)(*p - '0');
    p++;
  }

  if (*p == '.') {
    p++;
    while (*p >= '0' && *p <= '9') {
      hasDigit = true;
      fraction = fraction * 10.0f + (float)(*p - '0');
      scale *= 10.0f;
      p++;
    }
  }

  if (!hasDigit) {
    return false;
  }

  *value = sign * (integer + fraction / scale);
  if (end != NULL) {
    *end = p;
  }
  return true;
}

static bool parse_rmc_location(const char *rmc,
                               float *out_lat,
                               char *out_lat_dir,
                               float *out_lon,
                               char *out_lon_dir,
                               float *out_speed)
{
  const char *p = rmc;

  if (!move_after_next_comma(&p)) {
    return false;
  }

  if (!move_after_next_comma(&p)) {
    return false;
  }

  if (*p != 'A') {
    return false;
  }

  if (!move_after_next_comma(&p)) {
    return false;
  }

  if (!parse_nmea_float_field(p, out_lat, &p) || *p != ',') {
    return false;
  }

  p++;
  if (*p != 'N' && *p != 'S') {
    return false;
  }
  *out_lat_dir = *p;

  if (!move_after_next_comma(&p)) {
    return false;
  }

  if (!parse_nmea_float_field(p, out_lon, &p) || *p != ',') {
    return false;
  }

  p++;
  if (*p != 'E' && *p != 'W') {
    return false;
  }
  *out_lon_dir = *p;

  if (!move_after_next_comma(&p)) {
    return false;
  }

  if (*p == ',' || *p == '*' || *p == '\0') {
    *out_speed = 0.0f;
  } else if (!parse_nmea_float_field(p, out_speed, NULL)) {
    return false;
  }

  return true;
}

static float gps_distance_m(float from_lat, float from_lon, float to_lat, float to_lon)
{
  float lat1 = from_lat * GPS_DEG_TO_RAD;
  float lat2 = to_lat * GPS_DEG_TO_RAD;
  float dlat = (to_lat - from_lat) * GPS_DEG_TO_RAD;
  float dlon = (to_lon - from_lon) * GPS_DEG_TO_RAD;
  float mean_lat = (lat1 + lat2) * 0.5f;
  float x = dlon * cosf(mean_lat);
  float y = dlat;

  return sqrtf((x * x) + (y * y)) * GPS_EARTH_RADIUS_M;
}

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for mainTask */
osThreadId_t mainTaskHandle;
const osThreadAttr_t mainTask_attributes = {
  .name = "mainTask",
  .stack_size = 4096 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for powerTask */
osThreadId_t powerTaskHandle;
const osThreadAttr_t powerTask_attributes = {
  .name = "powerTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for gpsTask */
osThreadId_t gpsTaskHandle;
const osThreadAttr_t gpsTask_attributes = {
  .name = "gpsTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for imuTask */
osThreadId_t imuTaskHandle;
const osThreadAttr_t imuTask_attributes = {
  .name = "imuTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow7,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void MainTaskFunc(void *argument);
void PowerTaskFunc(void *argument);
void GPSTaskFunc(void *argument);
void ImuTaskFunc(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of mainTask */
  mainTaskHandle = osThreadNew(MainTaskFunc, NULL, &mainTask_attributes);

  /* creation of powerTask */
  powerTaskHandle = osThreadNew(PowerTaskFunc, NULL, &powerTask_attributes);

  /* creation of gpsTask */
  gpsTaskHandle = osThreadNew(GPSTaskFunc, NULL, &gpsTask_attributes);

  /* creation of imuTask */
  imuTaskHandle = osThreadNew(ImuTaskFunc, NULL, &imuTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_MainTaskFunc */
/**
 * @brief  Function implementing the mainTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_MainTaskFunc */
void MainTaskFunc(void *argument)
{
  /* USER CODE BEGIN MainTaskFunc */

  /* Infinite loop */
  // 启动时需要立即挂载 SD，否则上次定位和地图瓦片会在首次访问时失败。
  const char *mountPath = (SDPath[0] != '\0') ? SDPath : "0:";
  FRESULT mountResult = f_mount(&SDFatFS, mountPath, 1);
  COM_DEBUG_LN("fatfs mount path=%s res=%d", mountPath, mountResult);
#if GPS_DEBUG_VIRTUAL_LOCATION
  lon = GPS_DEBUG_VIRTUAL_LON;
  lat = GPS_DEBUG_VIRTUAL_LAT;
  gpsSaleNum = GPS_DEBUG_VIRTUAL_SATELLITES;
  speed = 0.0f;
  bikeMoving = false;
  ui_setInitialLocation(lon, lat);
  COM_DEBUG_LN("GPS virtual initial lat=%f lon=%f sat=%u",
               lat,
               lon,
               (unsigned int)gpsSaleNum);
#else
  {
    Inf_LastLocation lastLocation;

    if ((mountResult == FR_OK) && Inf_LastLocation_Load(&lastLocation))
    {
      lon = lastLocation.lon;
      lat = lastLocation.lat;
      ui_setInitialLocation(lon, lat);
      COM_DEBUG_LN("last location loaded lat=%f lon=%f", lat, lon);
    }
  }
#endif
  // 初始化lvgl
  lv_init();

  // 设置获取当前时间回调
  lv_tick_set_cb(xTaskGetTickCount);
  // 设置延时回调
  lv_delay_set_cb(vTaskDelay);

  // 初始化输入、显示设备
  lv_port_disp_init();
  lv_port_indev_init();

  // 创建UI
  // 此方法主要用于测试显卡的，换一个可以测试剪贴
  //  lv_demo_benchmark();
  // lv_demo_widgets();
  ui_create();

  // uint16_t x= 0,y =0;
  for (;;)
  {
    // if(Inf_CST816D_IsPressed()){
    //   Inf_CST816D_GetXY(&x,&y);
    //   COM_DEBUG_LN("x = %d , y = %d ",x,y );

    // 获取下次循环所需剩余时间
    uint32_t tm = lv_timer_handler();
    if (tm < 5)
      tm = 5;

    // }
    osDelay(tm);
  }
  /* USER CODE END MainTaskFunc */
}

/* USER CODE BEGIN Header_PowerTaskFunc */
/**
 * @brief Function implementing the powerTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_PowerTaskFunc */
void PowerTaskFunc(void *argument)
{
  /* USER CODE BEGIN PowerTaskFunc */
  uint32_t uiUpdateElapsedMs = POWER_UI_UPDATE_MS;
  uint32_t debugElapsedMs = POWER_DEBUG_LOG_MS;
  const Inf_Power_BatteryState *battery;

  /* Infinite loop */
  // 初始化
  Inf_Power_Init();

  for (;;)
  {
    Inf_Power_Update();
    Inf_Power_KeyScan(POWER_TASK_POLL_MS);
    battery = Inf_Power_GetBatteryState();

    charge = battery->isCharging;
    v = ((float)battery->voltageMv) / 1000.0f;
    batterySocPercent = battery->socPercent;
    batteryLow = battery->isLow;
    batteryCritical = battery->isCritical;

    if (Inf_Power_ShutdownRequested())
    {
#if POWER_DEBUG_DISABLE_SHUTDOWN
      COM_DEBUG_LN("power shutdown request ignored for debug key=%d",
                   HAL_GPIO_ReadPin(PWR_KEY_DET_GPIO_Port, PWR_KEY_DET_Pin));
      Inf_Power_ClearShutdownRequest();
#else
      power_shutdown_sequence("Shutting down");
#endif
    }

    if (battery->isValid && battery->isCritical && !battery->isCharging)
    {
      power_shutdown_sequence("Battery critical");
    }

    uiUpdateElapsedMs += POWER_TASK_POLL_MS;
    if (uiUpdateElapsedMs >= POWER_UI_UPDATE_MS)
    {
      uiUpdateElapsedMs = 0U;
      lv_async_call(update_battery_callback, NULL);
    }

    debugElapsedMs += POWER_TASK_POLL_MS;
    if (debugElapsedMs >= POWER_DEBUG_LOG_MS)
    {
      debugElapsedMs = 0U;
      COM_DEBUG_LN("battery raw=%u mv=%u soc=%u charge=%d low=%d critical=%d key=%d",
                   battery->rawAdc,
                   battery->voltageMv,
                   battery->socPercent,
                   battery->isCharging ? 1 : 0,
                   battery->isLow ? 1 : 0,
                   battery->isCritical ? 1 : 0,
                   HAL_GPIO_ReadPin(PWR_KEY_DET_GPIO_Port, PWR_KEY_DET_Pin));
    }

    osDelay(POWER_TASK_POLL_MS);
  }
  /* USER CODE END PowerTaskFunc */
}

/* USER CODE BEGIN Header_GPSTaskFunc */
/**
* @brief Function implementing the gpsTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_GPSTaskFunc */
void GPSTaskFunc(void *argument)
{
  /* USER CODE BEGIN GPSTaskFunc */
#if GPS_DEBUG_VIRTUAL_LOCATION
  (void)argument;

  lon = GPS_DEBUG_VIRTUAL_LON;
  lat = GPS_DEBUG_VIRTUAL_LAT;
  gpsSaleNum = GPS_DEBUG_VIRTUAL_SATELLITES;
  speed = 0.0f;
  bikeMoving = false;

  osDelay(GPS_DEBUG_VIRTUAL_UPDATE_MS);
  for (;;)
  {
    lv_async_call(update_gps_satellite_callback, NULL);
    lv_async_call(update_bike_callback, NULL);
    COM_DEBUG_LN("GPS virtual update lat=%f lon=%f sat=%u",
                 lat,
                 lon,
                 (unsigned int)gpsSaleNum);
    osDelay(GPS_DEBUG_VIRTUAL_UPDATE_MS);
  }
#else
  /* Infinite loop */
  static char gpsFrame[LOC_BUFF_MAX_SIZE];
  char * str = NULL;
  uint8_t saleNum = 0;
  char lonDir = 0, latDir = 0;
  float parsedLon = 0.0f;
  float parsedLat = 0.0f;
  float rawLon = 0.0f;
  float rawLat = 0.0f;
  float parsedSpeed = 0.0f;
  uint32_t elapsedMs = 0U;
  TickType_t lastFixTime = 0;
  TickType_t currentTime = 0;
  bool hasLastFix = false;
  TickType_t lastLocationSaveTime = 0;
  bool hasLocationSaveAttempt = false;
  float lastDistanceLon = 0.0f;
  float lastDistanceLat = 0.0f;
  bool hasLastDistanceFix = false;
  Inf_ATGM336H_Init();
  for(;;)
  {
    Inf_ATGM336H_Poll();
    if(Inf_ATGM336H_TakeFrame(gpsFrame, sizeof(gpsFrame))){
#if GPS_DEBUG_FRAME_LOG
      COM_DEBUG_LN("GPS frame len=%u gga=%d rmc=%d",
                   (unsigned int)strlen(gpsFrame),
                   strstr(gpsFrame, "GGA,") != NULL ? 1 : 0,
                   strstr(gpsFrame, "RMC,") != NULL ? 1 : 0);
#endif
      //有数据需要接收
      //解析卫星个数
      str = strstr (gpsFrame,"GGA,");
      if (str != NULL){
        if (parse_gga_satellite_num(str, &saleNum)) {
          gpsSaleNum = saleNum;
          lv_async_call(update_gps_satellite_callback, NULL);
        }
      }

      str = strstr (gpsFrame,"RMC,");
      if (str != NULL){
        if(parse_rmc_location(str, &parsedLat, &latDir, &parsedLon, &lonDir, &parsedSpeed)){
          //有效数据，解析经纬度、方向、速度
          //将经纬度格式转换成°
          //纬度xxyy.yyy xx代表° yy.yyy代表′   需要转成 zz.ppp°
          //4006.818848 => 40 + 6.818848 / 60
          rawLat = nmea_to_degree(parsedLat);
          //经度xxxyy.yyy xxx代表° yy.yyy代表′ 需要转成 zzz.ppp°
          rawLon = nmea_to_degree(parsedLon);
          //南纬和西经是负值
          if( latDir == 'S' ){
            rawLat = -rawLat;
          }
          if( lonDir == 'W' ){
            rawLon = -rawLon;
          }

#if GPS_COORDINATE_USE_GCJ02
          //将GPS坐标系转成GCJ02坐标系
          gps_to_gcj02(rawLat, rawLon, &lat, &lon);
#else
          lat = rawLat;
          lon = rawLon;
#endif

          //转换速度的单位
          speed = parsedSpeed * 1.852f;

          //获取当前时间
          currentTime = xTaskGetTickCount();
          bikeMoving = false;
          if (hasLastFix) {
            elapsedMs = (uint32_t)((currentTime - lastFixTime) * portTICK_PERIOD_MS);
            if ((elapsedMs > 0U) && (elapsedMs <= GPS_MAX_FIX_INTERVAL_MS)) {
              float elapsedHour = ((float)elapsedMs) / 3600000.0f;

              if (hasLastDistanceFix && (elapsedHour > 0.0f)) {
                float deltaM = gps_distance_m(lastDistanceLat, lastDistanceLon, lat, lon);
                float deltaSpeedKmh = (deltaM / 1000.0f) / elapsedHour;

                if ((speed > GPS_MOVING_SPEED_THRESHOLD_KMH) &&
                    (gpsSaleNum >= GPS_MIN_DISTANCE_SATELLITES) &&
                    (deltaM >= GPS_MIN_MOVE_DISTANCE_M) &&
                    (deltaSpeedKmh <= GPS_MAX_REASONABLE_SPEED_KMH)) {
                  distance += deltaM / 1000.0f;
                  rideTimeMs += elapsedMs;
                  rideTimeSec = (int)(rideTimeMs / 1000U);
                  bikeMoving = true;
                }
              }
            } else if (elapsedMs > GPS_MAX_FIX_INTERVAL_MS) {
              COM_DEBUG_LN("GPS gap skip interval=%u ms", (unsigned int)elapsedMs);
            }
          } else {
            hasLastFix = true;
          }
          lastFixTime = currentTime;
          lastDistanceLon = lon;
          lastDistanceLat = lat;
          hasLastDistanceFix = true;

          if ((!hasLocationSaveAttempt) ||
              (((uint32_t)((currentTime - lastLocationSaveTime) * portTICK_PERIOD_MS)) >= GPS_LAST_LOCATION_SAVE_INTERVAL_MS))
          {
            bool saveOk = false;

            saveOk = Inf_LastLocation_Save(lon, lat);
            COM_DEBUG_LN("last location save %s lat=%f lon=%f sat=%u",
                         saveOk ? "ok" : "skip",
                         lat,
                         lon,
                         (unsigned int)gpsSaleNum);
            lastLocationSaveTime = currentTime;
            hasLocationSaveAttempt = true;
          }

          COM_DEBUG_LN("lat=%f lon=%f speed=%f distance=%f time=%d",lat,lon,speed,distance,rideTimeSec);
          //更新数据到lvgl
          lv_async_call( update_bike_callback, NULL );
        } else {
          speed = 0.0f;
          bikeMoving = false;
          COM_DEBUG_LN("GPS invalid RMC, speed forced to 0");
          lv_async_call(update_bike_callback, NULL);
        }
      }
    }
    osDelay(GPS_TASK_POLL_MS);
  }
#endif
  /* USER CODE END GPSTaskFunc */
}

/* USER CODE BEGIN Header_ImuTaskFunc */
/**
* @brief Function implementing the imuTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ImuTaskFunc */
void ImuTaskFunc(void *argument)
{
  /* USER CODE BEGIN ImuTaskFunc */
  (void)argument;

  TickType_t lastWake = xTaskGetTickCount();
  TickType_t previousSampleTick = lastWake;
  uint32_t logDivider = 0U;
  uint32_t uiDivider = 0U;

  for (;;)
  {
    if (!Inf_LSM6DSM_IsReady())
    {
      uint8_t id = Inf_LSM6DSM_ReadId();

      if (Inf_LSM6DSM_Init())
      {
        COM_DEBUG_LN("LSM6DSM init ok, WHO_AM_I=0x%02X", id);
        lastWake = xTaskGetTickCount();
        previousSampleTick = lastWake;
        logDivider = 0U;
        uiDivider = 0U;
        Inf_Slope_Init();
      }
      else
      {
        COM_DEBUG_LN("LSM6DSM init failed, WHO_AM_I=0x%02X", id);
        osDelay(1000);
      }
      continue;
    }

    if (Inf_LSM6DSM_GetAccGyro())
    {
      TickType_t nowTick = xTaskGetTickCount();
      uint32_t dtMs = (uint32_t)((nowTick - previousSampleTick) * portTICK_PERIOD_MS);
      bool slopeUpdated;
      const InfSlopeState *slopeState;

      previousSampleTick = nowTick;
      slopeUpdated = Inf_Slope_Update(&accgyro, dtMs);
      slopeState = Inf_Slope_GetState();

      if (slopeUpdated)
      {
        slope = Inf_Slope_GetPercent();
        uiDivider++;
        if (uiDivider >= 5U)
        {
          uiDivider = 0U;
          lv_async_call(update_slope_callback, NULL);
        }
      }

      logDivider++;
      if (logDivider >= 50U)
      {
        logDivider = 0U;
        COM_DEBUG_LN("IMU slope=%.1f%% pitch=%.1f st=%s acc=%.0f,%.0f,%.0f gyroY=%.0f",
                     slopeState->slope_percent,
                     slopeState->pitch_deg,
                     Inf_Slope_StatusName(slopeState->status),
                     accgyro.acc_x,
                     accgyro.acc_y,
                     accgyro.acc_z,
                     accgyro.gyro_y);
      }
    }
    else
    {
      logDivider++;
      if (logDivider >= 50U)
      {
        logDivider = 0U;
        COM_DEBUG_LN("LSM6DSM read failed");
      }
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(20U));
  }
  /* USER CODE END ImuTaskFunc */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

