#pragma once
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif
#define DEVICE_ID "node01"
#define MQTT_PORT 1883
#define MQTT_TOPIC "nhom01/node01/telemetry"
#define MQTT_COMMAND_TOPIC "nhom01/node01/command"
#define MQTT_STATE_TOPIC "nhom01/node01/state"
#define MQTT_PUB_INTERVAL 10000
#define WIFI_MAX_RETRIES 20
#define WIFI_RETRY_DELAY 5000
