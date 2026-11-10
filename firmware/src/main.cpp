#include <Arduino.h>
#include "sensor_task.h"
#include "mqtt_task.h"
#include "wifi_manager.h"
#include "control.h"
QueueHandle_t sensorQueue=nullptr;
static void require(bool ok) {
 if(!ok) { Serial.println("Startup failed; restarting with outputs OFF."); delay(100); ESP.restart(); }
}
void setup() {
 Serial.begin(115200);
 require(setupControl()); // Outputs OFF before waiting for any network.
 sensorQueue=xQueueCreate(1,sizeof(SensorData)); require(sensorQueue!=nullptr);
 require(xTaskCreatePinnedToCore(fsmTask,"Safety",4096,nullptr,3,nullptr,0)==pdPASS);
 require(xTaskCreatePinnedToCore(sensorTask,"Sensors",4096,nullptr,2,nullptr,1)==pdPASS);
 setupMQTT(); setupWiFi();
 require(xTaskCreatePinnedToCore(mqttTask,"MQTT",8192,nullptr,1,nullptr,1)==pdPASS);
 require(xTaskCreatePinnedToCore(wifiTask,"WiFi",4096,nullptr,2,nullptr,0)==pdPASS);
}
void loop() { vTaskDelay(pdMS_TO_TICKS(10000)); }
