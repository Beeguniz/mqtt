#include "sensor_task.h"
#include "control.h"
#include <Wire.h>
void sensorTask(void*) {
 Wire.begin(33,32); Wire.setTimeOut(100);
 for(;;) {
  SensorData data{}; data.rain=-1; data.timestamp=millis();
  // Agreed provisional UNO format: uint16 BE /10 temperature, then humidity.
  size_t n=Wire.requestFrom(uint8_t(0x08),uint8_t(4));
  if(n==4 && Wire.available()==4) {
   uint8_t b[4]; for(auto &v:b) v=Wire.read();
   data.temp=((uint16_t(b[0])<<8)|b[1])/10.0f;
   data.humi=((uint16_t(b[2])<<8)|b[3])/10.0f;
   data.valid=data.temp<=80 && data.humi<=100;
  }
  while(Wire.available()) Wire.read();
  updateSensor(data);
  xQueueOverwrite(sensorQueue,&data);
  vTaskDelay(pdMS_TO_TICKS(2000));
 }
}
