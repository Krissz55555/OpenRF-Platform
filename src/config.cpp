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

  String output;
  serializeJson(doc, output);
  return output;
}