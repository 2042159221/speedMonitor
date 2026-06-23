#include "Inf_LastLocation.h"

#include "Com_Debug.h"
#include "fatfs.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LAST_LOCATION_FILE_NAME "lastloc.bin"
#define LAST_LOCATION_MAGIC 0x434F4C42UL
#define LAST_LOCATION_VERSION 1U
#define LAST_LOCATION_SCALE 10000000.0f
#define LAST_LOCATION_LEGACY_FAKE_LON_E7 1140345570L
#define LAST_LOCATION_LEGACY_FAKE_LAT_E7 225516840L
#define LAST_LOCATION_LEGACY_FAKE_TOLERANCE_E7 500L
#define LAST_LOCATION_ZERO_TOLERANCE_E7 10L
#define LAST_LOCATION_ZERO_TOLERANCE_DEG 0.000001f

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    int32_t lon_e7;
    int32_t lat_e7;
    uint32_t checksum;
} LastLocationRecord;

static uint32_t calc_checksum(const void *data, uint32_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t hash = 2166136261UL;

    for (uint32_t i = 0U; i < size; i++) {
        hash ^= bytes[i];
        hash *= 16777619UL;
    }

    return hash;
}

static bool is_e7_valid(int32_t lon_e7, int32_t lat_e7)
{
    return (lon_e7 >= -1800000000L) &&
           (lon_e7 <= 1800000000L) &&
           (lat_e7 >= -900000000L) &&
           (lat_e7 <= 900000000L);
}

static int32_t abs_i32(int32_t value)
{
    return (value < 0) ? -value : value;
}

static bool is_legacy_fake_location(int32_t lon_e7, int32_t lat_e7)
{
    return (abs_i32(lon_e7 - LAST_LOCATION_LEGACY_FAKE_LON_E7) <= LAST_LOCATION_LEGACY_FAKE_TOLERANCE_E7) &&
           (abs_i32(lat_e7 - LAST_LOCATION_LEGACY_FAKE_LAT_E7) <= LAST_LOCATION_LEGACY_FAKE_TOLERANCE_E7);
}

static bool is_zero_location_e7(int32_t lon_e7, int32_t lat_e7)
{
    return (abs_i32(lon_e7) <= LAST_LOCATION_ZERO_TOLERANCE_E7) &&
           (abs_i32(lat_e7) <= LAST_LOCATION_ZERO_TOLERANCE_E7);
}

bool Inf_LastLocation_IsValid(float lon, float lat)
{
    return (lon >= -180.0f) && (lon <= 180.0f) &&
           (lat >= -90.0f) && (lat <= 90.0f) &&
           !((lon > -LAST_LOCATION_ZERO_TOLERANCE_DEG) && (lon < LAST_LOCATION_ZERO_TOLERANCE_DEG) &&
             (lat > -LAST_LOCATION_ZERO_TOLERANCE_DEG) && (lat < LAST_LOCATION_ZERO_TOLERANCE_DEG));
}

static int32_t degree_to_e7(float value)
{
    float scaled = value * LAST_LOCATION_SCALE;

    if (scaled >= 0.0f) {
        scaled += 0.5f;
    } else {
        scaled -= 0.5f;
    }

    return (int32_t)scaled;
}

static float e7_to_degree(int32_t value)
{
    return ((float)value) / LAST_LOCATION_SCALE;
}

static void build_file_path(char *path, uint32_t path_size)
{
    if ((path == NULL) || (path_size == 0U)) {
        return;
    }

    if (SDPath[0] != '\0') {
        (void)snprintf(path, path_size, "%s/%s", SDPath, LAST_LOCATION_FILE_NAME);
        return;
    }

    (void)snprintf(path, path_size, "0:/%s", LAST_LOCATION_FILE_NAME);
}

static bool record_is_valid(const LastLocationRecord *record)
{
    uint32_t checksum;

    if (record == NULL) {
        return false;
    }
    if ((record->magic != LAST_LOCATION_MAGIC) ||
        (record->version != LAST_LOCATION_VERSION) ||
        (record->size != sizeof(LastLocationRecord))) {
        return false;
    }
    if (!is_e7_valid(record->lon_e7, record->lat_e7) ||
        is_zero_location_e7(record->lon_e7, record->lat_e7) ||
        is_legacy_fake_location(record->lon_e7, record->lat_e7)) {
        return false;
    }

    checksum = calc_checksum(record, offsetof(LastLocationRecord, checksum));
    return checksum == record->checksum;
}

bool Inf_LastLocation_Load(Inf_LastLocation *location)
{
    FIL file;
    LastLocationRecord record;
    UINT bytes_read = 0U;
    FRESULT result;
    char path[24];

    if (location == NULL) {
        return false;
    }

    location->valid = false;
    build_file_path(path, sizeof(path));

    result = f_open(&file, path, FA_READ);
    if (result != FR_OK) {
        COM_DEBUG_LN("last location open failed res=%d path=%s", result, path);
        return false;
    }

    result = f_read(&file, &record, sizeof(record), &bytes_read);
    (void)f_close(&file);

    if ((result != FR_OK) || (bytes_read != sizeof(record))) {
        COM_DEBUG_LN("last location read failed res=%d bytes=%u path=%s",
                     result,
                     (unsigned int)bytes_read,
                     path);
        return false;
    }

    if (!record_is_valid(&record)) {
        COM_DEBUG_LN("last location invalid lon_e7=%ld lat_e7=%ld path=%s",
                     (long)record.lon_e7,
                     (long)record.lat_e7,
                     path);
        return false;
    }

    location->lon = e7_to_degree(record.lon_e7);
    location->lat = e7_to_degree(record.lat_e7);
    location->valid = true;
    return true;
}

bool Inf_LastLocation_Save(float lon, float lat)
{
    FIL file;
    LastLocationRecord record;
    UINT bytes_written = 0U;
    FRESULT result;
    char path[24];

    if (!Inf_LastLocation_IsValid(lon, lat)) {
        COM_DEBUG_LN("last location save invalid lat=%f lon=%f", lat, lon);
        return false;
    }

    record.magic = LAST_LOCATION_MAGIC;
    record.version = LAST_LOCATION_VERSION;
    record.size = sizeof(record);
    record.lon_e7 = degree_to_e7(lon);
    record.lat_e7 = degree_to_e7(lat);
    if (is_zero_location_e7(record.lon_e7, record.lat_e7) ||
        is_legacy_fake_location(record.lon_e7, record.lat_e7)) {
        COM_DEBUG_LN("last location save rejected lon_e7=%ld lat_e7=%ld",
                     (long)record.lon_e7,
                     (long)record.lat_e7);
        return false;
    }
    record.checksum = calc_checksum(&record, offsetof(LastLocationRecord, checksum));

    build_file_path(path, sizeof(path));

    result = f_open(&file, path, FA_CREATE_ALWAYS | FA_WRITE);
    if (result != FR_OK) {
        COM_DEBUG_LN("last location save open failed res=%d path=%s", result, path);
        return false;
    }

    result = f_write(&file, &record, sizeof(record), &bytes_written);
    if (result == FR_OK) {
        result = f_sync(&file);
    }
    (void)f_close(&file);

    if ((result != FR_OK) || (bytes_written != sizeof(record))) {
        COM_DEBUG_LN("last location save write failed res=%d bytes=%u path=%s",
                     result,
                     (unsigned int)bytes_written,
                     path);
    }

    return (result == FR_OK) && (bytes_written == sizeof(record));
}
