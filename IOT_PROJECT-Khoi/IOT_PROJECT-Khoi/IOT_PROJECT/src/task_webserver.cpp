#include "task_webserver.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ElegantOTA.h>
#include "global.h"
#include "task_handler.h"
#include "task_wifi.h" // <<< THÊM: Để gọi startAP()

// --- THÊM MỚI: Định nghĩa 2 thiết bị cho Task 4 ---
#define LED1_PIN 21 // (Tạm chọn, bạn có thể đổi)
#define LED2_PIN 22 // (Tạm chọn, bạn có thể đổi)

// Biến trạng thái cho 2 đèn
bool led1_state = false;
bool led2_state = false;
// ------------------------------------

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool webserver_isrunning = false;

void Webserver_sendata(String data)
{
    if (ws.count() > 0)
    {
        ws.textAll(data); // Gửi đến tất cả client đang kết nối
        // Serial.println("📤 Đã gửi dữ liệu qua WebSocket: " + data); // Bỏ spam
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        
        // --- THÊM: Gửi trạng thái đèn khi client mới kết nối ---
        StaticJsonDocument<200> doc;
        doc["type"] = "control";
        doc["led1"] = led1_state ? "ON" : "OFF";
        doc["led2"] = led2_state ? "ON" : "OFF";
        String response;
        serializeJson(doc, response);
        client->text(response);
        // -------------------------------------------------
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->opcode == WS_TEXT)
        {
            String message;
            message += String((char *)data).substring(0, len);
            handleWebSocketMessage(message); // Gọi hàm từ task_handler.cpp
        }
    }
}

void connnectWSV()
{
    // --- THÊM: Khởi tạo 2 pin của Task 4 ---
    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    // ------------------------------------

    // Cài đặt các route (đường dẫn)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/index.html", "text/html"); });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/script.js", "application/javascript"); });
    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/styles.css", "text/css"); });

    // Cài đặt WebSocket
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    // Khởi động ElegantOTA
    ElegantOTA.begin(&server);

    // Khởi động server
    server.begin();
    webserver_isrunning = true;
}

void Webserver_stop()
{
    ws.closeAll();
    server.end();
    webserver_isrunning = false;
}

// void Webserver_reconnect() // (Hàm này không còn cần thiết)

// --- SỬA TÊN TASK ---
void Task_Webserver(void *pvParameters) // Đổi tên thành Task_Webserver
{
    Serial.println("Task Web Server đang khởi chạy...");
    
    // --- Khởi động Access Point (AP) ---
    startAP(); // Gọi hàm từ task_wifi.cpp
    Serial.print("Địa chỉ IP của AP: ");
    Serial.println(WiFi.softAPIP());

    // --- Khởi động Server ---
    connnectWSV();

    Serial.println("Task Web Server đã chạy.");

    unsigned long last_data_send = 0; // Bộ đếm thời gian

    // Vòng lặp vĩnh viễn của Task
    for(;;)
    {
        if (webserver_isrunning)
        {
            ElegantOTA.loop();  
            // ws.cleanupClients();
        }

        // --- THÊM: Gửi dữ liệu cảm biến 2 giây một lần ---
        if (millis() - last_data_send > 2000) {
            last_data_send = millis();
            
            float local_temp;
            float local_humi;

            // 1. Đọc dữ liệu global an toàn (Tuân thủ Task 3)
            if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                local_temp = glob_temperature;
                local_humi = glob_humidity;
                xSemaphoreGive(xDataMutex);
            } else {
                local_temp = 0.0;
                local_humi = 0.0;
            }

            // 2. Tạo JSON
            StaticJsonDocument<200> doc;
            doc["type"] = "sensor"; // Phân biệt loại JSON
            doc["temp"] = local_temp;
            doc["humi"] = local_humi;
            
            String response;
            serializeJson(doc, response);
            
            // 3. Gửi qua WebSocket
            Webserver_sendata(response);
        }
        // --- KẾT THÚC THÊM ---
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}