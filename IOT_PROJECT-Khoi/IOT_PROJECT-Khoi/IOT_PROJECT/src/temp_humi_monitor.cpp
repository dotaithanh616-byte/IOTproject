#include "global.h" // Đảm bảo đã include global.h
#include "temp_humi_monitor.h"
#include <DHT20.h>

DHT20 dht20;

void temp_humi_monitor(void *pvParameters){

    dht20.begin(); // Hàm này sẽ được gọi sau Wire.begin() trong main.cpp
    Serial.println("Temp/Humi Task: Running..");

    while (1){
        float temperature;
        float humidity;

        // --- SỬA LỖI I2C: Thêm Mutex bảo vệ bus I2C ---
        if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            dht20.read();
            temperature = dht20.getTemperature();
            humidity = dht20.getHumidity();
            xSemaphoreGive(xI2CMutex);
        } else {
            Serial.println("[Sensor Task] LỖI: Không thể chiếm được I2C Mutex!");
            temperature = humidity = -1; 
        }
        // --- KẾT THÚC SỬA LỖI I2C ---


        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            temperature = humidity =  -1;
        }

        // --- BẮT ĐẦU PHẦN BẢO VỆ DỮ LIỆU (REQ 3) ---
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            glob_temperature = temperature;
            glob_humidity = humidity;
            xSemaphoreGive(xDataMutex);
        }
        // --- KẾT THÚC PHẦN BẢO VỆ ---

        // --- LOGIC TASK 1 (LED) ---
        TemperatureState_t newState;
        if (temperature < 28.0) {
            newState = STATE_COLD;
        } else if (temperature >= 28.0 && temperature < 30.0) {
            newState = STATE_NORMAL;
        } else { // temp >= 30.0
            newState = STATE_HOT;
        }
        
        if (xSemaphoreTake(xTempStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (g_currentTempState != newState) {
                g_currentTempState = newState;
                
                // --- SỬA LỖI (In ra chữ) ---
                Serial.printf("[Sensor Task] Trạng thái NHIỆT ĐỘ mới: %s (%.1f°C)\n", 
                    (newState == STATE_COLD) ? "COLD" :
                    (newState == STATE_NORMAL) ? "NORMAL" : "HOT",
                    temperature);
                // --- HẾT SỬA ---
            }
            xSemaphoreGive(xTempStateMutex);
        }
        // --- KẾT THÚC LOGIC TASK 1 ---

        // --- LOGIC TASK 2 (NEOPIXEL) ---
        HumidityState_t newHumiState;
        if (humidity < 55.0) {
            newHumiState = STATE_DRY;
        } else if (humidity >= 55.0 && humidity < 70.0) {
            newHumiState = STATE_COMFORT;
        } else { // >= 70.0
            newHumiState = STATE_WET;
        }

        if (xSemaphoreTake(xHumiStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (g_currentHumiState != newHumiState) {
                g_currentHumiState = newHumiState;

                //In ra chữ
                Serial.printf("[Sensor Task] Trạng thái ĐỘ ẨM mới: %s (%.1f%%)\n", 
                    (newHumiState == STATE_DRY) ? "DRY" :
                    (newHumiState == STATE_COMFORT) ? "COMFORT" : "WET",
                    humidity);
            }
            xSemaphoreGive(xHumiStateMutex);
        }
        // --- KẾT THÚC LOGIC TASK 2 ---

        // --- Cập nhật trạng thái cho Task 3 (LCD) ---
        LcdState_t newLcdState;
        if (temperature > 32.0 || humidity > 80.0) {
            newLcdState = LCD_CRITICAL;
        } 
        else if (temperature > 30.0 || humidity > 70.0) {
            newLcdState = LCD_WARNING;
        } 
        else {
            newLcdState = LCD_NORMAL;
        }

        if (xSemaphoreTake(xLcdStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            
            // --- BẮT ĐẦU SỬA LỖI ---
            if (g_currentLcdState != newLcdState) {
                g_currentLcdState = newLcdState;
                if (newLcdState == LCD_CRITICAL) {
                    Serial.println("[Sensor Task] Trạng thái LCD mới: CRITICAL");
                } else if (newLcdState == LCD_WARNING) {
                    Serial.println("[Sensor Task] Trạng thái LCD mới: WARNING");
                } else {
                    Serial.println("[Sensor Task] Trạng thái LCD mới: NORMAL");
                }
            }
            xSemaphoreGive(xLcdStateMutex); 
            
        } 

        // --- KẾT THÚC TASK 3 ---

        // In kết quả (giữ nguyên)
        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.print("%  Temperature: ");
        Serial.print(temperature);
        Serial.println("°C");
        
        // --- CSV LOGGING (Giữ lại phần của bạn) ---
        const bool isNormal = (temperature >= 28.0f && temperature < 30.0f) &&
                              (humidity    >= 40.0f && humidity    < 70.0f);
        Serial.print(temperature, 1); Serial.print(",");
        Serial.print(humidity, 1);    Serial.print(",");
        // Serial.println(isNormal ? "normal" : "anomaly");
        // --- HẾT CSV LOGGING ---

        vTaskDelay(pdMS_TO_TICKS(5000)); // Chờ 5 giây
    }
}