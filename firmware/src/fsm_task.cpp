#include "control.h"
#include <esp_system.h>
#include <esp_arduino_version.h>
// Only LED1, LED2 and MOTOR1. MOTOR2 must NEVER be initialized or written.
static const uint8_t pins[] = {13,12,5};
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static ControlState state = {};
struct Command { int channel, power; uint32_t session, revision, received; };
static QueueHandle_t commands;
static uint8_t stops=0;
static void output(int i,int power) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
 ledcWrite(pins[i], power*255/100);
#else
 ledcWrite(i, power*255/100);
#endif
}
bool setupControl() {
 pinMode(17,OUTPUT); digitalWrite(17,LOW);
 bool ok=true;
 for(int i=0;i<3;i++) {
  pinMode(pins[i],OUTPUT); digitalWrite(pins[i],LOW);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ok=ledcAttach(pins[i],1000,8) && ok;
#else
  ledcSetup(i,1000,8); ledcAttachPin(pins[i],i);
#endif
  output(i,0);
 }
 commands=xQueueCreate(8,sizeof(Command));
 state.session=esp_random(); state.ready=ok && commands;
 return state.ready;
}
ControlState readControl() {
 portENTER_CRITICAL(&mux); auto copy=state; portEXIT_CRITICAL(&mux); return copy;
}
void updateSensor(const SensorData &data) {
 portENTER_CRITICAL(&mux); state.sensor=data; portEXIT_CRITICAL(&mux);
}
void networkHeartbeat(bool online) {
 portENTER_CRITICAL(&mux);
 if(online && !state.online) { state.session=esp_random(); state.revision++; }
 state.online=online; state.heartbeat=millis();
 portEXIT_CRITICAL(&mux);
}
bool submitControl(int channel,int power,uint32_t session,uint32_t revision) {
 if(channel<0 || channel>3 || power<0 || power>100 || (channel==3 && power!=0)) return false;
 if(power==0) {
  portENTER_CRITICAL(&mux); stops |= channel==3 ? 7 : (1<<channel); portEXIT_CRITICAL(&mux);
  return true;
 }
 Command c{channel,power,session,revision,millis()};
 return commands && xQueueSend(commands,&c,0)==pdPASS;
}
void fsmTask(void*) {
 for(;;) {
  auto now=millis();
  Command c;
  // All GPIO writes occur in this task. Short lock protects shared state only.
  portENTER_CRITICAL(&mux);
  bool safe=state.ready && state.online && now-state.heartbeat<3500 &&
    state.sensor.valid && now-state.sensor.timestamp<6000;
  uint8_t stopMask=stops; stops=0;
  for(int i=0;i<3;i++) if((stopMask&(1<<i)) || !safe ||
      (state.power[i]>0 && now-state.started[i]>=20000)) {
   if(state.power[i]) {state.power[i]=0; state.revision++;}
  }
  if(stopMask) state.revision++; // Invalidate queued START after STOP.
  portEXIT_CRITICAL(&mux);
  while(xQueueReceive(commands,&c,0)==pdPASS) {
   portENTER_CRITICAL(&mux);
   if(safe && !stopMask && now-c.received<2000 && c.session==state.session &&
      c.revision==state.revision) {
    // Changing power does not extend the 20-second run deadline.
    if(!state.power[c.channel]) state.started[c.channel]=now;
    state.power[c.channel]=c.power; state.revision++;
   }
   portEXIT_CRITICAL(&mux);
  }
  auto snapshot=readControl();
  for(int i=0;i<3;i++) output(i,snapshot.power[i]);
  vTaskDelay(pdMS_TO_TICKS(20));
 }
}
