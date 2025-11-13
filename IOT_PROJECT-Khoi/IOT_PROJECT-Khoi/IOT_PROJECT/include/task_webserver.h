#ifndef __TASK_WEBSERVER_H__
#define __TASK_WEBSERVER_H__

#include <Arduino.h>
#include <ESPAsyncWebServer.h> // Thêm các thư viện phụ thuộc
#include <ElegantOTA.h>
#include <AsyncWebSocket.h>
extern AsyncWebSocket ws;
// Hàm này được gọi bởi main.cpp (chế độ AP)
void Task_Webserver(void *pvParameters);

// Các hàm khác trong tệp .cpp
void connnectWSV();
void Webserver_stop();
void Webserver_sendata(String data);
void handleWebSocketMessage(String message);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

// Hàm cũ (không còn dùng)
// void Webserver_reconnect();

#endif