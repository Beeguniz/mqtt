#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
struct SensorData {
 float temp, humi;
 int rain;
 uint32_t timestamp;
 bool valid;
};
extern QueueHandle_t sensorQueue;
void sensorTask(void*);
