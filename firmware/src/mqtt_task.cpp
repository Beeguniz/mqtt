#include "mqtt_task.h"
#include "config.h"
#include "control.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
static WiFiClient transport;
static PubSubClient client(transport);
static const char* names[]={"led1","led2","motor1","all"};
static void receive(char* topic,byte* payload,unsigned int length) {
 if(strcmp(topic,MQTT_COMMAND_TOPIC) || length>256) return;
 StaticJsonDocument<384> doc;
 if(deserializeJson(doc,payload,length)) return;
 if(!doc["target"].is<const char*>() || !doc["power"].is<int>()) return;
 const char* target=doc["target"]; int channel=-1;
 for(int i=0;i<4;i++) if(!strcmp(target,names[i])) channel=i;
 int power=doc["power"];
 if(power && (!doc["session"].is<uint32_t>() || !doc["revision"].is<uint32_t>())) return;
 submitControl(channel,power,doc["session"]|0u,doc["revision"]|0u);
}
void setupMQTT() {
 client.setServer(MQTT_BROKER,MQTT_PORT);
 client.setCallback(receive); client.setBufferSize(1024);
 client.setSocketTimeout(2); client.setKeepAlive(10);
}
void mqttTask(void*) {
 uint32_t attempted=0,backoff=1000,lastState=0,lastTelemetry=0;
 for(;;) {
  uint32_t now=millis();
  if(!client.connected()) {
   networkHeartbeat(false);
   if(WiFi.status()==WL_CONNECTED && now-attempted>=backoff) {
    attempted=now;
    if(client.connect(DEVICE_ID,strlen(MQTT_USER) ? MQTT_USER : nullptr,strlen(MQTT_USER) ? MQTT_PASSWORD : nullptr)) {
     // New connection invalidates any command recorded before reconnect.
     // Commands are clean-session QoS0; state is never retained.
     if(!client.subscribe(MQTT_COMMAND_TOPIC,0)) client.disconnect();
     backoff=1000;
    } else backoff=min<uint32_t>(backoff*2,30000);
   }
  }
  client.loop(); networkHeartbeat(client.connected());
  now=millis();
  if(client.connected() && now-lastState>=1000) {
   auto s=readControl();
   StaticJsonDocument<768> d;
   d["device_id"]=DEVICE_ID; d["session"]=s.session; d["revision"]=s.revision;
   d["uptime_ms"]=now;
   d["valid"]=s.sensor.valid && now-s.sensor.timestamp<6000;
   for(int i=0;i<3;i++) {
    d["outputs"][names[i]]["power"]=s.power[i];
    d["outputs"][names[i]]["remaining_ms"]=s.power[i] ?
      (now-s.started[i]>=20000 ? 0 : 20000-(now-s.started[i])) : 0;
   }
   char buffer[768]; serializeJson(d,buffer,sizeof(buffer));
   client.publish(MQTT_STATE_TOPIC,buffer,false); lastState=now;
  }
  if(client.connected() && now-lastTelemetry>=MQTT_PUB_INTERVAL) {
   auto s=readControl(); bool valid=s.sensor.valid && now-s.sensor.timestamp<6000;
   StaticJsonDocument<512> d;
   d["device_id"]=DEVICE_ID; d["ts"]=s.sensor.timestamp; d["ts_unit"]="uptime_ms";
   d["valid"]=valid;
   if(valid) { d["temp"]=s.sensor.temp; d["humi"]=s.sensor.humi; }
   else {d["temp"]=nullptr; d["humi"]=nullptr;}
   d["units"]["temp"]="C"; d["units"]["humi"]="%";
   d["rain"]=nullptr; d["rain_valid"]=false;
   char buffer[512]; serializeJson(d,buffer,sizeof(buffer));
   client.publish(MQTT_TOPIC,buffer,false); lastTelemetry=now;
  }
  vTaskDelay(pdMS_TO_TICKS(50));
 }
}
