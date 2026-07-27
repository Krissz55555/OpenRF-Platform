#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"

AppConfig config;

static const char* CONFIG_FILE = "/config.json";

void configResetDefaults() {
  config.hostname = "OpenRF-Platform";
  config.wifiSsid = "";
  config.wifiPassword = "";
  config.mqttEnabled = false;
  config.mqttHost = "192.168.0.10";
  config.mqttPort = 1883;
  config.mqttUser = "";
  config.mqttPassword = "";
  config.homeAssistantDiscovery = true;
  config.replayCount = 1;
  config.analyzerMinRssi = -75;
  config.analyzerMinPulseCount = 20;
  config.analyzerMinDurationUs = 5000;
  config.analyzerSimilarity = 82;
  config.analyzerOccurrences = 3;
  config.analyzerShowRejected = true;
  config.analyzerFreezeCandidate = false;
  config.analyzerAlternationTolerance = 90;
  config.analyzerDeveloperMode = false;
}

void configBegin() {
  configResetDefaults();
  configLoad();
}

bool configLoad() {
  if (!LittleFS.exists(CONFIG_FILE)) {
    configSave();
    return false;
  }

  File file = LittleFS.open(CONFIG_FILE, "r");
  if (!file) {
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    configResetDefaults();
    configSave();
    return false;
  }

  config.hostname = doc["hostname"] | "OpenRF-Platform";
  config.wifiSsid = doc["wifi_ssid"] | "";
  config.wifiPassword = doc["wifi_password"] | "";
  config.mqttEnabled = doc["mqtt_enabled"] | false;
  config.mqttHost = doc["mqtt_host"] | "192.168.0.10";
  config.mqttPort = doc["mqtt_port"] | 1883;
  config.mqttUser = doc["mqtt_user"] | "";
  config.mqttPassword = doc["mqtt_password"] | "";
  config.homeAssistantDiscovery = doc["home_assistant_discovery"] | true;
  config.replayCount = doc["replay_count"] | 1;
  config.analyzerMinRssi = doc["analyzer_min_rssi"] | -75;
  config.analyzerMinPulseCount = doc["analyzer_min_pulse_count"] | 20;
  config.analyzerMinDurationUs = doc["analyzer_min_duration_us"] | 5000;
  config.analyzerSimilarity = doc["analyzer_similarity"] | 82;
  config.analyzerOccurrences = doc["analyzer_occurrences"] | 3;
  config.analyzerShowRejected = doc["analyzer_show_rejected"] | true;
  config.analyzerFreezeCandidate = doc["analyzer_freeze_candidate"] | false;
  config.analyzerAlternationTolerance = doc["analyzer_alternation_tolerance"] | 90;
  config.analyzerDeveloperMode = doc["analyzer_developer_mode"] | false;

  config.hostname.trim();
  config.wifiSsid.trim();
  config.mqttHost.trim();
  config.mqttUser.trim();

  if (config.hostname.length() == 0) {
    config.hostname = "OpenRF-Platform";
  }

  if (config.mqttHost.length() == 0) {
    config.mqttHost = "192.168.0.10";
  }

  if (config.mqttPort == 0) {
    config.mqttPort = 1883;
  }

  if (config.replayCount < 1 || config.replayCount > 10) {
    config.replayCount = 1;
  }

  if (config.analyzerMinRssi < -100 || config.analyzerMinRssi > -20) config.analyzerMinRssi = -75;
  if (config.analyzerMinPulseCount < 2 || config.analyzerMinPulseCount > 300) config.analyzerMinPulseCount = 20;
  if (config.analyzerMinDurationUs < 500 || config.analyzerMinDurationUs > 500000) config.analyzerMinDurationUs = 5000;
  if (config.analyzerSimilarity < 50 || config.analyzerSimilarity > 100) config.analyzerSimilarity = 82;
  if (config.analyzerOccurrences < 1 || config.analyzerOccurrences > 10) config.analyzerOccurrences = 3;
  if (config.analyzerAlternationTolerance < 50 || config.analyzerAlternationTolerance > 100) config.analyzerAlternationTolerance = 90;

  return true;
}

bool configSave() {
  JsonDocument doc;

  doc["hostname"] = config.hostname;
  doc["wifi_ssid"] = config.wifiSsid;
  doc["wifi_password"] = config.wifiPassword;
  doc["mqtt_enabled"] = config.mqttEnabled;
  doc["mqtt_host"] = config.mqttHost;
  doc["mqtt_port"] = config.mqttPort;
  doc["mqtt_user"] = config.mqttUser;
  doc["mqtt_password"] = config.mqttPassword;
  doc["home_assistant_discovery"] = config.homeAssistantDiscovery;
  doc["replay_count"] = config.replayCount;
  doc["analyzer_min_rssi"] = config.analyzerMinRssi;
  doc["analyzer_min_pulse_count"] = config.analyzerMinPulseCount;
  doc["analyzer_min_duration_us"] = config.analyzerMinDurationUs;
  doc["analyzer_similarity"] = config.analyzerSimilarity;
  doc["analyzer_occurrences"] = config.analyzerOccurrences;
  doc["analyzer_show_rejected"] = config.analyzerShowRejected;
  doc["analyzer_freeze_candidate"] = config.analyzerFreezeCandidate;
  doc["analyzer_alternation_tolerance"] = config.analyzerAlternationTolerance;
  doc["analyzer_developer_mode"] = config.analyzerDeveloperMode;

  File file = LittleFS.open(CONFIG_FILE, "w");
  if (!file) {
    return false;
  }

  size_t written = serializeJsonPretty(doc, file);
  file.close();

  return written > 0;
}

String configToJson() {
  JsonDocument doc;

  doc["hostname"] = config.hostname;
  doc["wifi_ssid"] = config.wifiSsid;
  doc["wifi_password_set"] = config.wifiPassword.length() > 0;
  doc["mqtt_enabled"] = config.mqttEnabled;
  doc["mqtt_host"] = config.mqttHost;
  doc["mqtt_port"] = config.mqttPort;
  doc["mqtt_user"] = config.mqttUser;
  doc["mqtt_password_set"] = config.mqttPassword.length() > 0;
  doc["home_assistant_discovery"] = config.homeAssistantDiscovery;
  doc["replay_count"] = config.replayCount;
  doc["analyzer_min_rssi"] = config.analyzerMinRssi;
  doc["analyzer_min_pulse_count"] = config.analyzerMinPulseCount;
  doc["analyzer_min_duration_us"] = config.analyzerMinDurationUs;
  doc["analyzer_similarity"] = config.analyzerSimilarity;
  doc["analyzer_occurrences"] = config.analyzerOccurrences;
  doc["analyzer_show_rejected"] = config.analyzerShowRejected;
  doc["analyzer_freeze_candidate"] = config.analyzerFreezeCandidate;
  doc["analyzer_alternation_tolerance"] = config.analyzerAlternationTolerance;
  doc["analyzer_developer_mode"] = config.analyzerDeveloperMode;

  String output;
  serializeJson(doc, output);
  return output;
}