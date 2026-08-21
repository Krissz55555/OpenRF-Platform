#pragma once

#include <Arduino.h>

struct RFEventMessage;

void mqttBegin();
void mqttLoop();
bool mqttIsConnected();
const char* mqttStateName();
String mqttBaseTopic();
int mqttLastError();
void mqttPublishStatus();

void mqttPublishDiscovery();

void mqttHandleRFEvent(const RFEventMessage& event);
