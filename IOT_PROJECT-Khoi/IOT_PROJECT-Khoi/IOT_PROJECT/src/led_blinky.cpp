#include "global.h"
#include "led_blinky.h"

/**
 * @brief Task nhấp nháy LED
 * Đọc trạng thái chung (g_currentTempState) một cách an toàn và
 * thực hiện kiểu nhấp nháy tương ứng.
 */
void led_blinky(void *pvParameters) {
    pinMode(LED_GPIO, OUTPUT);
    Serial.println("LED Blink Task: Running..");
    
    TemperatureState_t localState; // Biến trạng thái cục bộ

    while(1) {
        // --- Logic Semaphore ---
        // Lấy trạng thái hiện tại một cách an toàn
        if (xSemaphoreTake(xTempStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            localState = g_currentTempState;
            xSemaphoreGive(xTempStateMutex);
        } else {
            // Không lấy được Mutex, tiếp tục dùng trạng thái cũ
        }
        // --- Hết vùng an toàn ---

        // Thực hiện 3 kiểu nhấp nháy
        switch (localState) {
            case STATE_COLD: // Lạnh: Nháy chậm
                digitalWrite(LED_GPIO, HIGH); vTaskDelay(pdMS_TO_TICKS(1000));
                digitalWrite(LED_GPIO, LOW);  vTaskDelay(pdMS_TO_TICKS(1000));
                break;

            case STATE_NORMAL: // Bình thường: Nhịp tim
                digitalWrite(LED_GPIO, HIGH); vTaskDelay(pdMS_TO_TICKS(100));
                digitalWrite(LED_GPIO, LOW);  vTaskDelay(pdMS_TO_TICKS(100));
                digitalWrite(LED_GPIO, HIGH); vTaskDelay(pdMS_TO_TICKS(100));
                digitalWrite(LED_GPIO, LOW);  vTaskDelay(pdMS_TO_TICKS(700));
                break;

            case STATE_HOT: // Nóng: Nháy rất nhanh
                digitalWrite(LED_GPIO, HIGH); vTaskDelay(pdMS_TO_TICKS(100));
                digitalWrite(LED_GPIO, LOW);  vTaskDelay(pdMS_TO_TICKS(100));
                break;
        }
    }
}