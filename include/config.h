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
  uint16_t radioFrequencyMhz;
  int8_t rxSlotLearnMinRssi;

  int8_t analyzerMinRssi;
  uint16_t analyzerMinPulseCount;
  uint32_t analyzerMinDurationUs;
  uint8_t analyzerSimilarity;
  uint8_t analyzerOccurrences;
  bool analyzerShowRejected;
  bool analyzerFreezeCandidate;
  uint8_t analyzerAlternationTolerance;
  bool analyzerDeveloperMode;
};

extern AppConfig config;

void configBegin();
void configResetDefaults();

bool configLoad();
bool configSave();

String configToJson();