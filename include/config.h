#pragma once

#include <Arduino.h>

struct AppConfig {
  String hostname;

  String wifiSsid;
  String wifiPassword;

  bool mqttEnabled;
  String mqttHost;
  uint16_t mqttPort;
  String mqttUser;
  String mqttPassword;
  bool homeAssistantDiscovery;

  uint8_t replayCount;
};

extern AppConfig config;

void configBegin();
void configResetDefaults();

bool configLoad();
bool configSave();

String configToJson();