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
  bool radio1Enabled;
  bool radio2Enabled;
  bool loraEnabled;
  // Persisted CC1101 operating frequencies. Legacy field names are kept in
  // the config schema for backward compatibility; the RF layer separately
  // exposes immutable default frequencies.
  float radio1FrequencyMhz;
  float radio2FrequencyMhz;
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