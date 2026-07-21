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
  if (config.homeAssistantDiscovery) { discoveryPending = true; mqttPublishDiscovery(); }
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
}  // namespace

void mqttPublishRxSlotEvent(uint8_t slot, const RxSlotInfo& info) {
  if (!client.connected()) return;
  JsonDocument doc;
  doc["event"] = "pressed";
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
  if (discoveryPending && config.homeAssistantDiscovery) mqttPublishDiscovery();
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
  if (!config.homeAssistantDiscovery) { discoveryPending = false; return; }
  if (!client.connected()) { discoveryPending = true; return; }
  discoveryPending = true;

  const String id = deviceIdentifier();
  const String availability = baseTopic + "/availability";


  // Learn the next valid RF signal into the first empty slot.
  {
    JsonDocument doc;
    doc["name"] = "Learn next empty slot";
    doc["unique_id"] = id + "_learn_next";
    doc["command_topic"] = baseTopic + "/learn/next";
    doc["payload_press"] = "PRESS";
    doc["availability_topic"] = availability;
    doc["icon"] = "mdi:remote-plus";
    addDevice(doc);
    publishDiscoveryDocument("homeassistant/button/" + id + "/learn_next/config", doc);
  }
  {
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
  }

  // Diagnostic bridge status sensor.
  {
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
  }

  // Last received RAW frame metrics.
  {
    JsonDocument doc;
    doc["name"] = "Last RF pulse count";
    doc["unique_id"] = id + "_rx_pulses";
    doc["state_topic"] = baseTopic + "/rx";
    doc["value_template"] = "{{ value_json.pulse_count }}";
    doc["availability_topic"] = availability;
    doc["icon"] = "mdi:pulse";
    addDevice(doc);
    publishDiscoveryDocument("homeassistant/sensor/" + id + "/rx_pulses/config", doc);
  }
  {
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
  }

  // One Home Assistant button for every saved slot. Empty slots remove any
  // previously retained discovery entry, so deleted slots disappear from HA.
  for (uint8_t slot = 1; slot <= OPENRF_SLOT_COUNT; slot++) {
    const SlotInfo info = storageGetSlotInfo(slot);
    const String discoveryTopic = "homeassistant/button/" + id + "/slot_" + String(slot) + "/config";

    if (!info.used) {
      client.publish(discoveryTopic.c_str(), "", true);
      client.publish(("homeassistant/button/" + id + "/slot_" + String(slot) + "_relearn/config").c_str(), "", true);
      client.publish(("homeassistant/button/" + id + "/slot_" + String(slot) + "_delete/config").c_str(), "", true);
      yield();
      continue;
    }

    {
      JsonDocument doc;
      doc["name"] = info.name;
      doc["unique_id"] = id + "_slot_" + String(slot);
      doc["command_topic"] = baseTopic + "/slot/" + String(slot) + "/send";
      doc["payload_press"] = "PRESS";
      doc["availability_topic"] = availability;
      doc["icon"] = "mdi:remote";
      addDevice(doc);
      publishDiscoveryDocument(discoveryTopic, doc);
    }
    {
      JsonDocument doc;
      doc["name"] = info.name + " Relearn";
      doc["unique_id"] = id + "_slot_" + String(slot) + "_relearn";
      doc["command_topic"] = baseTopic + "/slot/" + String(slot) + "/relearn";
      doc["payload_press"] = "PRESS";
      doc["availability_topic"] = availability;
      doc["icon"] = "mdi:refresh";
      addDevice(doc);
      publishDiscoveryDocument("homeassistant/button/" + id + "/slot_" + String(slot) + "_relearn/config", doc);
    }
    {
      JsonDocument doc;
      doc["name"] = info.name + " Delete";
      doc["unique_id"] = id + "_slot_" + String(slot) + "_delete";
      doc["command_topic"] = baseTopic + "/slot/" + String(slot) + "/delete";
      doc["payload_press"] = "PRESS";
      doc["availability_topic"] = availability;
      doc["icon"] = "mdi:delete";
      addDevice(doc);
      publishDiscoveryDocument("homeassistant/button/" + id + "/slot_" + String(slot) + "_delete/config", doc);
    }
  }

  // RX slots are exposed as Home Assistant device automation triggers.
  for (uint8_t slot = 1; slot <= OPENRF_RX_SLOT_COUNT; slot++) {
    const RxSlotInfo info = rxSlotGetInfo(slot);
    const String topic = "homeassistant/device_automation/" + id + "/rx_slot_" + String(slot) + "/config";
    if (!info.used || !info.enabled) { client.publish(topic.c_str(), "", true); client.publish(("homeassistant/binary_sensor/" + id + "/rx_slot_" + String(slot) + "/config").c_str(), "", true); yield(); continue; }
    JsonDocument doc;
    doc["automation_type"] = "trigger";
    doc["type"] = "button_short_press";
    doc["subtype"] = "rx_slot_" + String(slot);
    doc["topic"] = baseTopic + "/rxslot/" + String(slot) + "/event";
    doc["value_template"] = "{{ value_json.event }}";
    doc["payload"] = "pressed";
    addDevice(doc);
    publishDiscoveryDocument(topic, doc);

    // A visible entity as well as a device automation trigger. It turns on
    // briefly for each matched remote-button press.
    JsonDocument sensor;
    sensor["name"] = info.name;
    sensor["unique_id"] = id + "_rx_slot_" + String(slot);
    sensor["state_topic"] = baseTopic + "/rxslot/" + String(slot) + "/event";
    sensor["value_template"] = "{{ value_json.event }}";
    sensor["payload_on"] = "pressed";
    sensor["off_delay"] = 1;
    sensor["availability_topic"] = availability;
    sensor["icon"] = "mdi:remote";
    addDevice(sensor);
    publishDiscoveryDocument("homeassistant/binary_sensor/" + id + "/rx_slot_" + String(slot) + "/config", sensor);
  }

  discoveryPending = false;
  Serial.print("Home Assistant discovery published, TX slots: ");
  Serial.print(storageCountUsedSlots());
  Serial.print(", RX slots: ");
  Serial.println(rxSlotCountUsed());
}
