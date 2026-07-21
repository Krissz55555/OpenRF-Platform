#pragma once

#include <Arduino.h>

void mqttBegin();
void mqttLoop();
bool mqttIsConnected();
const char* mqttStateName();
String mqttBaseTopic();
int mqttLastError();
void mqttPublishStatus();

void mqttPublishDiscovery();
