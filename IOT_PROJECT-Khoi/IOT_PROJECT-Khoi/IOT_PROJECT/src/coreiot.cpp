#include "coreiot.h"
#include "global.h" // <<< Đã thêm

// --- XÓA CÁC BIẾN HARDCODE ---
// const char* coreIOT_Server = "app.coreiot.io";  
// const char* coreIOT_Token = "g7drm1amhd3dchr379xu";
// const int   mqttPort = 1883;
// ----------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // --- SỬA LỖI: Dùng biến global từ global.h/info.dat ---
    if (client.connect("ESP32Client", CORE_IOT_TOKEN.c_str(), NULL)) {
      Serial.println("connected to CoreIOT Server!");
      client.subscribe("v1/devices/me/rpc/request/+");
      Serial.println("Subscribed to v1/devices/me/rpc/request/+");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      vTaskDelay(pdMS_TO_TICKS(5000)); // Đã sửa
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
    // ... (Code của bạn giữ nguyên) ...
}

void setup_coreiot(){
  // Chờ tín hiệu có Internet từ Task_Wifi
  while(1){
    if (xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY)) {
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(500)); // Dùng vTaskDelay
    Serial.print(".");
  }
  Serial.println(" Connected!");

  // --- SỬA LỖI: Dùng biến global từ global.h/info.dat ---
  client.setServer(CORE_IOT_SERVER.c_str(), CORE_IOT_PORT.toInt());
  client.setCallback(callback);
}

void coreiot_task(void *pvParameters){
    setup_coreiot();
    while(1){
        if (!client.connected()) {
            reconnect();
        }
        client.loop();

        // Phần còn lại của task (đọc/gửi data) đã đúng
        float local_temp;
        float local_humi;
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            local_temp = glob_temperature;
            local_humi = glob_humidity;
            xSemaphoreGive(xDataMutex);
        }
        String payload = "{\"temperature\":" + String(local_temp) +  ",\"humidity\":" + String(local_humi) + "}";
        client.publish("v1/devices/me/telemetry", payload.c_str());
        Serial.println("Published payload: " + payload);
        vTaskDelay(10000);
    }
}