#pragma once
#include <Arduino.h>
#include "sensor_task.h"
struct ControlState {
 int power[3];
 uint32_t started[3], revision, session, heartbeat;
 SensorData sensor;
 bool online, ready;
};
bool setupControl();
ControlState readControl();
void updateSensor(const SensorData &data);
void networkHeartbeat(bool online);
bool submitControl(int channel, int power, uint32_t session, uint32_t revision);
void fsmTask(void*);
