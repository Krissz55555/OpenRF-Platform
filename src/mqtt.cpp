#include <Arduino.h>
#include "dualcore.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "config.h"
#include "mqtt.h"
#include "radio.h"
#include "rxslots.h"
#include "storage.h"
#include "raw_slot_matcher.h"
#include "scratch.h"
#include "version.h"
#include "openrf_wifi.h"
#include "platform_compat.h"

namespace {

String v2ActionCodeHex(uint64_t code) {
  char buffer[19];
  const uint32_t hi = static_cast<uint32_t>(code >> 32);
  const uint32_t lo = static_cast<uint32_t>(code);
  if (hi) snprintf(buffer, sizeof(buffer), "%08X%08X", hi, lo);
  else snprintf(buffer, sizeof(buffer), "%08X", lo);
  String out(buffer);
  while (out.length() > 1 && out[0] == '0') out.remove(0, 1);
  return out;
}

const char* v2ActionProtocolName(uint16_t protocolId) {
  switch (protocolId) {
    case 1: return "EV1527/Princeton";
    case 2: return "PT2262/Tri-State";
    case 4: return "HT12E";
    default: return "Unknown";
  }
}

WiFiClient networkClient;
PubSubClient client(networkClient);
uint32_t lastConnectAttemptMs = 0;
String baseTopic;
String clientId;
bool discoveryPending = false;
bool discoveryRescanRequested = false;
uint16_t discoveryStep = 0;
uint32_t discoveryNextStepMs = 0;

constexpr uint32_t DISCOVERY_START_DELAY_MS = 15000;
constexpr uint32_t DISCOVERY_RESCAN_DELAY_MS = 5000;
constexpr uint32_t DISCOVERY_STEP_DELAY_MS = 150;

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
  if (!rfCommandStartLearn()) return false;
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
  return "openrf_" + openrfChipIdHex();
}

void addDevice(JsonDocument& doc) {
  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(deviceIdentifier());
  device["name"] = config.hostname;
  device["manufacturer"] = "OpenRF";
  device["model"] = "ESP32-S3 CC1101 RF Platform";
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
  const uint8_t radioId = (info.radioId == 1 || info.radioId == 2)
                              ? info.radioId
                              : (info.frequencyMHz >= 700.0F ? 2 : 1);
  return rfCommandSendRawTuned(openrfScratch, info.pulseCount, config.replayCount,
                               radioId, info.frequencyMHz);
}

void publishRxSlotSendState(uint8_t slot, const char* state, const RxSlotInfo* info = nullptr) {
  if (!client.connected()) return;
  JsonDocument doc;
  doc["state"] = state;
  doc["slot"] = slot;
  if (info != nullptr) {
    doc["name"] = info->name;
    doc["protocol"] = info->protocol;
    doc["radio_id"] = info->radioId;
    doc["frequency_mhz"] = serialized(String(info->frequencyMHz, 4));
  }
  String payload;
  serializeJson(doc, payload);
  const String topic = baseTopic + "/rxslot/" + String(slot) + "/send/state";
  client.publish(topic.c_str(), payload.c_str(), false);
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

  const String rxPrefix = baseTopic + "/rxslot/";
  if (incoming.startsWith(rxPrefix)) {
    const int slash = incoming.indexOf('/', rxPrefix.length());
    if (slash < 0) return;
    const int slotNumber = incoming.substring(rxPrefix.length(), slash).toInt();
    if (slotNumber < 1 || slotNumber > OPENRF_RX_SLOT_COUNT) return;
    const uint8_t slot = static_cast<uint8_t>(slotNumber);
    const String action = incoming.substring(slash + 1);
    if (action != "send") return;

    const RxSlotInfo info = rxSlotGetInfo(slot);
    if (!info.used) {
      publishRxSlotSendState(slot, "empty", &info);
      return;
    }
    if (!info.sendSupported) {
      publishRxSlotSendState(slot, "unsupported", &info);
      return;
    }

    // Step 25: MQTT and Home Assistant use the exact same central decoded
    // RX Slot TX path as the WebUI. rxSlotSend() owns protocol encoding,
    // temporary slot-frequency tuning, TX and restoration of Operating RF.
    const bool ok = rxSlotSend(slot, config.replayCount);
    publishRxSlotSendState(slot, ok ? "sent" : "error", &info);
    Serial.print(F("MQTT RX slot command: "));
    Serial.print(slot);
    Serial.println(ok ? F(" sent") : F(" failed"));
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
    if (ok) rawSlotMatcherClear(slot);
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
  client.subscribe((baseTopic + "/rxslot/+/send").c_str());
  client.subscribe((baseTopic + "/learn/next").c_str());
  mqttPublishStatus();
  if (config.homeAssistantDiscovery) mqttPublishDiscovery();
  Serial.print("MQTT connected, base topic: ");
  Serial.println(baseTopic);
}

void mqttHandleRFEventImpl(const RFEventMessage& event) {
  if (event.type == RFEventType::RX_FRAME) {
    if (!config.mqttEnabled || !client.connected()) return;

    JsonDocument doc;
    doc["sequence"] = event.sequence;
    doc["pulse_count"] = event.pulseCount;
    doc["duration_us"] = event.durationUs;
    doc["rssi_dbm"] = event.rssiDbm;
    doc["frequency_mhz"] = event.frequencyMhz;
    doc["radio_id"] = event.radioId;

    String payload;
    serializeJson(doc, payload);
    const String topic = baseTopic + "/rx";
    client.publish(topic.c_str(), payload.c_str(), false);
    return;
  }

  if (event.type == RFEventType::LEARN_PREVIEW) {
    if (!pendingLearnActive || event.pulseCount == 0) return;

    uint32_t fingerprint = 0;
    const bool ok = storageSaveSlot(
        pendingLearnSlot, pendingLearnName, event.frequencyMhz, event.radioId,
        event.pulses, event.pulseCount, event.durationUs, &fingerprint);

    const uint8_t completedSlot = pendingLearnSlot;
    if (ok) rawSlotMatcherReload(completedSlot);
    pendingLearnActive = false;
    pendingLearnSlot = 0;
    pendingLearnName = "";
    rfCommandDiscardLearn();

    if (client.connected()) {
      const String stateTopic =
          baseTopic + "/slot/" + String(completedSlot) + "/state";
      client.publish(stateTopic.c_str(),
                     ok ? "learned" : "learn_error", true);
      publishLearnState(ok ? "saved" : "save_error", completedSlot);
    }

    if (ok) {
      mqttPublishStatus();
      if (config.homeAssistantDiscovery) mqttPublishDiscovery();
      Serial.print("MQTT learn saved slot ");
      Serial.print(completedSlot);
      Serial.print(", R"); Serial.print(event.radioId);
      Serial.print(" @ "); Serial.print(event.frequencyMhz, 4);
      Serial.print(" MHz, fingerprint ");
      Serial.println(fingerprint, HEX);
    } else {
      Serial.print("MQTT learn save failed for slot ");
      Serial.println(completedSlot);
    }
    return;
  }

  if (event.type == RFEventType::RADIO_ERROR) {
    Serial.print(F("Radio event error, code: "));
    Serial.println(event.errorCode);
  }
}

void processDiscovery() {
  if (!discoveryPending || !config.homeAssistantDiscovery || !client.connected()) return;

  const uint32_t now = millis();
  if (static_cast<int32_t>(now - discoveryNextStepMs) < 0) return;

  // ESP32-S3: Discovery is still paced to avoid flooding MQTT, but it is no
  // longer suspended based on ESP8266 heap / contiguous-block thresholds.

  const String id = deviceIdentifier();
  const String availability = baseTopic + "/availability";

  // Fixed bridge entities: steps 0..2. Steps 3..4 remove the legacy raw-frame
  // diagnostic sensors. The /rx MQTT stream remains available, but Home
  // Assistant receives only actionable protocol, RX Slot and Learned RAW
  // events instead of recording every captured RF frame.
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
    const String topic = "homeassistant/sensor/" + id + "/rx_pulses/config";
    client.publish(topic.c_str(), "", true);
  } else if (discoveryStep == 4) {
    const String topic = "homeassistant/sensor/" + id + "/rx_rssi/config";
    client.publish(topic.c_str(), "", true);
  } else if (discoveryStep < 5 + OPENRF_SLOT_COUNT * 5U) {
    // Step 40: each RAW RF slot is now bidirectional. Existing Send/Relearn/
    // Delete buttons remain, and two receive-side discovery documents expose
    // the same learned RAW identity as a trigger + one-second binary sensor.
    const uint16_t relative = discoveryStep - 5;
    const uint8_t slot = static_cast<uint8_t>(relative / 5U) + 1;
    const uint8_t item = static_cast<uint8_t>(relative % 5U);
    const SlotInfo info = storageGetSlotInfo(slot);
    const String buttonBase = "homeassistant/button/" + id + "/slot_" + String(slot);
    const String triggerTopic = "homeassistant/device_automation/" + id + "/raw_slot_" + String(slot) + "/config";
    const String sensorTopic = "homeassistant/binary_sensor/" + id + "/raw_slot_" + String(slot) + "/config";

    if (item <= 2U) {
      const String topic = item == 0U ? buttonBase + "/config"
                         : item == 1U ? buttonBase + "_relearn/config"
                                      : buttonBase + "_delete/config";
      if (!info.used) {
        client.publish(topic.c_str(), "", true);
      } else {
        JsonDocument doc;
        if (item == 0U) {
          doc["name"] = info.name;
          doc["unique_id"] = id + "_slot_" + String(slot);
          doc["command_topic"] = baseTopic + "/slot/" + String(slot) + "/send";
          doc["icon"] = "mdi:remote";
        } else if (item == 1U) {
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
        publishDiscoveryDocument(topic, doc);
      }
    } else if (item == 3U) {
      if (!info.used) {
        client.publish(triggerTopic.c_str(), "", true);
      } else {
        JsonDocument doc;
        doc["automation_type"] = "trigger";
        doc["type"] = "button_short_press";
        doc["subtype"] = "raw_slot_" + String(slot);
        doc["topic"] = baseTopic + "/slot/" + String(slot) + "/event";
        doc["value_template"] = "{{ value_json.event }}";
        doc["payload"] = "matched";
        addDevice(doc);
        publishDiscoveryDocument(triggerTopic, doc);
      }
    } else {
      if (!info.used) {
        client.publish(sensorTopic.c_str(), "", true);
      } else {
        JsonDocument doc;
        doc["name"] = info.name + " RX";
        doc["unique_id"] = id + "_raw_slot_" + String(slot);
        doc["state_topic"] = baseTopic + "/slot/" + String(slot) + "/event";
        doc["value_template"] = "{{ value_json.event }}";
        doc["payload_on"] = "matched";
        doc["off_delay"] = 1;
        doc["availability_topic"] = availability;
        doc["icon"] = "mdi:radio-tower";
        addDevice(doc);
        publishDiscoveryDocument(sensorTopic, doc);
      }
    }
  } else {
    // Protocol RX slots keep their receive trigger + binary sensor + optional
    // V2-native Send button.
    const uint16_t rxStart = 5 + OPENRF_SLOT_COUNT * 5U;
    const uint16_t relative = discoveryStep - rxStart;
    const uint8_t slot = static_cast<uint8_t>(relative / 3U) + 1;
    const uint8_t item = static_cast<uint8_t>(relative % 3U);
    const RxSlotInfo info = rxSlotGetInfo(slot);
    const String triggerTopic = "homeassistant/device_automation/" + id + "/rx_slot_" + String(slot) + "/config";
    const String sensorTopic = "homeassistant/binary_sensor/" + id + "/rx_slot_" + String(slot) + "/config";
    const String sendTopic = "homeassistant/button/" + id + "/rx_slot_" + String(slot) + "_send/config";

    if (item == 0) {
      if (!info.used || !info.enabled) {
        client.publish(triggerTopic.c_str(), "", true);
      } else {
        JsonDocument doc;
        doc["automation_type"] = "trigger";
        doc["type"] = "button_short_press";
        doc["subtype"] = "rx_slot_" + String(slot);
        doc["topic"] = baseTopic + "/rxslot/" + String(slot) + "/event";
        doc["value_template"] = "{{ value_json.event }}";
        doc["payload"] = "pressed";
        addDevice(doc);
        publishDiscoveryDocument(triggerTopic, doc);
      }
    } else if (item == 1) {
      if (!info.used || !info.enabled) {
        client.publish(sensorTopic.c_str(), "", true);
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
    } else {
      if (!info.used || !info.sendSupported) {
        client.publish(sendTopic.c_str(), "", true);
      } else {
        JsonDocument doc;
        doc["name"] = info.name + " Send";
        doc["unique_id"] = id + "_rx_slot_" + String(slot) + "_send";
        doc["command_topic"] = baseTopic + "/rxslot/" + String(slot) + "/send";
        doc["payload_press"] = "PRESS";
        doc["availability_topic"] = availability;
        doc["icon"] = "mdi:remote";
        addDevice(doc);
        publishDiscoveryDocument(sendTopic, doc);
      }
    }
  }

  discoveryStep++;
  const uint16_t totalSteps = 5 + OPENRF_SLOT_COUNT * 5U + OPENRF_RX_SLOT_COUNT * 3U;
  if (discoveryStep >= totalSteps) {
    discoveryPending = false;
    discoveryStep = 0;
    Serial.print(F("Home Assistant discovery published gradually, bidirectional RAW slots: "));
    Serial.print(storageCountUsedSlots());
    Serial.print(F(", RX slots: "));
    Serial.print(rxSlotCountUsed());
    Serial.println(F(" (receive + supported Send discovery)"));

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

void mqttHandleRFEvent(const RFEventMessage& event) {
  mqttHandleRFEventImpl(event);
}

void mqttHandleV2Action(const RFEventMessage& event) {
  if (!event.v2Action.available || !config.mqttEnabled || !client.connected()) {
    return;
  }

  JsonDocument doc;
  doc["sequence"] = event.sequence;
  doc["protocol"] = v2ActionProtocolName(event.v2Action.protocolId);
  doc["code"] = v2ActionCodeHex(event.v2Action.code);
  doc["symbols"] = event.v2Action.symbolCount;
  doc["repeats"] = event.v2Action.repeats;
  doc["radio_id"] = event.radioId;
  doc["frequency_mhz"] = event.frequencyMhz;
  doc["rssi_dbm"] = event.rssiDbm;
  doc["timestamp_ms"] = event.timestampMs;

  String payload;
  serializeJson(doc, payload);
  const String topic = baseTopic + "/v2/event";
  client.publish(topic.c_str(), payload.c_str(), false);
}


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
  doc["radio_id"] = info.radioId;
  doc["frequency_mhz"] = serialized(String(info.frequencyMHz, 4));
  doc["quality"] = info.lastQuality;
  doc["rssi_dbm"] = info.lastRssi;
  String payload; serializeJson(doc, payload);
  const String topic = baseTopic + "/rxslot/" + String(slot) + "/event";
  client.publish(topic.c_str(), payload.c_str(), false);
}


void mqttPublishRawSlotEvent(uint8_t slot, const SlotInfo& info,
                             const RawSlotMatchStats& stats) {
  if (!client.connected() || !info.used) return;
  JsonDocument doc;
  doc["event"] = "matched";
  doc["slot"] = slot;
  doc["name"] = info.name;
  doc["source"] = "learned_raw";
  doc["radio_id"] = info.radioId;
  doc["frequency_mhz"] = serialized(String(info.frequencyMHz, 4));
  doc["pulse_count"] = info.pulseCount;
  doc["fingerprint"] = info.fingerprint;
  doc["similarity"] = stats.lastSimilarity;
  doc["rssi_dbm"] = stats.lastRssi;
  doc["match_count"] = stats.matchCount;
  String payload;
  serializeJson(doc, payload);
  const String topic = baseTopic + "/slot/" + String(slot) + "/event";
  client.publish(topic.c_str(), payload.c_str(), false);
}

void mqttBegin() {
  baseTopic = "openrf/" + sanitizeTopicPart(config.hostname);
  clientId = sanitizeTopicPart(config.hostname) + "-" + openrfChipIdHex();
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
  doc["radio1_active"] = Radio.isRadioActive(1);
  doc["radio2_active"] = Radio.isRadioActive(2);
  doc["radio1_default_frequency_mhz"] = Radio.getDefaultFrequency(1);
  doc["radio2_default_frequency_mhz"] = Radio.getDefaultFrequency(2);
  doc["radio1_frequency_mhz"] = Radio.getOperatingFrequency(1);
  doc["radio2_frequency_mhz"] = Radio.getOperatingFrequency(2);
  doc["radio1_frequency_tuned"] = Radio.isFrequencyTuned(1);
  doc["radio2_frequency_tuned"] = Radio.isFrequencyTuned(2);
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
