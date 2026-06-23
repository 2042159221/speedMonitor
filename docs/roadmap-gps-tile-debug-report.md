# Roadmap GPS Tile Debug Report

Date: 2026-06-23

## Background

The Roadmap page could display GPS longitude and latitude in the LVGL UI, but the map background stayed blank when using real outdoor GPS data.

Indoor virtual test data at `lon=113.84`, `lat=22.63` could load map tiles correctly.

## Findings

### 1. Tile loading is local file based

The firmware does not download map tiles. Roadmap loads local BMP files from the SD card through LVGL FatFs:

```text
S:/<zoom>/<tile_x>/<tile_y>/tile.bmp
```

With current LVGL configuration:

```text
LV_FS_FATFS_LETTER = 'S'
LV_FS_FATFS_PATH   = "0:/map"
```

So the real FatFs path is:

```text
0:/map/<zoom>/<tile_x>/<tile_y>/tile.bmp
```

For `lon=113.84`, `lat=22.63`, `zoom=14`, the expected center tile is:

```text
0:/map/14/13372/7134/tile.bmp
```

The 3x3 grid around that center is also required.

### 2. The blank-map cause was coordinate-system mismatch

The virtual test coordinate was fed directly to the UI as WGS-84:

```text
113.84, 22.63 -> z14 tile 13372,7134
```

The real GPS path previously converted NMEA WGS-84 coordinates to GCJ-02 before updating LVGL:

```c
gps_to_gcj02(lat, lon, &lat, &lon);
```

For the same point, GCJ-02 shifts the coordinate to approximately:

```text
113.844986, 22.627025 -> z14 tile 13373,7134
```

If the SD card contains WGS/Web-Mercator tiles generated for `13372/7134`, the real GPS path will look for `13373/7134` after GCJ conversion and the map appears blank.

### 3. Outdoor retest NMEA points to a different tile area

The outdoor NMEA sample contains this valid WGS-84 position:

```text
$GNGGA,...,4006.81888,N,11621.89413,E,...
$GNRMC,...,A,4006.81888,N,11621.89413,E,...
```

Converted to decimal degrees:

```text
lat = 40 + 6.81888 / 60  = 40.113648
lon = 116 + 21.89413 / 60 = 116.364902
```

At `zoom=14`, this position maps to:

```text
center tile = 13487,6195
expected file = 0:/map/14/13487/6195/tile.bmp
```

This is not the same area as the virtual test coordinate:

```text
113.84,22.63 -> z14 tile 13372,7134
```

If the SD card only contains tiles around `13372/7134`, the firmware will correctly look for `13487/6195` outdoors and still display a blank map. In that case the fix is to generate/copy the 3x3 tile set around the real test location, not to change LVGL image loading.

### 4. The earlier flower-screen freeze was DMA/CCM related

After real tiles started loading, the board once showed display corruption and froze.

Root cause: LVGL heap was configured in STM32F405 CCM RAM:

```text
LV_MEM_ADR = 0x10000000
```

CCM RAM cannot be accessed by DMA. BMP decoding reads SD card data through FatFs into LVGL-allocated buffers. Since SDIO uses DMA, reading directly into a CCM buffer can corrupt or stall the runtime.

The FatFs LVGL driver was changed to detect CCM destination buffers and use a small normal-SRAM scratch buffer before copying into CCM.

## Code Changes

### `Core/Src/freertos.c`

- `GPS_DEBUG_VIRTUAL_LOCATION` was restored to `0`.
- Added `GPS_COORDINATE_USE_GCJ02`.
- Real GPS coordinates now default to WGS-84, matching the existing SD tile set.
- GCJ-02 conversion is still available by setting `GPS_COORDINATE_USE_GCJ02` to `1`.
- Added a short GPS frame log showing whether the frame handed to the parser contains `GGA` and `RMC`.

### `Inf/Inf_ATGM336H.c`

- Real UART receive now accumulates location-bearing NMEA fragments until an `RMC` sentence is present.
- The GPS task receives a more complete positioning cycle instead of whichever `GGA`/`RMC` fragment happened to be latest at the polling time.

### `App/ui/ui_roadmap.c`

- Added debug logs for:
  - current zoom
  - current lat/lon
  - center tile index
  - user tile index
  - center tile FatFs path and `f_stat()` result

Useful serial log markers:

```text
roadmap load z=...
roadmap center tile=...
map center tile res=... path=...
map tile missing res=... path=...
```

### `Middlewares/lvgl/src/libs/fsdrv/lv_fs_fatfs.c`

- Added CCM-range detection for FatFs reads.
- Reads targeting CCM memory now go through a 512-byte normal-SRAM scratch buffer.
- This prevents SDIO DMA from writing directly into CCM.

## Build Verification

Keil MDK build was executed through the project script:

```text
python3 .codex/skills/stm32-compile/scripts/mdk_build.py --workspace . --pretty
```

Result:

```text
"Bike\\Bike.axf" - 0 Error(s), 0 Warning(s).
```

Latest manual flash artifact:

```text
MDK-ARM/Bike/Bike.hex
```

## Current Operating Rule

The coordinate system of all three inputs must match:

1. GPS coordinates used by firmware
2. Virtual test coordinates
3. SD card tile generation source

Current mode:

```text
GPS / virtual data / SD tiles = WGS-84 Web Mercator
GPS_COORDINATE_USE_GCJ02 = 0
```

If switching to GCJ-02 map tiles, update both real GPS and virtual test path:

```text
GPS_COORDINATE_USE_GCJ02 = 1
virtual test coordinates must also be GCJ-02
```

## Manual Flash Note

This board requires holding `PWR_KEY` during manual flashing. After flashing:

1. Release `PWR_KEY`.
2. Power-cycle or short-press boot normally.
3. Do not keep holding `PWR_KEY` while the application starts, otherwise the power task may treat it as a long-press shutdown request.

## Next Debug Checklist

If roadmap is blank again:

1. Check serial log for the exact center tile:

```text
roadmap center tile=x,y
map center tile res=n path=0:/map/z/x/y/tile.bmp
```

2. If `res != 0`, inspect SD card tile directory and file existence.
3. If `res == 0` but display is wrong, inspect BMP format and LVGL decoder compatibility.
4. If the board freezes, inspect DMA target buffers and SDIO read path first.
