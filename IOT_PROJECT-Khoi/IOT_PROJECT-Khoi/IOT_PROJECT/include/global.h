#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern float glob_temperature;
extern float glob_humidity;

extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;

extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;
// --- Thêm code cho Task 1 vào đây ---
typedef enum {
    STATE_COLD,
    STATE_NORMAL,
    STATE_HOT
} TemperatureState_t;
extern SemaphoreHandle_t xTempStateMutex; // Mutex bảo vệ trạng thái nhiệt độ
extern TemperatureState_t g_currentTempState; // Biến trạng thái chung
// --- THÊM MỚI CHO TASK 2 ---

// 2. Định nghĩa 3 trạng thái độ ẩm
typedef enum {
    STATE_DRY,     // Khô
    STATE_COMFORT, // Dễ chịu
    STATE_WET      // Ẩm ướt
} HumidityState_t;

// 3. Khai báo Mutex và biến chung cho Độ ẩm
extern SemaphoreHandle_t xHumiStateMutex;
extern HumidityState_t g_currentHumiState;
// --- THÊM MỚI: Task 3 (LCD Display) ---
typedef enum {
    LCD_NORMAL,
    LCD_WARNING,
    LCD_CRITICAL
} LcdState_t;

extern SemaphoreHandle_t xLcdStateMutex; // Mutex cho trạng thái LCD
extern LcdState_t g_currentLcdState;     // Biến trạng thái LCD

// --- THÊM MỚI: Bảo vệ Dữ liệu (cho Req 3) ---
extern SemaphoreHandle_t xDataMutex; // Mutex để bảo vệ glob_temperature/glob_humidity
extern SemaphoreHandle_t xI2CMutex; // <<< THÊM DÒNG NÀY
#endif