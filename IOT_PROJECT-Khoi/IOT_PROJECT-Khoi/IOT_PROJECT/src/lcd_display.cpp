#include "global.h"
#include "lcd_display.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h> 

// Địa chỉ I2C, 16 cột, 2 hàng
LiquidCrystal_I2C lcd(33, 16, 2); 

void lcd_display_task(void *pvParameters){
    Serial.println("LCD Task: Running..");
    
    // Khởi tạo LCD
    // Wire.begin(11, 12); // ĐÚNG: Đã xóa (chuyển sang main.cpp)
    lcd.begin();
    lcd.backlight();

    LcdState_t localState;

    while(1){
        // 1. Lấy trạng thái LCD (việc này không dùng I2C, không cần khóa)
        if (xSemaphoreTake(xLcdStateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            localState = g_currentLcdState;
            xSemaphoreGive(xLcdStateMutex);
        }

        // --- SỬA LỖI I2C: Thêm Mutex bảo vệ bus I2C ---
        
        // 2. Yêu cầu quyền truy cập bus I2C
        if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        
            // --- Vùng an toàn I2C ---
            // (Tất cả các lệnh lcd.* phải nằm trong đây)
            lcd.clear();
            lcd.setCursor(0, 0);

            switch (localState) {
                case LCD_NORMAL:
                    lcd.print("Trang thai:");
                    lcd.setCursor(0, 1);
                    lcd.print("BINH THUONG");
                    break;
                
                case LCD_WARNING:
                    lcd.print("Trang thai:");
                    lcd.setCursor(0, 1);
                    lcd.print("!! CANH BAO !!");
                    break;

                case LCD_CRITICAL:
                    lcd.print("Trang thai:");
                    lcd.setCursor(0, 1);
                    lcd.print("!!! NGUY HIEM !!!");
                    break;
            }
            // --- Hết vùng an toàn I2C ---

            // 3. Trả quyền truy cập
            xSemaphoreGive(xI2CMutex);
            
        } else {
             Serial.println("[LCD Task] LỖI: Không thể chiếm được I2C Mutex!");
        }
        // --- KẾT THÚC SỬA LỖI I2C ---


        // 4. Chờ 2 giây cho lần cập nhật tiếp theo
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}