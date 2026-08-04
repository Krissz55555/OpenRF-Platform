#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <Updater.h>

#include "config.h"
#include "version.h"
#include "radio.h"
#include "storage.h"
#include "rxslots.h"
#include "scratch.h"
#include "mqtt.h"
#include "wifi.h"
#include "backup.h"
#include "analyzer.h"
#include "web.h"

ESP8266WebServer server(80);

namespace {
bool restartScheduled = false;
uint32_t restartAtMs = 0;
bool otaUploadOk = false;
String otaUploadMessage;
bool backupUploadOk = false;
String backupUploadMessage;
File backupUploadFile;
constexpr const char* BACKUP_UPLOAD_PATH = "/openrf-backup-upload.tmp";

void scheduleRestart(uint32_t delayMs, const __FlashStringHelper* reason);

String uint64Hex(uint64_t value) {
  if (value == 0) return "0";
  char buffer[17];
  buffer[16] = '\0';
  int index = 16;
  constexpr char digits[] = "0123456789ABCDEF";
  while (value && index > 0) {
    buffer[--index] = digits[value & 0x0F];
    value >>= 4;
  }
  return String(&buffer[index]);
}


void sendJsonError(uint16_t statusCode, const String& message) {
  JsonDocument doc;
  doc["success"] = false;
  doc["message"] = message;

  String output;
  serializeJson(doc, output);
  server.send(statusCode, "application/json", output);
}


void sendJsonDoc(uint16_t statusCode, JsonDocument& doc) {
  String output; serializeJson(doc, output); server.send(statusCode, "application/json", output);
}

void serveFile(const char* path, const char* contentType) {
  if (!LittleFS.exists(path)) {
    server.send(404, "text/plain", String(path) + " not found");
    return;
  }

  File file = LittleFS.open(path, "r");
  server.streamFile(file, contentType);
  file.close();
}

void handleRoot() {
  serveFile("/index.html", "text/html");
}

void handleStatusApi() {
  JsonDocument doc;

  doc["device"] = "OpenRF Platform";
  doc["hostname"] = config.hostname;
  doc["version"] = FW_VERSION;
  doc["wifi_mode"] = wifiModeName();
  doc["ip"] = wifiIpAddress();
  doc["sta_connected"] = wifiStationConnected();
  doc["mqtt_enabled"] = config.mqttEnabled;
  doc["mqtt_connected"] = mqttIsConnected();
  doc["mqtt_state"] = mqttStateName();
  doc["mqtt_base_topic"] = mqttBaseTopic();
  doc["radio"] = Radio.getModeName();
  doc["slots"] = 30;
  doc["uptime_seconds"] = millis() / 1000UL;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["max_free_block"] = ESP.getMaxFreeBlockSize();
  doc["heap_fragmentation_percent"] = ESP.getHeapFragmentation();
  doc["reset_reason"] = ESP.getResetReason();

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}


void handleRadioApi() {
  JsonDocument doc;

  doc["initialized"] = Radio.isInitialized();
  doc["chip"] = Radio.getChipName();
  doc["frequency_mhz"] = Radio.getFrequency();
  doc["mode"] = Radio.getModeName();
  doc["receiving"] = Radio.isReceiving();
  doc["rssi_dbm"] = Radio.getRSSI();
  doc["last_error"] = Radio.getLastError();
  doc["modulation"] = "OOK";

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}


void handleRadioRawApi() {
  const RawFrameInfo info = Radio.getLastFrameInfo();

  if (!info.available) {
    JsonDocument doc;
    doc["available"] = false;
    doc["pulse_count"] = 0;
    doc["message"] = "No RAW frame received yet";

    String output;
    serializeJson(doc, output);
    server.send(200, "application/json", output);
    return;
  }

  const uint16_t copied = Radio.copyLastRaw(openrfScratch, OPENRF_MAX_RAW_PULSES);

  String output;
  output.reserve(256 + copied * 7);
  output += "{\"available\":true";
  output += ",\"sequence\":" + String(info.sequence);
  output += ",\"pulse_count\":" + String(copied);
  output += ",\"duration_us\":" + String(info.durationUs);
  output += ",\"rssi_dbm\":" + String(info.rssiDbm, 1);
  output += ",\"age_ms\":" + String(millis() - info.receivedAtMs);
  output += ",\"raw\":[";

  for (uint16_t i = 0; i < copied; i++) {
    if (i > 0) output += ',';
    output += String(openrfScratch[i]);
    if ((i & 0x3F) == 0) yield();
  }

  output += "]}";
  server.send(200, "application/json", output);
}


void sendSuccess(const String& message) {
  JsonDocument doc;
  doc["success"] = true;
  doc["message"] = message;
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void handleLearnStatusApi() {
  const LearnCaptureInfo info = Radio.getLearnCaptureInfo();
  JsonDocument doc;
  doc["state"] = Radio.getLearnStateName();
  doc["available"] = info.available;
  doc["sequence"] = info.sequence;
  doc["pulse_count"] = info.pulseCount;
  doc["duration_us"] = info.durationUs;
  doc["rssi_dbm"] = info.rssiDbm;
  doc["age_ms"] = info.capturedAtMs > 0 ? millis() - info.capturedAtMs : 0;
  doc["persistent"] = false;
  doc["noise_floor_dbm"] = info.noiseFloorDbm;
  doc["rejected_during_learn"] = info.rejectedDuringLearn;
  doc["last_reject_reason"] = info.lastRejectReason;

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void handleLearnRawApi() {
  const LearnCaptureInfo info = Radio.getLearnCaptureInfo();
  if (!info.available) {
    sendJsonError(404, "No Learn preview is available");
    return;
  }

  const uint16_t copied = Radio.copyLearnRaw(openrfScratch, OPENRF_MAX_RAW_PULSES);
  String output;
  output.reserve(256 + copied * 7);
  output += "{\"available\":true";
  output += ",\"state\":\"" + String(Radio.getLearnStateName()) + "\"";
  output += ",\"pulse_count\":" + String(copied);
  output += ",\"duration_us\":" + String(info.durationUs);
  output += ",\"rssi_dbm\":" + String(info.rssiDbm, 1);
  output += ",\"raw\":[";
  for (uint16_t i = 0; i < copied; i++) {
    if (i > 0) output += ',';
    output += String(openrfScratch[i]);
    if ((i & 0x3F) == 0) yield();
  }
  output += "]}";
  server.send(200, "application/json", output);
}

void handleLearnStartApi() {
  if (!Radio.startLearning()) {
    sendJsonError(409, "Radio is not ready for learning");
    return;
  }
  sendSuccess("Learning started. Press the remote button once.");
}

void handleLearnAcceptApi() {
  if (!Radio.acceptLearnCapture()) {
    sendJsonError(409, "No preview is ready to accept");
    return;
  }
  sendSuccess("Capture accepted in RAM. Persistent slot storage is not enabled yet.");
}

void handleLearnDiscardApi() {
  if (!Radio.discardLearnCapture()) {
    sendJsonError(409, "There is no active Learn capture");
    return;
  }
  sendSuccess("Learn capture discarded");
}


void handleLearnTestSendApi() {
  if (!Radio.testSendLearnCapture(config.replayCount)) {
    sendJsonError(409, "No valid Learn preview is ready, or TX failed");
    return;
  }
  sendSuccess("Test transmission completed and RAW receiver restored");
}


void handleAnalyzerApi() {
  // ESP8266 stability profile: build a bounded snapshot and stream it directly
  // to the client. Avoiding one large temporary String prevents repeated heap
  // reallocations and fragmentation while the Analyzer page is open.
  const bool developerMode = config.analyzerDeveloperMode;

  // In normal gateway mode the full Analyzer is intentionally stopped on
  // ESP8266. Return only a tiny status document so the page can explain the
  // operating mode without allocating the full diagnostics JSON.
  if (!developerMode) {
    char disabledJson[384];
    snprintf(disabledJson, sizeof(disabledJson),
             "{\"available\":false,\"analyzer_disabled\":true,"
             "\"analyzer_developer_mode\":false,"
             "\"status\":\"Analyzer standby - normal gateway mode\","
             "\"frequency_mhz\":%.3f,\"current_rssi_dbm\":%.1f,"
             "\"heap_free\":%lu,\"heap_max_block\":%lu}",
             Radio.getFrequency(), Radio.getRSSI(),
             static_cast<unsigned long>(ESP.getFreeHeap()),
             static_cast<unsigned long>(ESP.getMaxFreeBlockSize()));
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", disabledJson);
    return;
  }

  const AnalyzerSnapshot a = analyzerGetSnapshot();
  const AnalyzerCandidateSnapshot c = analyzerGetLastCandidate();
  constexpr uint16_t DEVELOPER_RAW_LIMIT = 96;
  const uint16_t rawLimit = DEVELOPER_RAW_LIMIT;

  // Final ESP8266 safety guard. Do not start a dynamic Analyzer JSON build if
  // the TCP/IP stack does not have enough contiguous heap available. Returning
  // a tiny fixed-buffer response is preferable to corrupting the allocator.
  const uint32_t freeHeap = ESP.getFreeHeap();
  const uint32_t maxBlock = ESP.getMaxFreeBlockSize();
  if (freeHeap < 17000U || maxBlock < 9000U) {
    char lowMemoryJson[192];
    snprintf(lowMemoryJson, sizeof(lowMemoryJson),
             "{\"available\":false,\"low_memory\":true,\"heap_free\":%lu,"
             "\"heap_max_block\":%lu,\"status\":\"Analyzer paused: low memory\"}",
             static_cast<unsigned long>(freeHeap),
             static_cast<unsigned long>(maxBlock));
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", lowMemoryJson);
    return;
  }

  JsonDocument doc;
  doc["available"] = a.available;
  doc["sequence"] = a.sequence;
  doc["age_ms"] = a.available ? millis() - a.capturedAtMs : 0;
  doc["frequency_mhz"] = a.frequencyMHz;
  doc["rssi_dbm"] = a.rssiDbm;
  doc["pulse_count"] = a.pulseCount;
  doc["duration_us"] = a.durationUs;
  doc["accepted"] = a.accepted;
  doc["status"] = a.status;
  doc["reject_reason"] = a.rejectReason;
  doc["protocol"] = a.protocol;
  doc["encoding"] = a.encoding;
  doc["device_id"] = a.deviceId;
  doc["command"] = a.command;
  doc["symbol_count"] = a.symbolCount;
  doc["code_hex"] = a.symbolCount ? uint64Hex(a.code) : String();
  doc["base_pulse_us"] = a.basePulseUs;
  doc["frame_count"] = a.frameCount;
  doc["quality"] = a.quality;
  doc["bitstream"] = a.bitstream;
  doc["min_pulse_us"] = a.minPulseUs;
  doc["max_pulse_us"] = a.maxPulseUs;
  doc["average_pulse_us"] = a.averagePulseUs;
  doc["shortest_class_us"] = a.shortestClassUs;
  doc["class_ratio"] = a.classRatio;

  const uint16_t rawCount = min(a.rawPulseCount, rawLimit);
  doc["raw_truncated"] = rawCount < a.pulseCount;
  JsonArray raw = doc["raw_pulses_us"].to<JsonArray>();
  for (uint16_t i = 0; i < rawCount; i++) raw.add(a.rawPulses[i]);

  doc["decoded_frames"] = a.decodedFrames;
  doc["unknown_frames"] = a.unknownFrames;
  doc["structured_signal"] = a.structuredSignal;
  doc["occurrences"] = a.occurrences;
  doc["similarity"] = a.similarity;
  JsonArray classes = doc["pulse_classes_us"].to<JsonArray>();
  for (uint8_t i = 0; i < a.pulseClassCount; i++) classes.add(a.pulseClasses[i]);

  const RadioDiagnostics d = Radio.getDiagnostics();
  doc["raw_candidates"] = d.rawCandidates;
  doc["accepted_frames"] = d.acceptedFrames;
  doc["rejected_frames"] = d.rejectedFrames;
  doc["background_filtered_frames"] = d.backgroundFilteredFrames;
  doc["ignored_glitch_edges"] = d.ignoredGlitchEdges;
  doc["gap_finalized_frames"] = d.gapFinalizedFrames;
  doc["timeout_finalized_frames"] = d.timeoutFinalizedFrames;
  doc["buffer_full_frames"] = d.bufferFullFrames;
  doc["merged_same_sign_pulses"] = d.mergedSameSignPulses;
  doc["weak_rssi_frames"] = a.weakRssiFrames;
  doc["analyzer_min_rssi"] = config.analyzerMinRssi;
  doc["current_rssi_dbm"] = Radio.getRSSI();
  doc["peak_rssi_dbm"] = a.peakRssiDbm;
  doc["analyzer_min_pulse_count"] = config.analyzerMinPulseCount;
  doc["analyzer_min_duration_us"] = config.analyzerMinDurationUs;
  doc["analyzer_similarity"] = config.analyzerSimilarity;
  doc["analyzer_occurrences"] = config.analyzerOccurrences;
  doc["analyzer_show_rejected"] = config.analyzerShowRejected;
  doc["analyzer_freeze_candidate"] = config.analyzerFreezeCandidate;
  doc["analyzer_alternation_tolerance"] = config.analyzerAlternationTolerance;
  doc["analyzer_developer_mode"] = developerMode;
  doc["heap_free"] = freeHeap;
  doc["heap_max_block"] = maxBlock;
  doc["low_memory"] = false;

  JsonObject candidate = doc["last_candidate"].to<JsonObject>();
  candidate["available"] = c.available;
  candidate["sequence"] = c.sequence;
  candidate["age_ms"] = c.available ? millis() - c.capturedAtMs : 0;
  candidate["frequency_mhz"] = c.frequencyMHz;
  candidate["rssi_dbm"] = c.rssiDbm;
  candidate["pulse_count"] = c.pulseCount;
  candidate["duration_us"] = c.durationUs;
  candidate["reject_reason"] = c.rejectReason;
  candidate["min_pulse_us"] = c.minPulseUs;
  candidate["max_pulse_us"] = c.maxPulseUs;
  candidate["alternation_ratio"] = c.alternationRatio;
  candidate["same_sign_pairs"] = c.sameSignPairs;
  candidate["longest_same_sign_run"] = c.longestSameSignRun;
  candidate["normalized_pulse_count"] = c.normalizedPulseCount;

  const uint16_t candidateRawCount = developerMode ? min(c.rawPulseCount, DEVELOPER_RAW_LIMIT) : 0;
  candidate["raw_truncated"] = candidateRawCount < c.pulseCount;
  JsonArray candidateRaw = candidate["raw_pulses_us"].to<JsonArray>();
  for (uint16_t i = 0; i < candidateRawCount; i++) candidateRaw.add(c.rawPulses[i]);

  JsonArray normalizedRaw = candidate["normalized_pulses_us"].to<JsonArray>();
  if (developerMode) {
    const uint16_t normalizedCount = min(c.normalizedPulseCount, DEVELOPER_RAW_LIMIT);
    for (uint16_t i = 0; i < normalizedCount; i++) normalizedRaw.add(c.normalizedPulses[i]);
  }

  const size_t contentLength = measureJson(doc);
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(contentLength);
  server.send(200, "application/json", "");
  serializeJson(doc, server.client());
}

void handleAnalyzerSettingsApi() {
  if (!server.hasArg("plain")) {
    sendJsonError(400, "Missing JSON request body");
    return;
  }

  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    sendJsonError(400, "Invalid JSON request body");
    return;
  }

  const bool previousDeveloperMode = config.analyzerDeveloperMode;

  if (doc["min_rssi"].is<int>()) {
    const int value = doc["min_rssi"];
    if (value < -100 || value > -20) { sendJsonError(400, "Minimum RSSI must be between -100 and -20 dBm"); return; }
    config.analyzerMinRssi = static_cast<int8_t>(value);
  }
  if (doc["min_pulse_count"].is<int>()) {
    const int value = doc["min_pulse_count"];
    if (value < 2 || value > 300) { sendJsonError(400, "Minimum pulse count must be between 2 and 300"); return; }
    config.analyzerMinPulseCount = static_cast<uint16_t>(value);
  }
  if (!doc["min_duration_us"].isNull()) {
    const uint32_t value = doc["min_duration_us"].as<uint32_t>();
    if (value < 500 || value > 500000) { sendJsonError(400, "Minimum duration must be between 500 and 500000 us"); return; }
    config.analyzerMinDurationUs = value;
  }
  if (doc["similarity"].is<int>()) {
    const int value = doc["similarity"];
    if (value < 50 || value > 100) { sendJsonError(400, "Similarity must be between 50 and 100 percent"); return; }
    config.analyzerSimilarity = static_cast<uint8_t>(value);
  }
  if (doc["occurrences"].is<int>()) {
    const int value = doc["occurrences"];
    if (value < 1 || value > 10) { sendJsonError(400, "Occurrences must be between 1 and 10"); return; }
    config.analyzerOccurrences = static_cast<uint8_t>(value);
  }
  if (doc["show_rejected"].is<bool>()) config.analyzerShowRejected = doc["show_rejected"];
  if (doc["freeze_candidate"].is<bool>()) config.analyzerFreezeCandidate = doc["freeze_candidate"];
  if (doc["alternation_tolerance"].is<int>()) {
    const int value = doc["alternation_tolerance"];
    if (value < 50 || value > 100) { sendJsonError(400, "Alternation tolerance must be between 50 and 100 percent"); return; }
    config.analyzerAlternationTolerance = static_cast<uint8_t>(value);
  }
  if (doc["developer_mode"].is<bool>()) config.analyzerDeveloperMode = doc["developer_mode"];

  if (previousDeveloperMode != config.analyzerDeveloperMode) analyzerReset();

  if (!configSave()) {
    sendJsonError(500, "Failed to save Analyzer settings");
    return;
  }

  JsonDocument response;
  response["success"] = true;
  response["message"] = "Analyzer settings saved";
  sendJsonDoc(200, response);
}

void handleRadioDebugApi() {
  const RadioDiagnostics d = Radio.getDiagnostics();
  JsonDocument doc;
  doc["raw_candidates"] = d.rawCandidates;
  doc["accepted_frames"] = d.acceptedFrames;
  doc["rejected_frames"] = d.rejectedFrames;
  doc["background_filtered_frames"] = d.backgroundFilteredFrames;
  doc["ignored_glitch_edges"] = d.ignoredGlitchEdges;
  doc["gap_finalized_frames"] = d.gapFinalizedFrames;
  doc["timeout_finalized_frames"] = d.timeoutFinalizedFrames;
  doc["buffer_full_frames"] = d.bufferFullFrames;
  doc["merged_same_sign_pulses"] = d.mergedSameSignPulses;
  doc["tx_count"] = d.txCount;
  doc["tx_errors"] = d.txErrors;
  doc["last_noise_floor_dbm"] = d.lastNoiseFloorDbm;
  doc["last_reject_reason"] = d.lastRejectReason;
  doc["learn_state"] = Radio.getLearnStateName();
  doc["monitor_min_pulses"] = 60;
  doc["monitor_min_duration_us"] = 80000;
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void handleSlotsApi() {
  // Stream the JSON response in small chunks. This avoids building the full
  // 30-slot document in a single String, which can be truncated on ESP8266
  // when the heap is fragmented even though HTTP 200 has already been sent.
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");

  server.sendContent(F("{\"slots\":["));

  uint8_t usedCount = 0;
  for (uint8_t i = 1; i <= OPENRF_SLOT_COUNT; i++) {
    const SlotInfo info = storageGetSlotInfo(i);
    if (info.used) usedCount++;

    JsonDocument slotDoc;
    slotDoc["id"] = static_cast<unsigned int>(i);
    slotDoc["name"] = info.name;
    slotDoc["used"] = info.used;
    slotDoc["frequency_mhz"] = info.frequencyMHz;
    slotDoc["pulse_count"] = info.pulseCount;
    slotDoc["duration_us"] = info.durationUs;
    slotDoc["fingerprint"] = info.fingerprint;

    String chunk;
    chunk.reserve(220);
    serializeJson(slotDoc, chunk);
    if (i > 1) server.sendContent(F(","));
    server.sendContent(chunk);
    yield();
  }

  String tail;
  tail.reserve(48);
  tail += F("],\"count\":");
  tail += String(static_cast<unsigned int>(OPENRF_SLOT_COUNT));
  tail += F(",\"used_count\":");
  tail += String(static_cast<unsigned int>(usedCount));
  tail += '}';
  server.sendContent(tail);
  server.sendContent("");

  Serial.print(F("Slots API streamed, used="));
  Serial.println(static_cast<unsigned int>(usedCount));
}

bool readSlotRequest(JsonDocument& doc, uint8_t& slot) {
  if (!server.hasArg("plain")) { sendJsonError(400, "Missing JSON request body"); return false; }
  if (deserializeJson(doc, server.arg("plain"))) { sendJsonError(400, "Invalid JSON request body"); return false; }
  const int requested = doc["slot"] | 0;
  if (requested < 1 || requested > OPENRF_SLOT_COUNT) { sendJsonError(400, "Slot must be between 1 and 30"); return false; }
  slot = static_cast<uint8_t>(requested);
  return true;
}

void handleSlotSaveApi() {
  JsonDocument doc; uint8_t slot;
  if (!readSlotRequest(doc, slot)) return;
  const LearnCaptureInfo capture = Radio.getLearnCaptureInfo();
  if (!capture.available || (capture.state != LearnState::PREVIEW_READY && capture.state != LearnState::ACCEPTED_RAM)) {
    sendJsonError(409, "No valid Learn preview is ready to save"); return;
  }
  String name = doc["name"].is<const char*>() ? doc["name"].as<String>() : ("RF Slot " + String(slot));
  name.trim();
  if (name.length() > OPENRF_SLOT_NAME_MAX) { sendJsonError(400, "Slot name is too long"); return; }
  const uint16_t count = Radio.copyLearnRaw(openrfScratch, OPENRF_MAX_RAW_PULSES);
  uint32_t fingerprint = 0;
  if (!storageSaveSlot(slot, name, Radio.getFrequency(), openrfScratch, count, capture.durationUs, &fingerprint)) {
    sendJsonError(500, "Failed to save slot to LittleFS"); return;
  }
  JsonDocument response;
  response["success"] = true;
  response["message"] = "Signal saved to slot " + String(slot);
  response["slot"] = slot;
  response["fingerprint"] = fingerprint;
  String output; serializeJson(response, output); server.send(200, "application/json", output);
  if (mqttIsConnected() && config.homeAssistantDiscovery) mqttPublishDiscovery();
  Serial.print("SLOT saved: "); Serial.print(slot); Serial.print(", "); Serial.print(count);
  Serial.print(" pulses, fingerprint "); Serial.println(fingerprint, HEX);
}

void handleSlotSendApi() {
  JsonDocument doc; uint8_t slot;
  if (!readSlotRequest(doc, slot)) return;
  SlotInfo info;
  if (!storageLoadSlot(slot, openrfScratch, OPENRF_MAX_RAW_PULSES, info)) { sendJsonError(404, "Slot is empty or invalid"); return; }
  if (!Radio.sendRaw(openrfScratch, info.pulseCount, config.replayCount)) { sendJsonError(500, "RF transmission failed"); return; }
  sendSuccess("Slot " + String(slot) + " transmitted");
  Serial.print("SLOT sent: "); Serial.println(slot);
}

void handleSlotRenameApi() {
  JsonDocument doc; uint8_t slot;
  if (!readSlotRequest(doc, slot)) return;
  String name = doc["name"] | ""; name.trim();
  if (name.length() == 0 || name.length() > OPENRF_SLOT_NAME_MAX) { sendJsonError(400, "Name must contain 1 to 32 characters"); return; }
  if (!storageRenameSlot(slot, name)) { sendJsonError(404, "Slot is empty or rename failed"); return; }
  sendSuccess("Slot " + String(slot) + " renamed");
  if (mqttIsConnected() && config.homeAssistantDiscovery) mqttPublishDiscovery();
}

void handleSlotDeleteApi() {
  JsonDocument doc; uint8_t slot;
  if (!readSlotRequest(doc, slot)) return;
  if (!storageDeleteSlot(slot)) { sendJsonError(500, "Failed to delete slot"); return; }
  sendSuccess("Slot " + String(slot) + " deleted");
  if (mqttIsConnected() && config.homeAssistantDiscovery) mqttPublishDiscovery();
  Serial.print("SLOT deleted: "); Serial.println(slot);
}


void handleRxSlotsApi() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent("{\"count\":10,\"used_count\":" + String(rxSlotCountUsed()) + ",\"learn_state\":\"" + String(rxSlotLearnState()) + "\",\"learning_slot\":" + String(rxSlotLearningId()) + ",\"slots\":[");
  for (uint8_t i=1;i<=OPENRF_RX_SLOT_COUNT;i++) {
    if (i>1) server.sendContent(",");
    RxSlotInfo x=rxSlotGetInfo(i); JsonDocument d;
    d["id"]=i; d["used"]=x.used; d["enabled"]=x.enabled; d["name"]=x.name;
    d["protocol"]=x.protocol; d["symbol_count"]=x.symbolCount;
    d["device_id"]=x.deviceId; d["command"]=x.command; d["code"]=x.code;
    d["match_code"]=x.matchCode; d["pulse_length_us"]=x.pulseLengthUs;
    d["match_count"]=x.matchCount; d["last_quality"]=x.lastQuality; d["last_rssi"]=x.lastRssi;
    String out; serializeJson(d,out); server.sendContent(out); yield();
  }
  server.sendContent("]}");
}
void handleRxLearnApi() {
  JsonDocument body; if (deserializeJson(body, server.arg("plain"))) { sendJsonError(400,"Invalid JSON"); return; }
  uint8_t slot=body["slot"]|0; String name=body["name"]|("RX Slot "+String(slot));
  if (!rxSlotStartLearn(slot,name)) { sendJsonError(409,"RX learn is busy or unavailable"); return; }
  JsonDocument d; d["ok"]=true; d["message"]="Waiting for RF signal"; sendJsonDoc(200,d);
}
void handleRxDeleteApi(){ JsonDocument b;if(deserializeJson(b,server.arg("plain"))){sendJsonError(400,"Invalid JSON");return;} uint8_t slot=b["slot"]|0; bool ok=rxSlotDelete(slot); if(ok&&config.homeAssistantDiscovery)mqttPublishDiscovery(); JsonDocument d;d["ok"]=ok;d["message"]=ok?"RX slot deleted":"Delete failed";sendJsonDoc(ok?200:400,d);}
void handleRxRenameApi(){ JsonDocument b;if(deserializeJson(b,server.arg("plain"))){sendJsonError(400,"Invalid JSON");return;} uint8_t slot=b["slot"]|0;String name=b["name"]|"";bool ok=rxSlotRename(slot,name);if(ok&&config.homeAssistantDiscovery)mqttPublishDiscovery();JsonDocument d;d["ok"]=ok;d["message"]=ok?"RX slot renamed":"Rename failed";sendJsonDoc(ok?200:400,d);}
void handleRxEnableApi(){ JsonDocument b;if(deserializeJson(b,server.arg("plain"))){sendJsonError(400,"Invalid JSON");return;}uint8_t slot=b["slot"]|0;bool enabled=b["enabled"]|false;bool ok=rxSlotSetEnabled(slot,enabled);if(ok&&config.homeAssistantDiscovery)mqttPublishDiscovery();JsonDocument d;d["ok"]=ok;d["message"]=ok?(enabled?"RX slot enabled":"RX slot disabled"):"Update failed";sendJsonDoc(ok?200:400,d);}

void handleGetConfigApi() {
  server.send(200, "application/json", configToJson());
}

void handlePostConfigApi() {
  if (!server.hasArg("plain")) {
    sendJsonError(400, "Missing JSON request body");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    sendJsonError(400, "Invalid JSON request body");
    return;
  }

  if (!doc["hostname"].is<const char*>()) {
    sendJsonError(400, "Hostname is required");
    return;
  }

  String hostname = doc["hostname"].as<String>();
  hostname.trim();

  if (hostname.length() == 0 || hostname.length() > 32) {
    sendJsonError(400, "Hostname must contain 1 to 32 characters");
    return;
  }

  String wifiSsid = doc["wifi_ssid"] | "";
  wifiSsid.trim();
  if (wifiSsid.length() > 32) {
    sendJsonError(400, "WiFi SSID must be 32 characters or fewer");
    return;
  }

  bool mqttEnabled = doc["mqtt_enabled"] | false;

  String mqttHost = doc["mqtt_host"] | "";
  mqttHost.trim();

  uint32_t mqttPort = doc["mqtt_port"] | 1883;
  if (mqttPort < 1 || mqttPort > 65535) {
    sendJsonError(400, "MQTT port must be between 1 and 65535");
    return;
  }

  if (mqttEnabled && mqttHost.length() == 0) {
    sendJsonError(400, "MQTT host is required when MQTT is enabled");
    return;
  }

  String mqttUser = doc["mqtt_user"] | "";
  mqttUser.trim();

  uint32_t replayCount = doc["replay_count"] | 1;
  const uint16_t radioFrequencyMhz = doc["radio_frequency_mhz"] | 433;
  if (replayCount < 1 || replayCount > 10) {
    sendJsonError(400, "Replay count must be between 1 and 10");
    return;
  }
  if (radioFrequencyMhz != 433 && radioFrequencyMhz != 868) {
    sendJsonError(400, "Radio frequency must be 433 or 868 MHz");
    return;
  }

  config.hostname = hostname;
  config.wifiSsid = wifiSsid;
  if (doc["wifi_password"].is<const char*>()) {
    String wifiPassword = doc["wifi_password"].as<String>();
    if (wifiPassword.length() > 0) config.wifiPassword = wifiPassword;
  }
  config.mqttEnabled = mqttEnabled;
  config.mqttHost = mqttHost;
  config.mqttPort = static_cast<uint16_t>(mqttPort);
  config.mqttUser = mqttUser;
  config.homeAssistantDiscovery = doc["home_assistant_discovery"] | true;
  config.replayCount = static_cast<uint8_t>(replayCount);
  config.radioFrequencyMhz = radioFrequencyMhz;

  // Empty password means: keep the currently saved password.
  if (doc["mqtt_password"].is<const char*>()) {
    String mqttPassword = doc["mqtt_password"].as<String>();
    if (mqttPassword.length() > 0) {
      config.mqttPassword = mqttPassword;
    }
  }

  if (!configSave()) {
    sendJsonError(500, "Failed to save configuration to LittleFS");
    return;
  }

  JsonDocument response;
  response["success"] = true;
  response["message"] = "Configuration saved. OpenRF Platform is restarting and will join the configured WiFi network.";
  response["restart_required"] = true;

  String output;
  serializeJson(response, output);
  server.sendHeader("Connection", "close");
  server.send(200, "application/json", output);
  scheduleRestart(3000, F("configuration save"));
}


void scheduleRestart(uint32_t delayMs, const __FlashStringHelper* reason) {
  restartScheduled = true;
  restartAtMs = millis() + delayMs;
  Serial.print(F("Restart scheduled: "));
  Serial.println(reason);
}

void handleBackupDownload() {
  uint16_t fileCount = 0;
  const size_t contentLength = backupCalculateSize(fileCount);
  if (fileCount == 0 || contentLength == 0) {
    sendJsonError(500, "Nothing is available to back up");
    return;
  }

  String filename = "OpenRF-Platform-backup-" + String(millis()) + ".orfbackup";
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(contentLength);
  server.send(200, "application/octet-stream", "");

  String error;
  if (!backupStreamToClient(server.client(), error)) {
    Serial.print(F("Backup stream failed: "));
    Serial.println(error);
  } else {
    Serial.print(F("Backup downloaded, files="));
    Serial.println(fileCount);
  }
}

void handleBackupUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    backupUploadOk = false;
    backupUploadMessage = "Backup upload started";
    if (LittleFS.exists(BACKUP_UPLOAD_PATH)) LittleFS.remove(BACKUP_UPLOAD_PATH);
    backupUploadFile = LittleFS.open(BACKUP_UPLOAD_PATH, "w");
    if (!backupUploadFile) backupUploadMessage = "Could not create temporary backup file";
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (backupUploadFile && backupUploadFile.write(upload.buf, upload.currentSize) != upload.currentSize) {
      backupUploadMessage = "Could not write uploaded backup";
      backupUploadFile.close();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (backupUploadFile) backupUploadFile.close();
    String error;
    backupUploadOk = backupRestoreFromFile(BACKUP_UPLOAD_PATH, error);
    backupUploadMessage = backupUploadOk ? "Backup restored successfully" : error;
    LittleFS.remove(BACKUP_UPLOAD_PATH);
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (backupUploadFile) backupUploadFile.close();
    LittleFS.remove(BACKUP_UPLOAD_PATH);
    backupUploadMessage = "Backup upload was aborted";
  }
}

void handleBackupRestoreComplete() {
  JsonDocument doc;
  doc["success"] = backupUploadOk;
  doc["message"] = backupUploadMessage;
  doc["restart_required"] = backupUploadOk;
  String output;
  serializeJson(doc, output);
  server.send(backupUploadOk ? 200 : 400, "application/json", output);
  if (backupUploadOk) scheduleRestart(2500, F("backup restore"));
}

void handleOtaUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    otaUploadOk = false;
    otaUploadMessage = "Firmware upload started";
    Radio.stopReceive();
    const uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace, U_FLASH)) {
      otaUploadMessage = Update.getErrorString();
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!Update.hasError() && Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      otaUploadMessage = Update.getErrorString();
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!Update.hasError() && Update.end(true)) {
      otaUploadOk = true;
      otaUploadMessage = "Firmware installed successfully. Restarting...";
    } else {
      otaUploadMessage = Update.getErrorString();
      Update.printError(Serial);
      Radio.startReceive();
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end(false);
    otaUploadMessage = "Firmware upload was aborted";
    Radio.startReceive();
  }
}

void handleOtaComplete() {
  JsonDocument doc;
  doc["success"] = otaUploadOk;
  doc["message"] = otaUploadMessage;
  doc["restart_required"] = otaUploadOk;
  String output;
  serializeJson(doc, output);
  server.sendHeader("Connection", "close");
  server.send(otaUploadOk ? 200 : 400, "application/json", output);
  if (otaUploadOk) scheduleRestart(2000, F("firmware update"));
}

void handleNotFound() {
  if (server.uri().startsWith("/api/")) {
    sendJsonError(404, "API endpoint not found");
    return;
  }

  server.send(404, "text/plain", "Not found");
}

}  // namespace

void webBegin() {
  server.on("/", HTTP_GET, handleRoot);

  server.on("/style.css", HTTP_GET, []() {
    serveFile("/style.css", "text/css");
  });

  server.on("/app.js", HTTP_GET, []() {
    serveFile("/app.js", "application/javascript");
  });

  server.on("/logo.svg", HTTP_GET, []() {
    serveFile("/logo.svg", "image/svg+xml");
  });

  server.on("/api/status", HTTP_GET, handleStatusApi);
  server.on("/api/radio", HTTP_GET, handleRadioApi);
  server.on("/api/radio/raw", HTTP_GET, handleRadioRawApi);
  server.on("/api/radio/learn", HTTP_GET, handleLearnStatusApi);
  server.on("/api/radio/learn/raw", HTTP_GET, handleLearnRawApi);
  server.on("/api/radio/learn/start", HTTP_POST, handleLearnStartApi);
  server.on("/api/radio/learn/accept", HTTP_POST, handleLearnAcceptApi);
  server.on("/api/radio/learn/discard", HTTP_POST, handleLearnDiscardApi);
  server.on("/api/radio/learn/test-send", HTTP_POST, handleLearnTestSendApi);
  server.on("/api/debug/radio", HTTP_GET, handleRadioDebugApi);
  server.on("/api/analyzer", HTTP_GET, handleAnalyzerApi);
  server.on("/api/analyzer/settings", HTTP_POST, handleAnalyzerSettingsApi);
  server.on("/api/slots", HTTP_GET, handleSlotsApi);
  server.on("/api/slots/save", HTTP_POST, handleSlotSaveApi);
  server.on("/api/slots/send", HTTP_POST, handleSlotSendApi);
  server.on("/api/slots/rename", HTTP_POST, handleSlotRenameApi);
  server.on("/api/slots/delete", HTTP_POST, handleSlotDeleteApi);
  server.on("/api/rxslots", HTTP_GET, handleRxSlotsApi);
  server.on("/api/rxslots/learn", HTTP_POST, handleRxLearnApi);
  server.on("/api/rxslots/delete", HTTP_POST, handleRxDeleteApi);
  server.on("/api/rxslots/rename", HTTP_POST, handleRxRenameApi);
  server.on("/api/rxslots/enable", HTTP_POST, handleRxEnableApi);
  server.on("/api/config", HTTP_GET, handleGetConfigApi);
  server.on("/api/config", HTTP_POST, handlePostConfigApi);
  server.on("/api/system/backup", HTTP_GET, handleBackupDownload);
  server.on("/api/system/restore", HTTP_POST, handleBackupRestoreComplete, handleBackupUpload);
  server.on("/api/system/ota", HTTP_POST, handleOtaComplete, handleOtaUpload);

  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("Web server started");
}

void webLoop() {
  server.handleClient();
  if (restartScheduled && static_cast<int32_t>(millis() - restartAtMs) >= 0) {
    restartScheduled = false;
    Serial.println("Restarting after configuration save");
    delay(50);
    ESP.restart();
  }
}
