#include "global.h" // << THÊM CÁI NÀY
#include "neo_blinky.h"
#include <Adafruit_NeoPixel.h> // << THÊM CÁI NÀY

// Khởi tạo đối tượng NeoPixel
Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

void neo_blinky(void *pvParameters){

    strip.begin();
    strip.clear(); // Xóa đèn
    strip.show();  // Cập nhật
    Serial.println("NeoPixel Task: Running..");

    HumidityState_t localState; // Biến trạng thái cục bộ

    while(1) {
        // --- Logic Semaphore Task 2 ---
        // Lấy trạng thái độ ẩm hiện tại một cách an toàn
        if (xSemaphoreTake(xHumiStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            localState = g_currentHumiState;
            xSemaphoreGive(xHumiStateMutex);
        } else {
            // Không lấy được Mutex, tiếp tục dùng trạng thái cũ
        }
        // --- Hết vùng an toàn ---

        // Thực hiện 3 kiểu màu khác nhau
        switch (localState) {
            case STATE_DRY: // Khô: Màu Vàng
                strip.setPixelColor(0, strip.Color(255, 255, 0));
                break;

            case STATE_COMFORT: // Dễ chịu: Màu Xanh lá
                strip.setPixelColor(0, strip.Color(0, 255, 0));
                break;

            case STATE_WET: // Ẩm ướt: Màu Xanh dương
                strip.setPixelColor(0, strip.Color(0, 0, 255));
                break;
        }
        
        // Cập nhật đèn
        strip.show(); 

        // Chờ 1 giây cho lần cập nhật tiếp theo
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}