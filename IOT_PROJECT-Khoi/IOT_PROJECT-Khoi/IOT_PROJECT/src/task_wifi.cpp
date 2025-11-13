#include "task_wifi.h"
#include "global.h" // <<< Thêm
#include <WiFi.h>   // <<< Thêm

// Hàm startAP (được Task_Webserver sử dụng)
void startAP()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(String(SSID_AP), String(PASS_AP));
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
}

// --- ĐÂY LÀ TASK WIFI MỚI ---
void Task_Wifi(void *pvParameters)
{
    Serial.println("Task Wifi: Đang chạy...");

    // Chờ thông tin WiFi được nạp (từ check_info_File)
    while (WIFI_SSID.isEmpty()) {
        Serial.println("Task Wifi: Đang chờ WIFI_SSID...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    Serial.printf("Task Wifi: Đang kết nối tới %s...\n", WIFI_SSID.c_str());
    WiFi.mode(WIFI_STA); // Chuyển sang chế độ STA

    if (WIFI_PASS.isEmpty()) {
        WiFi.begin(WIFI_SSID.c_str());
    } else {
        WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    }

    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        Serial.print(".");
    }

    Serial.println("\nTask Wifi: Đã kết nối WiFi!");
    Serial.print("Địa chỉ IP: ");
    Serial.println(WiFi.localIP());
    isWifiConnected = true;
    
    // Báo hiệu cho các task khác (CoreIOT) rằng Internet đã sẵn sàng
    xSemaphoreGive(xBinarySemaphoreInternet); // (Giống code cũ)
    
    // Task Wifi đã xong việc, xóa chính nó
    vTaskDelete(NULL);
}

// Hàm Wifi_reconnect() không còn cần thiết trong cấu trúc RTOS này
// bool Wifi_reconnect() { ... }