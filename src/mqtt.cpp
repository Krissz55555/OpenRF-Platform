#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "config.h"
#include "mqtt.h"
#include "radio.h"
#include "rxslots.h"
#include "storage.h"
#include "scratch.h"
#include "version.h"
#include "wifi.h"

namespace {
WiFiClient networkClient;
PubSubClient client(networkClient);
uint32_t lastConnectAttemptMs = 0;
uint32_t lastPublishedSequence = 0;
String baseTopic;
String clientId;
bool discoveryPending = false;
bool discoveryRescanRequested = false;
uint16_t discoveryStep = 0;
uint32_t discoveryNextStepMs = 0;

constexpr uint32_t DISCOVERY_START_DELAY_MS = 15000;
constexpr uint32_t DISCOVERY_RESCAN_DELAY_MS = 5000;
constexpr uint32_t DISCOVERY_STEP_DELAY_MS = 150;
constexpr uint32_t DISCOVERY_RETRY_DELAY_MS = 500;
constexpr uint32_t DISCOVERY_MIN_FREE_HEAP = 12000;
constexpr uint32_t DISCOVERY_MIN_MAX_BLOCK = 3000;

uint8_t pendingLearnSlot = 0;
String pendingLearnName;
bool pendingLearnActive = false;

void publishLearnState(const char* state, uint8_t slot = 0) {
  if (!client.connected()) return;
  JsonDocument doc;
  doc["state"] = state;
  if (slot > 0) doc["slot"] = slot;
  String payload;
  serializeJson(doc, payload);
  const String topic = baseTopic + "/learn/state";
  client.publish(topic.c_str(), payload.c_str(), true);
}

uint8_t findFirstEmptySlot() {
  for (uint8_t slot = 1; slot <= OPENRF_SLOT_COUNT; slot++) {
    if (!storageSlotExists(slot)) return slot;
  }
  return 0;
}

bool beginMqttLearn(uint8_t slot, const String& name) {
  if (slot < 1 || slot > OPENRF_SLOT_COUNT || pendingLearnActive) return false;
  if (!Radio.startLearning()) return false;
  pendingLearnSlot = slot;
  pendingLearnName = name.length() ? name : ("RF Slot " + String(slot));
  pendingLearnActive = true;
  publishLearnState("waiting_for_signal", slot);
  Serial.print("MQTT learn started for slot ");
  Serial.println(slot);
  return true;
}

String sanitizeTopicPart(String value) {
  value.toLowerCase();
  String out;
  out.reserve(value.length());
  for (size_t i = 0; i < value.length(); i++) {
    const char c = value[i];
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_') out += c;
    else if (c == ' ' || c == '.') out += '-';
  }
  if (out.length() == 0) out = "openrf-platform";
  return out;
}

String deviceIdentifier() {
  return "openrf_" + String(ESP.getChipId(), HEX);
}

void addDevice(JsonDocument& doc) {
  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(deviceIdentifier());
  device["name"] = config.hostname;
  device["manufacturer"] = "OpenRF";
  device["model"] = "ESP8266 CC1101 RF Platform";
  device["sw_version"] = FW_VERSION;
  device["configuration_url"] = "http://" + WiFi.localIP().toString() + "/";
}

bool publishDiscoveryDocument(const String& topic, JsonDocument& doc) {
  String payload;
  payload.reserve(900);
  serializeJson(doc, payload);
  const bool ok = client.publish(topic.c_str(), payload.c_str(), true);
  yield();
  return ok;
}

bool sendSlot(uint8_t slot) {
  SlotInfo info;
  if (!storageLoadSlot(slot, openrfScratch, OPENRF_MAX_RAW_PULSES, info)) return false;
  return Radio.sendRaw(openrfScratch, info.pulseCount, config.replayCount);
}

void callback(char* topic, byte* payload, unsigned int length) {
  (void)payload;
  (void)length;
  const String incoming(topic);

  if (incoming == baseTopic + "/learn/next") {
    const uint8_t slot = findFirstEmptySlot();
    if (slot == 0) {
      publishLearnState("no_empty_slot");
      return;
    }
    if (!beginMqttLearn(slot, "RF Slot " + String(slot))) {
      publishLearnState("busy", slot);
    }
    return;
  }

  const String prefix = baseTopic + "/slot/";
  if (!incoming.startsWith(prefix)) return;

  const int slash = incoming.indexOf('/', prefix.length());
  if (slash < 0) return;
  const int slotNumber = incoming.substring(prefix.length(), slash).toInt();
  if (slotNumber < 1 || slotNumber > OPENRF_SLOT_COUNT) return;
  const uint8_t slot = static_cast<uint8_t>(slotNumber);
  const String action = incoming.substring(slash + 1);
  const String stateTopic = baseTopic + "/slot/" + String(slot) + "/state";

  if (action == "send") {
    const bool ok = sendSlot(slot);
    client.publish(stateTopic.c_str(), ok ? "sent" : "error", true);
    Serial.print("MQTT slot command: ");
    Serial.print(slot);
    Serial.println(ok ? " sent" : " failed");
    return;
  }

  if (action == "relearn") {
    const SlotInfo info = storageGetSlotInfo(slot);
    if (!info.used) {
      client.publish(stateTopic.c_str(), "empty", true);
      return;
    }
    if (!beginMqttLearn(slot, info.name)) {
      client.publish(stateTopic.c_str(), "learn_busy", true);
    } else {
      client.publish(stateTopic.c_str(), "waiting_for_signal", true);
    }
    return;
  }

  if (action == "delete") {
    const bool ok = storageDeleteSlot(slot);
    client.publish(stateTopic.c_str(), ok ? "deleted" : "error", true);
    if (ok && config.homeAssistantDiscovery) mqttPublishDiscovery();
    mqttPublishStatus();
    Serial.print("MQTT slot delete: ");
    Serial.print(slot);
    Serial.println(ok ? " deleted" : " failed");
  }
}

void connectIfNeeded() {
  if (!config.mqttEnabled || client.connected() || !wifiStationConnected()) return;
  if (millis() - lastConnectAttemptMs < 5000) return;
  lastConnectAttemptMs = millis();

  const String availability = baseTopic + "/availability";
  bool connected;
  if (config.mqttUser.length() > 0) {
    connected = client.connect(clientId.c_str(), config.mqttUser.c_str(), config.mqttPassword.c_str(),
                               availability.c_str(), 0, true, "offline");
  } else {
    connected = client.connect(clientId.c_str(), availability.c_str(), 0, true, "offline");
  }

  if (!connected) {
    Serial.print("MQTT connection failed, state: ");
    Serial.println(client.state());
    return;
  }

  client.publish(availability.c_str(), "online", true);
  client.subscribe((baseTopic + "/slot/+/send").c_str());
  client.subscribe((baseTopic + "/slot/+/relearn").c_str());
  client.subscribe((baseTopic + "/slot/+/delete").c_str());
  client.subscribe((baseTopic + "/learn/next").c_str());
  mqttPublishStatus();
  if (config.homeAssistantDiscovery) mqttPublishDiscovery();
  Serial.print("MQTT connected, base topic: ");
  Serial.println(baseTopic);
}

void publishRxIfNew() {
  if (!client.connected()) return;
  const RawFrameInfo info = Radio.getLastFrameInfo();
  if (!info.available || info.sequence == lastPublishedSequence) return;
  lastPublishedSequence = info.sequence;

  JsonDocument doc;
  doc["sequence"] = info.sequence;
  doc["pulse_count"] = info.pulseCount;
  doc["duration_us"] = info.durationUs;
  doc["rssi_dbm"] = info.rssiDbm;
  doc["frequency_mhz"] = Radio.getFrequency();
  String payload;
  serializeJson(doc, payload);
  const String topic = baseTopic + "/rx";
  client.publish(topic.c_str(), payload.c_str(), false);
}


void processPendingLearn() {
  if (!pendingLearnActive) return;
  const LearnCaptureInfo capture = Radio.getLearnCaptureInfo();
  if (capture.state != LearnState::PREVIEW_READY || !capture.available) return;

  const uint16_t count = Radio.copyLearnRaw(openrfScratch, OPENRF_MAX_RAW_PULSES);
  uint32_t fingerprint = 0;
  const bool ok = count > 0 && storageSaveSlot(
      pendingLearnSlot, pendingLearnName, Radio.getFrequency(), openrfScratch,
      count, capture.durationUs, &fingerprint);

  const uint8_t completedSlot = pendingLearnSlot;
  pendingLearnActive = false;
  pendingLearnSlot = 0;
  pendingLearnName = "";
  Radio.discardLearnCapture();

  const String stateTopic = baseTopic + "/slot/" + String(completedSlot) + "/state";
  client.publish(stateTopic.c_str(), ok ? "learned" : "learn_error", true);
  publishLearnState(ok ? "saved" : "save_error", completedSlot);

  if (ok) {
    mqttPublishStatus();
    if (config.homeAssistantDiscovery) mqttPublishDiscovery();
    Serial.print("MQTT learn saved slot ");
    Serial.print(completedSlot);
    Serial.print(", fingerprint ");
    Serial.println(fingerprint, HEX);
  } else {
    Serial.print("MQTT learn save failed for slot ");
    Serial.println(completedSlot);
  }
}

void processDiscovery() {
  if (!discoveryPending || !config.homeAssistantDiscovery || !client.connected()) return;

  const uint32_t now = millis();
  if (static_cast<int32_t>(now - discoveryNextStepMs) < 0) return;

  // Home Assistant discovery produces many temporary String/JSON allocations.
  // On ESP8266, run only one small discovery item per loop iteration and wait
  // until both total heap and the largest contiguous block are healthy.
  if (ESP.getFreeHeap() < DISCOVERY_MIN_FREE_HEAP ||
      ESP.getMaxFreeBlockSize() < DISCOVERY_MIN_MAX_BLOCK) {
    discoveryNextStepMs = now + DISCOVERY_RETRY_DELAY_MS;
    return;
  }

  const String id = deviceIdentifier();
  const String availability = baseTopic + "/availability";

  // Fixed bridge entities: steps 0..4.
  if (discoveryStep == 0) {
    JsonDocument doc;
    doc["name"] = "Learn next empty slot";
    doc["unique_id"] = id + "_learn_next";
    doc["command_topic"] = baseTopic + "/learn/next";
    doc["payload_press"] = "PRESS";
    doc["availability_topic"] = availability;
    doc["icon"] = "mdi:remote-plus";
    addDevice(doc);
    publishDiscoveryDocument("homeassistant/button/" + id + "/learn_next/config", doc);
  } else if (discoveryStep == 1) {
    JsonDocument doc;
    doc["name"] = "Learn state";
    doc["unique_id"] = id + "_learn_state";
    doc["state_topic"] = baseTopic + "/learn/state";
    doc["value_template"] = "{{ value_json.state }}";
    doc["availability_topic"] = availability;
    doc["entity_category"] = "diagnostic";
    doc["icon"] = "mdi:school";
    addDevice(doc);
    publishDiscoveryDocument("homeassistant/sensor/" + id + "/learn_state/config", doc);
  } else if (discoveryStep == 2) {
    JsonDocument doc;
    doc["name"] = "Status";
    doc["unique_id"] = id + "_status";
    doc["state_topic"] = baseTopic + "/availability";
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["availability_topic"] = availability;
    doc["entity_category"] = "diagnostic";
    doc["icon"] = "mdi:radio-tower";
    addDevice(doc);
    publishDiscoveryDocument("homeassistant/sensor/" + id + "/status/config", doc);
  } else if (discoveryStep == 3) {
    JsonDocument doc;
    doc["name"] = "Last RF pulse count";
    doc["unique_id"] = id + "_rx_pulses";
    doc["state_topic"] = baseTopic + "/rx";
    doc["value_template"] = "{{ value_json.pulse_count }}";
    doc["availability_topic"] = availability;
    doc["icon"] = "mdi:pulse";
    addDevice(doc);
    publishDiscoveryDocument("homeassistant/sensor/" + id + "/rx_pulses/config", doc);
  } else if (discoveryStep == 4) {
    JsonDocument doc;
    doc["name"] = "Last RF RSSI";
    doc["unique_id"] = id + "_rx_rssi";
    doc["state_topic"] = baseTopic + "/rx";
    doc["value_template"] = "{{ value_json.rssi_dbm }}";
    doc["unit_of_measurement"] = "dBm";
    doc["device_class"] = "signal_strength";
    doc["state_class"] = "measurement";
    doc["availability_topic"] = availability;
    addDevice(doc);
    publishDiscoveryDocument("homeassistant/sensor/" + id + "/rx_rssi/config", doc);
  } else if (discoveryStep < 5 + OPENRF_SLOT_COUNT * 3U) {
    // TX slots: three discovery documents per slot.
    const uint16_t relative = discoveryStep - 5;
    const uint8_t slot = static_cast<uint8_t>(relative / 3U) + 1;
    const uint8_t item = static_cast<uint8_t>(relative % 3U);
    const SlotInfo info = storageGetSlotInfo(slot);
    const String slotBase = "homeassistant/button/" + id + "/slot_" + String(slot);

    if (!info.used) {
      const char* suffix = item == 0 ? "/config" : (item == 1 ? "_relearn/config" : "_delete/config");
      client.publish((slotBase + suffix).c_str(), "", true);
    } else {
      JsonDocument doc;
      if (item == 0) {
        doc["name"] = info.name;
        doc["unique_id"] = id + "_slot_" + String(slot);
        doc["command_topic"] = baseTopic + "/slot/" + String(slot) + "/send";
        doc["icon"] = "mdi:remote";
      } else if (item == 1) {
        doc["name"] = info.name + " Relearn";
        doc["unique_id"] = id + "_slot_" + String(slot) + "_relearn";
        doc["command_topic"] = baseTopic + "/slot/" + String(slot) + "/relearn";
        doc["icon"] = "mdi:refresh";
      } else {
        doc["name"] = info.name + " Delete";
        doc["unique_id"] = id + "_slot_" + String(slot) + "_delete";
        doc["command_topic"] = baseTopic + "/slot/" + String(slot) + "/delete";
        doc["icon"] = "mdi:delete";
      }
      doc["payload_press"] = "PRESS";
      doc["availability_topic"] = availability;
      addDevice(doc);
      const String topic = item == 0 ? slotBase + "/config"
                                     : slotBase + (item == 1 ? "_relearn/config" : "_delete/config");
      publishDiscoveryDocument(topic, doc);
    }
  } else {
    // RX slots: trigger + binary sensor per slot.
    const uint16_t rxStart = 5 + OPENRF_SLOT_COUNT * 3U;
    const uint16_t relative = discoveryStep - rxStart;
    const uint8_t slot = static_cast<uint8_t>(relative / 2U) + 1;
    const uint8_t item = static_cast<uint8_t>(relative % 2U);
    const RxSlotInfo info = rxSlotGetInfo(slot);
    const String triggerTopic = "homeassistant/device_automation/" + id + "/rx_slot_" + String(slot) + "/config";
    const String sensorTopic = "homeassistant/binary_sensor/" + id + "/rx_slot_" + String(slot) + "/config";

    if (!info.used || !info.enabled) {
      client.publish((item == 0 ? triggerTopic : sensorTopic).c_str(), "", true);
    } else if (item == 0) {
      JsonDocument doc;
      doc["automation_type"] = "trigger";
      doc["type"] = "button_short_press";
      doc["subtype"] = "rx_slot_" + String(slot);
      doc["topic"] = baseTopic + "/rxslot/" + String(slot) + "/event";
      doc["value_template"] = "{{ value_json.event }}";
      doc["payload"] = "pressed";
      addDevice(doc);
      publishDiscoveryDocument(triggerTopic, doc);
    } else {
      JsonDocument doc;
      doc["name"] = info.name;
      doc["unique_id"] = id + "_rx_slot_" + String(slot);
      doc["state_topic"] = baseTopic + "/rxslot/" + String(slot) + "/event";
      doc["value_template"] = "{{ value_json.event }}";
      doc["payload_on"] = "pressed";
      doc["off_delay"] = 1;
      doc["availability_topic"] = availability;
      doc["icon"] = "mdi:remote";
      addDevice(doc);
      publishDiscoveryDocument(sensorTopic, doc);
    }
  }

  discoveryStep++;
  const uint16_t totalSteps = 5 + OPENRF_SLOT_COUNT * 3U + OPENRF_RX_SLOT_COUNT * 2U;
  if (discoveryStep >= totalSteps) {
    discoveryPending = false;
    discoveryStep = 0;
    Serial.print(F("Home Assistant discovery published gradually, TX slots: "));
    Serial.print(storageCountUsedSlots());
    Serial.print(F(", RX slots: "));
    Serial.println(rxSlotCountUsed());

    // Changes requested while a discovery pass was running are coalesced into
    // exactly one additional pass. This avoids restarting the sequence midway,
    // which could leave stale or partially updated Home Assistant entities.
    if (discoveryRescanRequested) {
      discoveryRescanRequested = false;
      discoveryPending = true;
      discoveryNextStepMs = now + DISCOVERY_RESCAN_DELAY_MS;
      Serial.println(F("Home Assistant discovery rescan queued"));
    }
    return;
  }

  discoveryNextStepMs = now + DISCOVERY_STEP_DELAY_MS;
}

}  // namespace

void mqttPublishRxSlotEvent(uint8_t slot, const RxSlotInfo& info) {
  if (!client.connected()) return;
  JsonDocument doc;
  // Keep the legacy "pressed" payload for existing Home Assistant device
  // automation triggers, and expose the normalized Kinetic action separately.
  doc["event"] = "pressed";
  doc["action"] = info.code.length() ? info.code : "PRESS";
  doc["slot"] = slot;
  doc["name"] = info.name;
  doc["protocol"] = info.protocol;
  doc["device_id"] = info.deviceId;
  doc["command"] = info.command;
  doc["symbols"] = info.symbolCount;
  doc["code"] = info.code;
  doc["quality"] = info.lastQuality;
  doc["rssi_dbm"] = info.lastRssi;
  String payload; serializeJson(doc, payload);
  const String topic = baseTopic + "/rxslot/" + String(slot) + "/event";
  client.publish(topic.c_str(), payload.c_str(), false);
}

void mqttBegin() {
  baseTopic = "openrf/" + sanitizeTopicPart(config.hostname);
  clientId = sanitizeTopicPart(config.hostname) + "-" + String(ESP.getChipId(), HEX);
  client.setServer(config.mqttHost.c_str(), config.mqttPort);
  client.setCallback(callback);
  client.setBufferSize(1024);
  lastConnectAttemptMs = millis() - 5000;

  if (config.mqttEnabled) {
    Serial.print("MQTT enabled, broker: ");
    Serial.print(config.mqttHost);
    Serial.print(':');
    Serial.println(config.mqttPort);
  } else {
    Serial.println("MQTT disabled");
  }
}

void mqttLoop() {
  if (!config.mqttEnabled) return;
  connectIfNeeded();
  if (!client.connected()) return;
  client.loop();
  processPendingLearn();
  publishRxIfNew();
  processDiscovery();
}

bool mqttIsConnected() { return client.connected(); }

const char* mqttStateName() {
  if (!config.mqttEnabled) return "disabled";
  if (!wifiStationConnected()) return "waiting_for_wifi";
  return client.connected() ? "connected" : "disconnected";
}

String mqttBaseTopic() { return baseTopic; }
int mqttLastError() { return client.state(); }

void mqttPublishStatus() {
  if (!client.connected()) return;
  JsonDocument doc;
  doc["online"] = true;
  doc["ip"] = WiFi.localIP().toString();
  doc["radio"] = Radio.getModeName();
  doc["frequency_mhz"] = Radio.getFrequency();
  doc["slots_used"] = storageCountUsedSlots();
  doc["rx_slots_used"] = rxSlotCountUsed();
  String payload;
  serializeJson(doc, payload);
  const String topic = baseTopic + "/status";
  client.publish(topic.c_str(), payload.c_str(), true);
}

void mqttPublishDiscovery() {
  if (!config.homeAssistantDiscovery) {
    discoveryPending = false;
    discoveryRescanRequested = false;
    discoveryStep = 0;
    return;
  }

  // Never restart a discovery pass that is already in progress. Repeated
  // requests from Learn, slot changes or web callbacks are merged into one
  // follow-up pass, so Home Assistant receives a complete and ordered set.
  if (discoveryPending) {
    discoveryRescanRequested = true;
    return;
  }

  // Delay the first item after Wi-Fi/MQTT connection or a configuration write.
  // This separates discovery JSON/TCP allocations from startup and LittleFS IO.
  discoveryPending = true;
  discoveryRescanRequested = false;
  discoveryStep = 0;
  discoveryNextStepMs = millis() + DISCOVERY_START_DELAY_MS;
}
