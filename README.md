# SpeedMonitor / Bike

STM32F405RGTx 自行车码表固件工程，基于 Keil MDK、STM32CubeMX、FreeRTOS、LVGL 和 FatFs。当前重点功能包括 LVGL 仪表 UI、GPS 定位、SD 卡本地地图瓦片显示、电池/电源管理、LSM6DSM IMU 姿态与坡度计算。

## 工程概览

- MCU: `STM32F405RGTx` / `STM32F405RGT6`
- 工程入口: `MDK-ARM/Bike.uvprojx`
- CubeMX 配置: `Bike.ioc`
- RTOS: FreeRTOS CMSIS-RTOS v2
- UI: LVGL
- 文件系统: FatFs over SDIO
- GPS: ATGM336H, NMEA 数据解析
- IMU: LSM6DSM, I2C 接口
- 地图: SD 卡本地 BMP 瓦片，不在线下载

## 目录结构

```text
App/ui/                         LVGL UI 页面和控件逻辑
Com/                            调试输出、延时等通用组件
Core/                           STM32CubeMX 生成和用户主逻辑
Drivers/                        STM32 HAL/CMSIS
FATFS/                          FatFs 应用层和 SD 卡适配
Inf/                            外设与业务接口层
MDK-ARM/                        Keil MDK 工程文件
Middlewares/                    FreeRTOS、FatFs、LVGL 等中间件
docs/                           调试报告和项目记录
```

## 关键任务

FreeRTOS 任务主要在 `Core/Src/freertos.c` 中创建：

- `mainTask`: 初始化 LVGL、显示、触摸、UI 和地图页面。
- `powerTask`: 电池采样、充电状态、PWR_KEY 扫描和关机流程。
- `gpsTask`: ATGM336H GPS 数据接收、NMEA 解析、速度/距离/位置更新。
- `imuTask`: LSM6DSM 初始化、加速度/陀螺仪读取和坡度计算。

## 构建

推荐使用 Keil MDK 打开：

```text
MDK-ARM/Bike.uvprojx
```

最近一次已验证构建结果：

```text
"Bike\\Bike.axf" - 0 Error(s), 0 Warning(s).
```

构建产物默认位于：

```text
MDK-ARM/Bike/
```

该目录已被 `.gitignore` 忽略。

## 烧录注意

当前板子烧录需要手动按住 `PWR_KEY`。流程建议：

1. 按住 `PWR_KEY`。
2. 执行 Keil 或外部工具烧录。
3. 烧录完成后松开 `PWR_KEY`。
4. 重新上电或短按启动。

不要在应用启动阶段一直按住 `PWR_KEY`，否则电源任务可能将其识别为长按关机。

## Roadmap 地图瓦片

地图瓦片从 SD 卡本地读取，不从网络下载。LVGL 路径和 FatFs 实际路径的对应关系：

```text
LVGL:  S:/<zoom>/<tile_x>/<tile_y>/tile.bmp
FatFs: 0:/map/<zoom>/<tile_x>/<tile_y>/tile.bmp
```

示例坐标：

```text
lon=113.84, lat=22.63, zoom=14
center tile = 13372,7134
expected file = 0:/map/14/13372/7134/tile.bmp
```

地图页面通常需要中心瓦片周围的 3x3 瓦片都存在，否则拖动或定位后可能出现空白区域。

## GPS 坐标系规则

当前固件默认使用 WGS-84 坐标，与现有 SD 卡 Web-Mercator 瓦片匹配：

```c
GPS_DEBUG_VIRTUAL_LOCATION 0
GPS_COORDINATE_USE_GCJ02 0
```

真实 GPS、虚拟测试坐标和 SD 卡瓦片生成源必须使用同一坐标系。之前真实 GPS 能显示经纬度但 roadmap 背景空白，根因是 GPS 路径将 WGS-84 转成 GCJ-02，导致瓦片索引偏移，查找到了 SD 卡不存在的瓦片。

详细记录见：

```text
docs/roadmap-gps-tile-debug-report.md
```

## Roadmap 调试日志

串口调试时重点看这些日志：

```text
roadmap load z=...
roadmap center tile=...
map center tile res=... path=...
map tile missing res=... path=...
```

判断方式：

- `res != 0`: 优先检查 SD 卡路径和瓦片文件是否存在。
- `res == 0` 但仍空白: 检查 BMP 格式和 LVGL 图片解码兼容性。
- 读取地图后花屏或卡死: 优先检查 SDIO DMA 是否写入了 CCM 内存。

## 已知约束

- SDIO DMA 不能直接写入 STM32F405 CCM RAM。
- LVGL/FatFs 读取目标若位于 CCM，需要通过普通 SRAM 缓冲区中转。
- 本仓库不提交 Keil 构建输出、日志、备份文件、本地 IDE 配置和助手缓存。

## 远程仓库

```text
git@github.com:2042159221/speedMonitor.git
```
