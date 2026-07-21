#include <Arduino.h>
#include <LittleFS.h>

#include "version.h"
#include "config.h"
#include "wifi.h"
#include "web.h"
#include "radio.h"
#include "mqtt.h"
#include "rxslots.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(FW_NAME);
  Serial.println(FW_VERSION);

  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS mount failed"));
  } else {
    Serial.println(F("LittleFS mounted"));
  }

  configBegin();
  wifiBegin();
  Radio.begin();
  mqttBegin();
  rxSlotsBegin();
  webBegin();
  Serial.print(F("Free heap after startup: "));
  Serial.println(ESP.getFreeHeap());
}

void loop() {
  static uint32_t lastHealthLogMs = 0;
  wifiLoop();
  Radio.loop();
  rxSlotsLoop();
  mqttLoop();
  webLoop();

  const uint32_t now = millis();
  if (now - lastHealthLogMs >= 300000UL) {
    lastHealthLogMs = now;
    Serial.print(F("Health: heap="));
    Serial.print(ESP.getFreeHeap());
    Serial.print(F(", max_block="));
    Serial.print(ESP.getMaxFreeBlockSize());
    Serial.print(F(", fragmentation="));
    Serial.print(ESP.getHeapFragmentation());
    Serial.println(F("%"));
  }
}
