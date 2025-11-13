#include "global.h"

// Includes cho TẤT CẢ các task
#include "lcd_display.h"
#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "tinyml.h"
#include "coreiot.h"
#include "task_check_info.h"
#include "task_toogle_boot.h"
#include "task_wifi.h"
#include "task_webserver.h"
#include <Wire.h>
// #include "task_core_iot.h" // <-- Đã xóa

// --- Định nghĩa Semaphores và States (cho tất cả các task) ---
SemaphoreHandle_t xTempStateMutex;
TemperatureState_t g_currentTempState = STATE_NORMAL;
SemaphoreHandle_t xHumiStateMutex;
HumidityState_t g_currentHumiState = STATE_COMFORT;
SemaphoreHandle_t xLcdStateMutex;
LcdState_t g_currentLcdState = LCD_NORMAL;
SemaphoreHandle_t xDataMutex;

void setup()
{
  Serial.begin(115200);
  Wire.begin(11, 12);
  check_info_File(0);
  // 1. TẠO TẤT CẢ SEMAPHORE/MUTEX
  xTempStateMutex = xSemaphoreCreateMutex();
  xBinarySemaphoreInternet = xSemaphoreCreateBinary();
  xHumiStateMutex = xSemaphoreCreateMutex();
  xLcdStateMutex = xSemaphoreCreateMutex();
  xDataMutex = xSemaphoreCreateMutex();
  xI2CMutex = xSemaphoreCreateMutex();
  
  // 2. KIỂM TRA TẤT CẢ
  if (xTempStateMutex == NULL) { Serial.println("LỖI: Không thể tạo xTempStateMutex!"); while(1); }
  if (xHumiStateMutex == NULL) { Serial.println("LỖI: Không thể tạo xHumiStateMutex!"); while(1); }
  if (xBinarySemaphoreInternet == NULL) { Serial.println("LỖI: Không thể tạo xBinarySemaphoreInternet!"); while(1); }
  if (xLcdStateMutex == NULL) { Serial.println("LỖI: Không thể tạo xLcdStateMutex!"); while(1); }
  if (xDataMutex == NULL) { Serial.println("LỖI: Không thể tạo xDataMutex!"); while(1); }
  if (xI2CMutex == NULL) { Serial.println("LỖI: Không thể tạo xI2CMutex!"); while(1);}
  Serial.println("Đã tạo tất cả Mutex và Semaphore thành công.");
  // (Task này chạy ngầm để nghe nút BOOT, cho phép xóa cài đặt WiFi)
  xTaskCreate(Task_Toogle_BOOT, "Task Boot Button", 2048, NULL, 5, NULL);
  // 3. KHỞI ĐỘNG LOGIC CHÍNH (SỬA LỖI TASK 4)
  //
  bool credentials_exist = check_info_File(0); // Khởi động LittleFS và đọc file

  if (credentials_exist)
  {
      // --- Chế độ Client (Đã có thông tin WiFi) ---
      Serial.println("Đã tìm thấy thông tin. Khởi động ở chế độ Client.");
      
      // Tạo tất cả các task
      xTaskCreate(Task_Wifi, "Task Wifi", 4096, NULL, 3, NULL); // Sẽ báo hiệu cho CoreIOT
      
      xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL); // Task 1
      xTaskCreate(neo_blinky, "Task NEO Blink", 2048, NULL, 2, NULL); // Task 2
      xTaskCreate(temp_humi_monitor, "Task TEMP HUMI", 4096, NULL, 2, NULL); // Producer
      xTaskCreate(tiny_ml_task, "TinyML", 4096, NULL, 2, NULL); // Task 5
      xTaskCreate(coreiot_task, "CoreIOT Task" ,4096  ,NULL  ,2 , NULL); // Task 6
      xTaskCreate(lcd_display_task, "Task LCD", 4096, NULL, 2, NULL); // Task 3
  }
  else
  {
      // --- Chế độ AP (Chưa có thông tin WiFi) ---
      Serial.println("Không tìm thấy thông tin. Khởi động ở chế độ Access Point.");
      // check_info_File(0) đã tự động gọi startAP()
      
      // Chúng ta chỉ cần một task để chạy web server
      xTaskCreate(Task_Webserver, "Task Webserver", 4096, NULL, 3, NULL); // Task 4

      // Chỉ khởi động các task không cần mạng
      xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL); // Task 1
      xTaskCreate(neo_blinky, "Task NEO Blink", 2048, NULL, 2, NULL); // Task 2
      xTaskCreate(temp_humi_monitor, "Task TEMP HUMI", 4096, NULL, 2, NULL); // Producer
      xTaskCreate(tiny_ml_task, "TinyML", 4096, NULL, 2, NULL); // Task 5
      xTaskCreate(lcd_display_task, "Task LCD", 4096, NULL, 2, NULL); // Task 3
  }
}

void loop()
{
  // Không làm gì ở đây. RTOS đang chạy.
  vTaskDelay(pdMS_TO_TICKS(1000));
}