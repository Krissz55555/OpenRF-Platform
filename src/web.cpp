#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <Update.h>

#include "config.h"
#include "version.h"
#include "radio.h"
#include "storage.h"
#include "raw_slot_matcher.h"
#include "rxslots.h"
#include "scratch.h"
#include "mqtt.h"
#include "openrf_wifi.h"
#include "backup.h"
#include "analyzer.h"
#include "web.h"
#include "platform_compat.h"
#include "dualcore.h"
#include "psram_buffers.h"
#include "hardware_status.h"
#include "protocol_diagnostics.h"
#include "v2_authoritative_action.h"
#include "protocol_tx_diagnostics.h"

WebServer server(80);

namespace {
uint32_t analyzerFullApiCalls = 0;
uint32_t analyzerLiveApiCalls = 0;
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
  doc["core0_load_percent"] = dualCoreLoad(0);
  doc["core1_load_percent"] = dualCoreLoad(1);

  const OpenRFHardwareStatus hw = hardwareStatusGet();
  doc["rf1_online"] = hw.radio1Online;
  doc["rf2_online"] = hw.radio2Online;
  doc["lora_online"] = hw.loraOnline;

  doc["rf1_enabled"] = hw.radio1Enabled;
  doc["rf2_enabled"] = hw.radio2Enabled;
  doc["lora_enabled"] = hw.loraEnabled;

  doc["rf1_active"] = hw.radio1Active;
  doc["rf2_active"] = hw.radio2Active;
  doc["lora_active"] = hw.loraActive;

  doc["rf1_partnum"] = hw.radio1Part;
  doc["rf1_version"] = hw.radio1Version;
  doc["rf2_partnum"] = hw.radio2Part;
  doc["rf2_version"] = hw.radio2Version;
  doc["lora_version"] = hw.loraVersion;
  doc["active_cc1101"] = Radio.getActiveRadioId();
  doc["rf1_rssi_dbm"] = Radio.getRadioRSSI(1);
  doc["rf2_rssi_dbm"] = Radio.getRadioRSSI(2);
  doc["rf1_default_frequency_mhz"] = Radio.getDefaultFrequency(1);
  doc["rf2_default_frequency_mhz"] = Radio.getDefaultFrequency(2);
  doc["rf1_operating_frequency_mhz"] = Radio.getOperatingFrequency(1);
  doc["rf2_operating_frequency_mhz"] = Radio.getOperatingFrequency(2);
  doc["rf1_frequency_tuned"] = Radio.isFrequencyTuned(1);
  doc["rf2_frequency_tuned"] = Radio.isFrequencyTuned(2);
  doc["rf1_tune_remaining_ms"] = Radio.getTuneSessionRemainingMs(1);
  doc["rf2_tune_remaining_ms"] = Radio.getTuneSessionRemainingMs(2);

  doc["flash_total"] = ESP.getFlashChipSize();
  doc["flash_total_mb"] = ESP.getFlashChipSize() / (1024UL * 1024UL);

  const uint32_t psramTotal = ESP.getPsramSize();
  const uint32_t psramFree = ESP.getFreePsram();
  doc["psram_total"] = psramTotal;
  doc["psram_free"] = psramFree;
  doc["psram_used_percent"] = psramTotal ? ((psramTotal - psramFree) * 100UL / psramTotal) : 0;
  doc["openrf_psram_buffers"] = psramOpenRFAllocatedBytes();
  doc["openrf_psram_external"] = psramBuffersUsingExternalRam();
  doc["analyzer_psram_buffers"] = analyzerPsramAllocatedBytes();
  doc["analyzer_psram_external"] = analyzerUsingExternalRam();

  const uint32_t heapTotal = ESP.getHeapSize();
  const uint32_t heapFree = ESP.getFreeHeap();
  doc["heap_total"] = heapTotal;
  doc["heap_used_percent"] = heapTotal ? ((heapTotal - heapFree) * 100UL / heapTotal) : 0;
  doc["rf_event_queue_depth"] = rfEventQueueDepth();
  doc["rf_event_processed"] = rfEventProcessedCount();
  doc["rf_event_dropped"] = rfEventDroppedCount();
  doc["analyzer_full_api_calls"] = analyzerFullApiCalls;
  doc["analyzer_live_api_calls"] = analyzerLiveApiCalls;
  doc["max_free_block"] = openrfMaxFreeBlock();
  doc["heap_fragmentation_percent"] = openrfHeapFragmentation();
  doc["reset_reason"] = openrfResetReason();

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}


void handleRadioEnableApi() {
  if (!server.hasArg("plain")) {
    sendJsonError(400, "Missing JSON request body");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    sendJsonError(400, "Invalid JSON request body");
    return;
  }

  if (!doc["radio1_enabled"].is<bool>() ||
      !doc["radio2_enabled"].is<bool>() ||
      !doc["lora_enabled"].is<bool>()) {
    sendJsonError(400, "radio1_enabled, radio2_enabled and lora_enabled are required");
    return;
  }

  config.radio1Enabled = doc["radio1_enabled"].as<bool>();
  config.radio2Enabled = doc["radio2_enabled"].as<bool>();
  config.loraEnabled = doc["lora_enabled"].as<bool>();

  if (!configSave()) {
    sendJsonError(500, "Failed to save radio enable configuration");
    return;
  }

  JsonDocument response;
  response["ok"] = true;
  response["restart_required"] = true;
  response["message"] = "Radio configuration saved. OpenRF is restarting.";

  String output;
  serializeJson(response, output);
  server.sendHeader("Connection", "close");
  server.send(200, "application/json", output);
  scheduleRestart(1800, F("radio enable configuration"));
}


void handleFrequencyScanApi() {
  uint8_t radioId = 2;
  if (server.hasArg("radio")) {
    radioId = static_cast<uint8_t>(server.arg("radio").toInt());
  }

  if (radioId != 1 && radioId != 2) {
    sendJsonError(400, "radio must be 1 or 2");
    return;
  }

  const bool enabled =
      radioId == 1 ? config.radio1Enabled : config.radio2Enabled;

  if (!enabled) {
    sendJsonError(409, radioId == 1
        ? "CC1101 Radio 1 is disabled in System"
        : "CC1101 Radio 2 is disabled in System");
    return;
  }

  if (!Radio.isRadioActive(radioId)) {
    sendJsonError(409, radioId == 1
        ? "CC1101 Radio 1 is not ACTIVE"
        : "CC1101 Radio 2 is not ACTIVE");
    return;
  }

  float startMHz = radioId == 1 ? 433.60F : 867.80F;
  float endMHz = radioId == 1 ? 434.20F : 868.90F;
  float stepMHz = 0.025F;
  uint16_t dwellMs = 35;
  uint8_t passes = 4;

  if (server.hasArg("start")) startMHz = server.arg("start").toFloat();
  if (server.hasArg("end")) endMHz = server.arg("end").toFloat();
  if (server.hasArg("step")) stepMHz = server.arg("step").toFloat();
  if (server.hasArg("dwell")) {
    dwellMs = static_cast<uint16_t>(server.arg("dwell").toInt());
  }
  if (server.hasArg("passes")) {
    passes = static_cast<uint8_t>(server.arg("passes").toInt());
  }

  const float minMHz = radioId == 1 ? 430.0F : 867.0F;
  const float maxMHz = radioId == 1 ? 440.0F : 870.0F;

  startMHz = constrain(startMHz, minMHz, maxMHz);
  endMHz = constrain(endMHz, minMHz, maxMHz);
  stepMHz = constrain(stepMHz, 0.010F, 0.100F);
  dwellMs = constrain(
      dwellMs,
      static_cast<uint16_t>(15),
      static_cast<uint16_t>(150));
  passes = constrain(
      passes,
      static_cast<uint8_t>(1),
      static_cast<uint8_t>(10));

  if (endMHz <= startMHz) {
    sendJsonError(400, "Scan end must be above scan start");
    return;
  }

  const uint16_t sampleCount =
      static_cast<uint16_t>(((endMHz - startMHz) / stepMHz) + 1.5F);

  if (sampleCount < 2 || sampleCount > 160) {
    sendJsonError(400, "Scan range/step produces an invalid sample count");
    return;
  }

  float bestRssi[160];
  float frequencies[160];

  for (uint16_t i = 0; i < sampleCount; ++i) {
    bestRssi[i] = -127.0F;
    frequencies[i] = startMHz + static_cast<float>(i) * stepMHz;
  }

  // Multi-pass sweep:
  // every pass revisits every frequency bin, retaining the strongest RSSI ever
  // observed at that bin. This makes short burst-mode remotes much less likely
  // to be missed at a particular frequency.
  for (uint8_t pass = 0; pass < passes; ++pass) {
    for (uint16_t i = 0; i < sampleCount; ++i) {
      const float frequency = frequencies[i];
      if (frequency > endMHz + 0.0005F) break;

      float rssi = -127.0F;
      if (!Radio.scanRssi(radioId, frequency, rssi, dwellMs)) {
        sendJsonError(500, "Frequency scan failed while retuning");
        return;
      }

      if (rssi > bestRssi[i]) bestRssi[i] = rssi;
      yield();
    }
  }

  uint16_t peakIndex = 0;
  for (uint16_t i = 1; i < sampleCount; ++i) {
    if (bestRssi[i] > bestRssi[peakIndex]) peakIndex = i;
  }

  const float strongestRssi = bestRssi[peakIndex];
  const float strongestFrequency = frequencies[peakIndex];

  // Estimate the carrier from the response plateau instead of trusting one
  // discrete peak bin. Use every bin within 3 dB of the peak and weight its
  // frequency by linear signal power converted from dBm.
  double weightedFrequency = 0.0;
  double totalWeight = 0.0;

  for (uint16_t i = 0; i < sampleCount; ++i) {
    if (bestRssi[i] >= strongestRssi - 3.0F) {
      const double linearPower = pow(10.0, bestRssi[i] / 10.0);
      weightedFrequency += linearPower * frequencies[i];
      totalWeight += linearPower;
    }
  }

  const float estimatedCarrier =
      totalWeight > 0.0
          ? static_cast<float>(weightedFrequency / totalWeight)
          : strongestFrequency;

  // Robust local noise floor from the quietest 60 % of final per-bin peaks.
  float sorted[160];
  for (uint16_t i = 0; i < sampleCount; ++i) sorted[i] = bestRssi[i];

  for (uint16_t i = 1; i < sampleCount; ++i) {
    const float value = sorted[i];
    int16_t j = static_cast<int16_t>(i) - 1;
    while (j >= 0 && sorted[j] > value) {
      sorted[j + 1] = sorted[j];
      --j;
    }
    sorted[j + 1] = value;
  }

  uint16_t quietCount =
      static_cast<uint16_t>((sampleCount * 60UL) / 100UL);
  if (quietCount < 1) quietCount = 1;

  float quietSum = 0.0F;
  for (uint16_t i = 0; i < quietCount; ++i) {
    quietSum += sorted[i];
  }

  const float noiseFloor = quietSum / quietCount;
  const float signalAboveNoise = strongestRssi - noiseFloor;

  constexpr float SIGNAL_DETECTION_DELTA_DB = 12.0F;
  const bool signalDetected =
      strongestRssi > -115.0F &&
      signalAboveNoise >= SIGNAL_DETECTION_DELTA_DB;

  const char* signalQuality = "NONE";
  if (signalDetected) {
    if (signalAboveNoise >= 30.0F) {
      signalQuality = "STRONG";
    } else if (signalAboveNoise >= 20.0F) {
      signalQuality = "GOOD";
    } else {
      signalQuality = "DETECTED";
    }
  }

  JsonDocument doc;
  doc["ok"] = true;
  doc["radio"] = radioId;
  doc["start_mhz"] = startMHz;
  doc["end_mhz"] = endMHz;
  doc["step_mhz"] = stepMHz;
  doc["dwell_ms"] = dwellMs;
  doc["passes"] = passes;

  JsonArray samples = doc["samples"].to<JsonArray>();
  for (uint16_t i = 0; i < sampleCount; ++i) {
    JsonObject point = samples.add<JsonObject>();
    point["frequency_mhz"] = serialized(String(frequencies[i], 3));
    point["rssi_dbm"] = serialized(String(bestRssi[i], 1));
  }

  doc["strongest_frequency_mhz"] =
      serialized(String(strongestFrequency, 3));
  doc["strongest_rssi_dbm"] =
      serialized(String(strongestRssi, 1));
  doc["estimated_carrier_mhz"] =
      serialized(String(estimatedCarrier, 4));
  doc["noise_floor_dbm"] =
      serialized(String(noiseFloor, 1));
  doc["signal_above_noise_db"] =
      serialized(String(signalAboveNoise, 1));
  doc["signal_detected"] = signalDetected;
  doc["signal_quality"] = signalQuality;
  doc["detection_threshold_db"] = SIGNAL_DETECTION_DELTA_DB;
  doc["current_operating_frequency_mhz"] =
      serialized(String(Radio.getOperatingFrequency(radioId), 4));

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void handleFrequencyTuneApi() {
  if (!server.hasArg("plain")) {
    sendJsonError(400, "Missing JSON request body");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    sendJsonError(400, "Invalid JSON request body");
    return;
  }

  const uint8_t radioId = doc["radio"] | 0;
  const float frequencyMHz = doc["frequency_mhz"] | 0.0F;

  if (radioId != 1 && radioId != 2) {
    sendJsonError(400, "radio must be 1 or 2");
    return;
  }

  const float minMHz = radioId == 1 ? 430.0F : 867.0F;
  const float maxMHz = radioId == 1 ? 440.0F : 870.0F;

  if (frequencyMHz < minMHz || frequencyMHz > maxMHz) {
    sendJsonError(400, "frequency is outside the supported radio range");
    return;
  }

  if (!Radio.setOperatingFrequency(radioId, frequencyMHz)) {
    sendJsonError(500, "Failed to tune radio");
    return;
  }

  // Step 26.2.1: persist the tuned Operating frequency. If OpenRF reboots
  // during a TUNED session, the same frequency is restored with a fresh
  // 15-minute safety window.
  if (radioId == 1) config.radio1FrequencyMhz = frequencyMHz;
  else config.radio2FrequencyMhz = frequencyMHz;
  if (!configSave()) {
    sendJsonError(500, "Radio tuned, but failed to save tuning session");
    return;
  }

  JsonDocument response;
  response["ok"] = true;
  response["radio"] = radioId;
  response["default_frequency_mhz"] = Radio.getDefaultFrequency(radioId);
  response["frequency_mhz"] = Radio.getOperatingFrequency(radioId);
  response["frequency_tuned"] = Radio.isFrequencyTuned(radioId);
  response["tune_remaining_ms"] = Radio.getTuneSessionRemainingMs(radioId);
  response["message"] = "Radio tuned for 15 minutes";

  String output;
  serializeJson(response, output);
  server.send(200, "application/json", output);
}


void handleFrequencyRestoreApi() {
  if (!server.hasArg("plain")) {
    sendJsonError(400, "Missing JSON request body");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, server.arg("plain"))) {
    sendJsonError(400, "Invalid JSON request body");
    return;
  }

  const uint8_t radioId = doc["radio"] | 0;
  if (radioId != 1 && radioId != 2) {
    sendJsonError(400, "radio must be 1 or 2");
    return;
  }

  if (!Radio.restoreDefaultFrequency(radioId)) {
    sendJsonError(500, "Failed to restore Default frequency");
    return;
  }
  if (radioId == 1) config.radio1FrequencyMhz = Radio.getDefaultFrequency(1);
  else config.radio2FrequencyMhz = Radio.getDefaultFrequency(2);
  if (!configSave()) {
    sendJsonError(500, "Default restored, but failed to save configuration");
    return;
  }

  JsonDocument response;
  response["ok"] = true;
  response["radio"] = radioId;
  response["default_frequency_mhz"] = Radio.getDefaultFrequency(radioId);
  response["frequency_mhz"] = Radio.getOperatingFrequency(radioId);
  response["frequency_tuned"] = false;
  response["tune_remaining_ms"] = 0;
  response["message"] = "Default frequency restored";

  String output;
  serializeJson(response, output);
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
  doc["rf1_rssi_dbm"] = Radio.getRadioRSSI(1);
  doc["rf2_rssi_dbm"] = Radio.getRadioRSSI(2);
  doc["rf1_active"] = Radio.isRadioActive(1);
  doc["rf2_active"] = Radio.isRadioActive(2);
  doc["dual_radio"] = Radio.isRadioActive(1) && Radio.isRadioActive(2);
  doc["last_error"] = Radio.getLastError();
  doc["modulation"] = "OOK";
  doc["rf1_default_frequency_mhz"] = Radio.getDefaultFrequency(1);
  doc["rf2_default_frequency_mhz"] = Radio.getDefaultFrequency(2);
  doc["rf1_operating_frequency_mhz"] = Radio.getOperatingFrequency(1);
  doc["rf2_operating_frequency_mhz"] = Radio.getOperatingFrequency(2);
  doc["rf1_frequency_tuned"] = Radio.isFrequencyTuned(1);
  doc["rf2_frequency_tuned"] = Radio.isFrequencyTuned(2);
  doc["rf1_tune_remaining_ms"] = Radio.getTuneSessionRemainingMs(1);
  doc["rf2_tune_remaining_ms"] = Radio.getTuneSessionRemainingMs(2);

  const RadioChannelDiagnostics rf1Diag = Radio.getChannelDiagnostics(1);
  const RadioChannelDiagnostics rf2Diag = Radio.getChannelDiagnostics(2);

  JsonObject rf1Capture = doc["radio1_capture"].to<JsonObject>();
  rf1Capture["edges"] = rf1Diag.edges;
  rf1Capture["raw_candidates"] = rf1Diag.rawCandidates;
  rf1Capture["accepted_frames"] = rf1Diag.acceptedFrames;
  rf1Capture["rejected_frames"] = rf1Diag.rejectedFrames;
  rf1Capture["current_pulses"] = rf1Diag.currentPulses;
  rf1Capture["ignored_glitch_edges"] = rf1Diag.ignoredGlitchEdges;
  rf1Capture["ignored_while_frame_ready"] = rf1Diag.ignoredWhileFrameReady;
  rf1Capture["short_gap_resets"] = rf1Diag.shortGapResets;
  rf1Capture["gap_finalized"] = rf1Diag.gapFinalizedFrames;
  rf1Capture["timeout_finalized"] = rf1Diag.timeoutFinalizedFrames;
  rf1Capture["stale_partial_finalized"] =
      rf1Diag.stalePartialFinalizedFrames;
  rf1Capture["buffer_full"] = rf1Diag.bufferFullFrames;
  rf1Capture["merged_same_sign"] = rf1Diag.mergedSameSignPulses;
  rf1Capture["frame_ready"] = rf1Diag.frameReady;
  rf1Capture["pending_age_us"] = rf1Diag.pendingAgeUs;
  rf1Capture["last_finalized_pulses"] = rf1Diag.lastFinalizedPulses;
  rf1Capture["last_finalized_duration_us"] = rf1Diag.lastFinalizedDurationUs;
  rf1Capture["last_finalized_age_ms"] =
      rf1Diag.lastFinalizedAtMs ? millis() - rf1Diag.lastFinalizedAtMs : 0;
  rf1Capture["last_finalize_reason"] = rf1Diag.lastFinalizeReason;
  rf1Capture["last_validation_result"] = rf1Diag.lastValidationResult;

  JsonObject rf2Capture = doc["radio2_capture"].to<JsonObject>();
  rf2Capture["edges"] = rf2Diag.edges;
  rf2Capture["raw_candidates"] = rf2Diag.rawCandidates;
  rf2Capture["accepted_frames"] = rf2Diag.acceptedFrames;
  rf2Capture["rejected_frames"] = rf2Diag.rejectedFrames;
  rf2Capture["current_pulses"] = rf2Diag.currentPulses;
  rf2Capture["ignored_glitch_edges"] = rf2Diag.ignoredGlitchEdges;
  rf2Capture["ignored_while_frame_ready"] = rf2Diag.ignoredWhileFrameReady;
  rf2Capture["short_gap_resets"] = rf2Diag.shortGapResets;
  rf2Capture["gap_finalized"] = rf2Diag.gapFinalizedFrames;
  rf2Capture["timeout_finalized"] = rf2Diag.timeoutFinalizedFrames;
  rf2Capture["stale_partial_finalized"] =
      rf2Diag.stalePartialFinalizedFrames;
  rf2Capture["buffer_full"] = rf2Diag.bufferFullFrames;
  rf2Capture["merged_same_sign"] = rf2Diag.mergedSameSignPulses;
  rf2Capture["frame_ready"] = rf2Diag.frameReady;
  rf2Capture["pending_age_us"] = rf2Diag.pendingAgeUs;
  rf2Capture["last_finalized_pulses"] = rf2Diag.lastFinalizedPulses;
  rf2Capture["last_finalized_duration_us"] = rf2Diag.lastFinalizedDurationUs;
  rf2Capture["last_finalized_age_ms"] =
      rf2Diag.lastFinalizedAtMs ? millis() - rf2Diag.lastFinalizedAtMs : 0;
  rf2Capture["last_finalize_reason"] = rf2Diag.lastFinalizeReason;
  rf2Capture["last_validation_result"] = rf2Diag.lastValidationResult;

  const ProtocolDiagnosticsSnapshot protocolDiag =
      protocolDiagnosticsGetSnapshot();
  JsonObject protocol = doc["protocol_diagnostics"].to<JsonObject>();
  protocol["available"] = protocolDiag.available;
  protocol["sequence"] = protocolDiag.sequence;
  protocol["age_ms"] =
      protocolDiag.available ? millis() - protocolDiag.capturedAtMs : 0;
  protocol["radio_id"] = protocolDiag.radioId;
  protocol["pulse_count"] = protocolDiag.pulseCount;
  protocol["duration_us"] = protocolDiag.durationUs;
  protocol["frequency_mhz"] = protocolDiag.frequencyMHz;
  protocol["rssi_dbm"] = protocolDiag.rssiDbm;
  protocol["frame_accepted"] = protocolDiag.frameAccepted;
  protocol["v2_evaluated"] = protocolDiag.v2Evaluated;
  protocol["v2_protocol"] =
      protocolDiagnosticsV2ProtocolName(protocolDiag.v2Protocol);
  protocol["v2_result"] =
      protocolDiagnosticsV2StatusName(protocolDiag.v2Status);
  protocol["registered_decoders"] = protocolDiag.registeredDecoders;
  protocol["evaluated_decoders"] = protocolDiag.evaluatedDecoders;
  protocol["v2_matches"] = protocolDiag.v2MatchCount;
  protocol["v2_no_matches"] = protocolDiag.v2NoMatchCount;
  protocol["v2_decision"] = protocolEngineDecisionStateName(protocolDiag.v2Decision);
  protocol["v2_candidate_count"] = protocolDiag.v2CandidateCount;
  protocol["v2_selected_protocol"] =
      protocolDiagnosticsV2ProtocolName(protocolDiag.v2SelectedProtocol);
  JsonArray v2Candidates = protocol["v2_candidates"].to<JsonArray>();
  for (uint8_t index = 0;
       index < protocolDiag.v2CandidateCount && index < kProtocolEngineMaxCandidates;
       ++index) {
    v2Candidates.add(protocolDiagnosticsV2ProtocolName(protocolDiag.v2Candidates[index]));
  }
  JsonObject decisionCounts = protocol["v2_decision_counts"].to<JsonObject>();
  decisionCounts["unknown"] = protocolDiag.v2UnknownDecisionCount;
  decisionCounts["known"] = protocolDiag.v2KnownDecisionCount;
  decisionCounts["ambiguous"] = protocolDiag.v2AmbiguousDecisionCount;

  JsonObject normalized = protocol["v2_normalized_event"].to<JsonObject>();
  normalized["available"] = protocolDiag.v2NormalizedEventAvailable;
  normalized["count"] = protocolDiag.v2NormalizedEventCount;
  if (protocolDiag.v2NormalizedEventAvailable) {
    const NormalizedRfEvent& event = protocolDiag.v2NormalizedEvent;
    normalized["protocol"] = protocolDiagnosticsV2ProtocolName(event.protocol);
    normalized["code"] = uint64Hex(event.code);
    normalized["symbol_count"] = event.symbolCount;
    normalized["repeats"] = event.repeats;
    normalized["radio_id"] = event.radioId;
    normalized["raw_pulses"] = event.rawPulseCount;
    normalized["raw_duration_us"] = event.rawDurationUs;
    normalized["captured_at_ms"] = event.capturedAtMs;
    normalized["frequency_mhz"] = event.frequencyMHz;
    normalized["rssi_dbm"] = event.rssiDbm;
  }

  JsonObject lastKnown = protocol["v2_last_known_event"].to<JsonObject>();
  lastKnown["available"] = protocolDiag.v2LastKnownEventAvailable;
  if (protocolDiag.v2LastKnownEventAvailable) {
    const NormalizedRfEvent& event = protocolDiag.v2LastKnownEvent;
    lastKnown["protocol"] = protocolDiagnosticsV2ProtocolName(event.protocol);
    lastKnown["code"] = uint64Hex(event.code);
    lastKnown["symbol_count"] = event.symbolCount;
    lastKnown["repeats"] = event.repeats;
    lastKnown["radio_id"] = event.radioId;
    lastKnown["frequency_mhz"] = event.frequencyMHz;
    lastKnown["rssi_dbm"] = event.rssiDbm;
    lastKnown["captured_at_ms"] = event.capturedAtMs;
    lastKnown["age_ms"] = millis() - event.capturedAtMs;
  }

  JsonObject dedup = protocol["v2_event_dedup"].to<JsonObject>();
  dedup["state"] = normalizedEventDedupStateName(protocolDiag.v2Dedup.state);
  dedup["window_ms"] = normalizedEventDedupWindowMs(protocolDiag.v2NormalizedEvent.protocol);
  dedup["nvkp_window_ms"] = kNvkp01EventDedupWindowMs;
  dedup["delta_ms"] = protocolDiag.v2Dedup.deltaMs;
  dedup["collapsed_in_burst"] = protocolDiag.v2Dedup.collapsedInBurst;
  dedup["normalized_input_count"] = protocolDiag.v2NormalizedEventCount;
  dedup["logical_event_count"] = protocolDiag.v2LogicalEventCount;
  dedup["collapsed_count"] = protocolDiag.v2CollapsedEventCount;

  JsonObject actionable = protocol["v2_actionable_dry_run"].to<JsonObject>();
  actionable["state"] =
      v2ActionableDryRunStateName(protocolDiag.v2ActionableDryRun.state);
  actionable["available"] = protocolDiag.v2ActionableDryRun.available;
  actionable["rx_slot_candidate"] =
      protocolDiag.v2ActionableDryRun.rxSlotCandidate;
  actionable["mqtt_candidate"] =
      protocolDiag.v2ActionableDryRun.mqttCandidate;
  actionable["home_assistant_candidate"] =
      protocolDiag.v2ActionableDryRun.homeAssistantCandidate;
  actionable["would_emit_count"] =
      protocolDiag.v2ActionableWouldEmitCount;
  actionable["suppressed_duplicate_count"] =
      protocolDiag.v2ActionableSuppressedDuplicateCount;
  if (protocolDiag.v2ActionableDryRun.available) {
    const NormalizedRfEvent& event = protocolDiag.v2ActionableDryRun.event;
    actionable["protocol"] = protocolDiagnosticsV2ProtocolName(event.protocol);
    actionable["code"] = uint64Hex(event.code);
    actionable["radio_id"] = event.radioId;
  }

  const V2AuthoritativeStatus authoritativeStatus =
      v2AuthoritativeActionGetStatus();
  JsonObject authoritative =
      protocol["v2_authoritative_action"].to<JsonObject>();
  authoritative["enabled"] = true;
  authoritative["persisted"] = true;
  authoritative["mode"] = "V2_ONLY";
  authoritative["boot_default"] = "V2_ONLY";
  authoritative["legacy_decode_policy"] = "RETIRED";
  authoritative["route_state"] =
      v2AuthoritativeRouteStateName(authoritativeStatus.lastState);
  authoritative["last_protocol"] =
      protocolDiagnosticsV2ProtocolName(authoritativeStatus.lastProtocol);
  authoritative["last_code"] =
      authoritativeStatus.lastProtocol != ProtocolId::UNKNOWN
          ? uint64Hex(authoritativeStatus.lastCode)
          : String();
  authoritative["v2_emit_count"] = authoritativeStatus.v2EmitCount;
  authoritative["duplicate_suppressed_count"] =
      authoritativeStatus.duplicateSuppressedCount;
  authoritative["unknown_no_action_count"] =
      authoritativeStatus.unknownNoActionCount;
  authoritative["unsupported_no_action_count"] =
      authoritativeStatus.unsupportedNoActionCount;
  authoritative["nvkp_confirmation_pending_count"] =
      authoritativeStatus.nvkpConfirmationPendingCount;
  authoritative["approved_ev1527"] = true;
  authoritative["approved_pt2262"] = true;
  authoritative["approved_ht12e"] = true;
  authoritative["approved_nvkp01"] = true;

  const RawSlotMatcherDiagnostics rawMatcherStatus =
      rawSlotMatcherGetDiagnostics();
  JsonObject rawMatcher = protocol["learned_raw_matcher"].to<JsonObject>();
  rawMatcher["available"] = rawMatcherStatus.available;
  rawMatcher["state"] = rawSlotMatchStateName(rawMatcherStatus.lastState);
  rawMatcher["slot"] = rawMatcherStatus.lastSlot;
  rawMatcher["similarity"] = rawMatcherStatus.lastSimilarity;
  rawMatcher["timing_similarity"] = rawMatcherStatus.lastTimingSimilarity;
  rawMatcher["count_similarity"] = rawMatcherStatus.lastCountSimilarity;
  rawMatcher["sign_agreement"] = rawMatcherStatus.lastSignAgreement;
  rawMatcher["compared_pulses"] = rawMatcherStatus.lastComparedPulses;
  rawMatcher["learned_pattern_pulses"] = rawMatcherStatus.learnedPatternPulses;
  rawMatcher["incoming_pattern_pulses"] = rawMatcherStatus.incomingPatternPulses;
  rawMatcher["learned_repeat_reduced"] = rawMatcherStatus.learnedRepeatReduced;
  rawMatcher["incoming_repeat_reduced"] = rawMatcherStatus.incomingRepeatReduced;
  rawMatcher["match_count"] = rawMatcherStatus.matchCount;
  rawMatcher["no_match_count"] = rawMatcherStatus.noMatchCount;
  rawMatcher["duplicate_suppressed_count"] =
      rawMatcherStatus.duplicateSuppressedCount;

  JsonObject nvkp = protocol["v2_nvkp01"].to<JsonObject>();
  nvkp["available"] = protocolDiag.nvkp01DiagnosticsAvailable;
  nvkp["result"] = protocolDiagnosticsV2StatusName(protocolDiag.nvkp01Status);
  if (protocolDiag.nvkp01DiagnosticsAvailable) {
    const Nvkp01DecodeDiagnostics& d = protocolDiag.nvkp01;
    nvkp["reject_reason"] = nvkp01RejectReasonName(d.rejectReason);
    nvkp["pulse_count"] = d.pulseCount;
    nvkp["duration_us"] = d.durationUs;
    nvkp["alternating"] = d.alternating;
    nvkp["canonical_pulse_count"] = d.canonicalPulseCount;
    nvkp["merged_pulses"] = d.mergedPulses;
    nvkp["structure"] = d.syncStructure ? "SYNC_SMQ" :
        (d.compactStructure ? "COMPACT_CELLS" : "NONE");
    nvkp["marker_pairs"] = d.markerPairs;
    nvkp["sync_pulses"] = d.syncPulses;
    nvkp["full_leader"] = d.fullLeader;
    nvkp["code_available"] = d.codeAvailable;
    nvkp["code"] = d.codeAvailable ? uint64Hex(d.normalizedCode) : String();
    nvkp["repeats"] = d.repeatCount;
  }
  JsonObject nvReject = nvkp["reject_counts"].to<JsonObject>();
  nvReject["capture_envelope"] = protocolDiag.nvkp01RejectCounts[static_cast<size_t>(Nvkp01RejectReason::CAPTURE_ENVELOPE)];
  nvReject["polarity_sequence"] = protocolDiag.nvkp01RejectCounts[static_cast<size_t>(Nvkp01RejectReason::POLARITY_SEQUENCE)];
  nvReject["marker_structure"] = protocolDiag.nvkp01RejectCounts[static_cast<size_t>(Nvkp01RejectReason::MARKER_STRUCTURE)];


  JsonObject ht = protocol["v2_ht12e"].to<JsonObject>();
  ht["available"] = protocolDiag.ht12eDiagnosticsAvailable;
  ht["result"] = protocolDiagnosticsV2StatusName(protocolDiag.ht12eStatus);
  if (protocolDiag.ht12eDiagnosticsAvailable) {
    const Ht12eDecodeDiagnostics& d = protocolDiag.ht12e;
    ht["reject_reason"] = ht12eRejectReasonName(d.rejectReason);
    ht["pulse_count"] = d.pulseCount;
    ht["duration_us"] = d.durationUs;
    ht["alternating"] = d.alternating;
    ht["candidate_words"] = d.candidateWords;
    ht["valid_words"] = d.validWords;
    ht["matching_words"] = d.matchingWords;
    ht["estimated_t_us"] = d.estimatedTUs;
    ht["pilot_min_us"] = d.pilotMinUs;
    ht["pilot_max_us"] = d.pilotMaxUs;
    ht["short_min_us"] = d.shortMinUs;
    ht["short_max_us"] = d.shortMaxUs;
    ht["long_min_us"] = d.longMinUs;
    ht["long_max_us"] = d.longMaxUs;
    ht["code_available"] = d.codeAvailable;
    ht["code"] = d.codeAvailable ? uint64Hex(d.decodedWord) : String();
    ht["address"] = d.address;
    ht["data"] = d.data;
    ht["repeats"] = d.repeatCount;
  }
  JsonObject htReject = ht["reject_counts"].to<JsonObject>();
  htReject["capture_envelope"] = protocolDiag.ht12eRejectCounts[static_cast<size_t>(Ht12eRejectReason::CAPTURE_ENVELOPE)];
  htReject["polarity_sequence"] = protocolDiag.ht12eRejectCounts[static_cast<size_t>(Ht12eRejectReason::POLARITY_SEQUENCE)];
  htReject["pilot_sync"] = protocolDiag.ht12eRejectCounts[static_cast<size_t>(Ht12eRejectReason::PILOT_SYNC)];
  htReject["symbol_timing"] = protocolDiag.ht12eRejectCounts[static_cast<size_t>(Ht12eRejectReason::SYMBOL_TIMING)];
  htReject["repeat_mismatch"] = protocolDiag.ht12eRejectCounts[static_cast<size_t>(Ht12eRejectReason::REPEAT_MISMATCH)];

  JsonObject ev1527 = protocol["ev1527"].to<JsonObject>();
  ev1527["available"] = protocolDiag.ev1527DiagnosticsAvailable;
  if (protocolDiag.ev1527DiagnosticsAvailable) {
    const Ev1527DecodeDiagnostics& d = protocolDiag.ev1527;
    const Ev1527DecoderLimits& limits = ev1527DecoderLimits();
    ev1527["reject_reason"] = ev1527RejectReasonName(d.rejectReason);
    ev1527["candidate_frames"] = d.candidateFrameCount;
    ev1527["valid_frames"] = d.validFrameCount;
    ev1527["first_valid_frame"] = d.firstValidFrame;
    ev1527["first_failing_frame"] = d.firstFailingFrame;
    ev1527["decoded_code"] =
        d.codeAvailable ? uint64Hex(d.decodedCode) : String();
    ev1527["base_t_us"] = d.estimatedBasePulseUs;
    ev1527["short_range_available"] = d.pulseRangeAvailable;
    ev1527["short_min_us"] = d.observedShortMinUs;
    ev1527["short_max_us"] = d.observedShortMaxUs;
    ev1527["long_min_us"] = d.observedLongMinUs;
    ev1527["long_max_us"] = d.observedLongMaxUs;
    ev1527["ratio_min"] = d.observedRatioMinX100 / 100.0F;
    ev1527["ratio_max"] = d.observedRatioMaxX100 / 100.0F;
    ev1527["sync_low_available"] = d.syncLowAvailable;
    ev1527["sync_low_t"] = d.observedSyncLowTX100 / 100.0F;
    ev1527["repeat_frames"] = d.repeatFrameCount;
    ev1527["matching_repeats"] = d.matchingRepeatCount;
    ev1527["repeat_deviation_pct"] =
        d.maximumRepeatBaseDeviationX10Percent / 10.0F;
    ev1527["address_check"] = ev1527CheckStateName(d.addressPattern);
    ev1527["command_check"] = ev1527CheckStateName(d.commandPattern);

    JsonObject allowed = ev1527["allowed"].to<JsonObject>();
    allowed["base_t_min_us"] = limits.minimumBasePulseUs;
    allowed["base_t_max_us"] = limits.maximumBasePulseUs;
    allowed["short_tolerance_pct"] = limits.shortTolerancePercent;
    allowed["long_tolerance_pct"] = limits.longTolerancePercent;
    allowed["pair_tolerance_pct"] = limits.pairTolerancePercent;
    allowed["sync_high_tolerance_pct"] = limits.syncHighTolerancePercent;
    allowed["ratio_min"] = limits.minimumLongShortRatioX100 / 100.0F;
    allowed["ratio_max"] = limits.maximumLongShortRatioX100 / 100.0F;
    allowed["sync_low_t_min"] = limits.minimumSyncLowTX100 / 100.0F;
    allowed["sync_low_t_max"] = limits.maximumSyncLowTX100 / 100.0F;
    allowed["repeat_deviation_max_pct"] =
        limits.maximumRepeatBaseDeviationPercent;
    allowed["bits_per_frame"] = limits.expectedBitsPerFrame;
  }

  JsonObject rejectCounts = ev1527["reject_counts"].to<JsonObject>();
  for (size_t index = 1; index < kEv1527RejectReasonCount; ++index) {
    rejectCounts[ev1527RejectReasonName(
        static_cast<Ev1527RejectReason>(index))] =
        protocolDiag.ev1527RejectCounts[index];
  }

  JsonObject pt2262 = protocol["pt2262"].to<JsonObject>();
  pt2262["available"] = protocolDiag.pt2262DiagnosticsAvailable;
  pt2262["result"] = protocolDiagnosticsV2StatusName(protocolDiag.pt2262Status);
  if (protocolDiag.pt2262DiagnosticsAvailable) {
    const Pt2262DecodeDiagnostics& d = protocolDiag.pt2262;
    const Pt2262DecoderLimits& limits = pt2262DecoderLimits();
    pt2262["reject_reason"] = pt2262RejectReasonName(d.rejectReason);
    pt2262["candidate_frames"] = d.candidateFrameCount;
    pt2262["valid_frames"] = d.validFrameCount;
    pt2262["first_valid_frame"] = d.firstValidFrame;
    pt2262["first_failing_frame"] = d.firstFailingFrame;
    pt2262["decoded_code"] =
        d.codeAvailable ? uint64Hex(d.decodedCode) : String();
    pt2262["decoded_trits"] = d.decodedTritCount;
    pt2262["zero_trits"] = d.zeroTritCount;
    pt2262["one_trits"] = d.oneTritCount;
    pt2262["floating_trits"] = d.floatingTritCount;
    pt2262["base_t_us"] = d.estimatedBasePulseUs;
    pt2262["pulse_range_available"] = d.pulseRangeAvailable;
    pt2262["short_min_us"] = d.observedShortMinUs;
    pt2262["short_max_us"] = d.observedShortMaxUs;
    pt2262["long_min_us"] = d.observedLongMinUs;
    pt2262["long_max_us"] = d.observedLongMaxUs;
    pt2262["ratio_min"] = d.observedRatioMinX100 / 100.0F;
    pt2262["ratio_max"] = d.observedRatioMaxX100 / 100.0F;
    pt2262["sync_low_available"] = d.syncLowAvailable;
    pt2262["sync_low_t"] = d.observedSyncLowTX100 / 100.0F;
    pt2262["repeat_frames"] = d.repeatFrameCount;
    pt2262["matching_repeats"] = d.matchingRepeatCount;
    pt2262["repeat_deviation_pct"] =
        d.maximumRepeatBaseDeviationX10Percent / 10.0F;

    JsonObject allowed = pt2262["allowed"].to<JsonObject>();
    allowed["base_t_min_us"] = limits.minimumBasePulseUs;
    allowed["base_t_max_us"] = limits.maximumBasePulseUs;
    allowed["classification_tolerance_pct"] =
        limits.classificationTolerancePercent;
    allowed["ratio_min"] = limits.minimumLongShortRatioX100 / 100.0F;
    allowed["ratio_max"] = limits.maximumLongShortRatioX100 / 100.0F;
    allowed["sync_low_t_min"] = limits.minimumSyncLowTX100 / 100.0F;
    allowed["sync_low_t_max"] = limits.maximumSyncLowTX100 / 100.0F;
    allowed["repeat_deviation_max_pct"] =
        limits.maximumRepeatBaseDeviationPercent;
    allowed["trits_per_frame"] = limits.expectedTritsPerFrame;
  }
  JsonObject ptRejectCounts = pt2262["reject_counts"].to<JsonObject>();
  for (size_t index = 1; index < kPt2262RejectReasonCount; ++index) {
    ptRejectCounts[pt2262RejectReasonName(
        static_cast<Pt2262RejectReason>(index))] =
        protocolDiag.pt2262RejectCounts[index];
  }

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
  output += ",\"frequency_mhz\":" + String(info.frequencyMHz, 3);
  output += ",\"radio_id\":" + String(info.radioId);
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
  doc["frequency_mhz"] = info.frequencyMHz;
  doc["radio_id"] = info.radioId;
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
  if (!rfCommandStartLearn()) {
    sendJsonError(409, "Radio is not ready for learning");
    return;
  }
  sendSuccess("Learning started. Press the remote button once.");
}

void handleLearnAcceptApi() {
  if (!rfCommandAcceptLearn()) {
    sendJsonError(409, "No preview is ready to accept");
    return;
  }
  sendSuccess("Capture accepted in RAM. Persistent slot storage is not enabled yet.");
}

void handleLearnDiscardApi() {
  if (!rfCommandDiscardLearn()) {
    sendJsonError(409, "There is no active Learn capture");
    return;
  }
  sendSuccess("Learn capture discarded");
}


void handleLearnTestSendApi() {
  if (!rfCommandTestLearnTx(config.replayCount)) {
    sendJsonError(409, "No valid Learn preview is ready, or TX failed");
    return;
  }
  sendSuccess("Test transmission completed and RAW receiver restored");
}


void handleAnalyzerApi() {
  const uint32_t analyzerApiStartedUs = micros();
  analyzerFullApiCalls++;

  uint8_t radioId = 1;
  if (server.hasArg("radio")) {
    radioId = static_cast<uint8_t>(server.arg("radio").toInt());
  }
  if (radioId != 1 && radioId != 2) radioId = 1;
  // Build a bounded snapshot and stream it directly to the client.
  // Keep the proven v1.2.0 response shape during the ESP32-S3 port.
  const bool developerMode = config.analyzerDeveloperMode;

  // When Analyzer is disabled, return a small status document. Gateway
  // operation is unaffected by the Analyzer switch on ESP32-S3.
  if (!developerMode) {
    char disabledJson[384];
    snprintf(disabledJson, sizeof(disabledJson),
             "{\"available\":false,\"analyzer_disabled\":true,"
             "\"analyzer_developer_mode\":false,"
             "\"status\":\"Analyzer disabled - gateway remains active\","
             "\"radio_id\":%u,\"frequency_mhz\":%.3f,\"current_rssi_dbm\":%.1f,"
             "\"heap_free\":%lu,\"heap_max_block\":%lu}",
             static_cast<unsigned int>(radioId),
             Radio.getOperatingFrequency(radioId), Radio.getRadioRSSI(radioId),
             static_cast<unsigned long>(ESP.getFreeHeap()),
             static_cast<unsigned long>(openrfMaxFreeBlock()));
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", disabledJson);
    return;
  }

  const AnalyzerSnapshot a = analyzerGetSnapshot(radioId);
  const AnalyzerCandidateSnapshot c = analyzerGetLastCandidate(radioId);
  // Keep the proven v1.2.0 Analyzer response size for this first cleanup step.
  // Only the ESP8266 low-heap pause is removed here; buffer/response expansion
  // will be handled separately after PSRAM-aware testing.
  constexpr uint16_t DEVELOPER_RAW_LIMIT = 96;
  const uint16_t rawLimit = DEVELOPER_RAW_LIMIT;
  const uint32_t freeHeap = ESP.getFreeHeap();
  const uint32_t maxBlock = openrfMaxFreeBlock();

  JsonDocument doc;
  doc["available"] = a.available;
  doc["radio_id"] = radioId;
  doc["sequence"] = a.sequence;
  doc["age_ms"] = a.available ? millis() - a.capturedAtMs : 0;
  doc["processing_us"] = a.processingUs;
  doc["frequency_mhz"] = a.frequencyMHz;
  doc["operating_frequency_mhz"] = Radio.getOperatingFrequency(radioId);
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
  const RadioChannelDiagnostics channelDiag =
      Radio.getChannelDiagnostics(radioId);
  doc["raw_candidates"] = channelDiag.rawCandidates;
  doc["accepted_frames"] = channelDiag.acceptedFrames;
  doc["rejected_frames"] = channelDiag.rejectedFrames;
  // These low-level counters remain aggregate diagnostics for now.
  doc["background_filtered_frames"] = d.backgroundFilteredFrames;
  doc["ignored_glitch_edges"] = d.ignoredGlitchEdges;
  doc["gap_finalized_frames"] = d.gapFinalizedFrames;
  doc["timeout_finalized_frames"] = d.timeoutFinalizedFrames;
  doc["buffer_full_frames"] = d.bufferFullFrames;
  doc["merged_same_sign_pulses"] = d.mergedSameSignPulses;
  doc["weak_rssi_frames"] = a.weakRssiFrames;
  doc["analyzer_min_rssi"] = config.analyzerMinRssi;
  doc["current_rssi_dbm"] = Radio.getRadioRSSI(radioId);
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
  doc["api_build_us"] = static_cast<uint32_t>(micros() - analyzerApiStartedUs);
  doc["server_uptime_ms"] = millis();

  JsonObject candidate = doc["last_candidate"].to<JsonObject>();
  candidate["available"] = c.available;
  candidate["sequence"] = c.sequence;
  candidate["age_ms"] = c.available ? millis() - c.capturedAtMs : 0;
  candidate["frequency_mhz"] = c.frequencyMHz;
  candidate["radio_id"] = c.radioId;
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

  // Step 16: build the Analyzer JSON once in memory, then send it as one
  // normal WebServer response instead of streaming ArduinoJson directly
  // into WiFiClient with many small writes.
  const size_t contentLength = measureJson(doc);
  String output;
  output.reserve(contentLength + 1);
  serializeJson(doc, output);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", output);
}


void handleAnalyzerLiveApi() {
  analyzerLiveApiCalls++;

  uint8_t radioId = 1;
  if (server.hasArg("radio")) {
    radioId = static_cast<uint8_t>(server.arg("radio").toInt());
  }
  if (radioId != 1 && radioId != 2) radioId = 1;

  const bool developerMode = config.analyzerDeveloperMode;
  const AnalyzerLiveState state = analyzerGetLiveState(radioId);

  JsonDocument doc;
  doc["enabled"] = developerMode;
  doc["radio_id"] = radioId;
  doc["frequency_mhz"] = Radio.getOperatingFrequency(radioId);
  doc["current_rssi_dbm"] = Radio.getRadioRSSI(radioId);
  doc["available"] = state.available;
  doc["sequence"] = state.sequence;
  doc["age_ms"] = state.available ? millis() - state.capturedAtMs : 0;
  doc["candidate_available"] = state.candidateAvailable;
  doc["candidate_sequence"] = state.candidateSequence;
  doc["candidate_age_ms"] = state.candidateAvailable ? millis() - state.candidateCapturedAtMs : 0;
  doc["peak_rssi_dbm"] = state.currentPeakRssiDbm;
  doc["weak_rssi_frames"] = state.weakRssiFrames;

  server.sendHeader("Cache-Control", "no-store");
  String output;
  output.reserve(192);
  serializeJson(doc, output);
  server.send(200, "application/json", output);
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
    slotDoc["radio_id"] = info.radioId;
    bool tuned = false;
    if (info.radioId == 1 || info.radioId == 2) {
      tuned = fabsf(info.frequencyMHz - Radio.getDefaultFrequency(info.radioId)) > 0.0005F;
    }
    slotDoc["frequency_tuned"] = tuned;
    slotDoc["pulse_count"] = info.pulseCount;
    slotDoc["duration_us"] = info.durationUs;
    slotDoc["fingerprint"] = info.fingerprint;
    const RawSlotMatchStats rawStats = rawSlotMatcherGetStats(i);
    slotDoc["rx_match_count"] = rawStats.matchCount;
    slotDoc["rx_last_similarity"] = rawStats.lastSimilarity;
    slotDoc["rx_last_rssi"] = rawStats.lastRssi;
    slotDoc["rx_last_age_ms"] = rawStats.lastMatchedAtMs > 0U
                                     ? static_cast<uint32_t>(millis() - rawStats.lastMatchedAtMs)
                                     : 0U;

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

void handleSlotStatsApi() {
  JsonDocument doc;
  JsonArray slots = doc["slots"].to<JsonArray>();
  for (uint8_t slot = 1U; slot <= OPENRF_SLOT_COUNT; ++slot) {
    const RawSlotMatchStats stats = rawSlotMatcherGetStats(slot);
    if (!stats.available) continue;
    JsonObject item = slots.add<JsonObject>();
    item["id"] = slot;
    item["match_count"] = stats.matchCount;
    item["last_similarity"] = stats.lastSimilarity;
    item["last_rssi"] = stats.lastRssi;
    item["last_age_ms"] = stats.lastMatchedAtMs > 0U
                              ? static_cast<uint32_t>(millis() - stats.lastMatchedAtMs)
                              : 0U;
  }
  const RawSlotMatcherDiagnostics d = rawSlotMatcherGetDiagnostics();
  JsonObject matcher = doc["matcher"].to<JsonObject>();
  matcher["available"] = d.available;
  matcher["state"] = rawSlotMatchStateName(d.lastState);
  matcher["slot"] = d.lastSlot;
  matcher["similarity"] = d.lastSimilarity;
  matcher["timing_similarity"] = d.lastTimingSimilarity;
  matcher["count_similarity"] = d.lastCountSimilarity;
  matcher["sign_agreement"] = d.lastSignAgreement;
  matcher["compared_pulses"] = d.lastComparedPulses;
  matcher["learned_pattern_pulses"] = d.learnedPatternPulses;
  matcher["incoming_pattern_pulses"] = d.incomingPatternPulses;
  matcher["learned_repeat_reduced"] = d.learnedRepeatReduced;
  matcher["incoming_repeat_reduced"] = d.incomingRepeatReduced;
  matcher["match_count"] = d.matchCount;
  matcher["no_match_count"] = d.noMatchCount;
  matcher["duplicate_suppressed_count"] = d.duplicateSuppressedCount;
  sendJsonDoc(200, doc);
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
  if (!storageSaveSlot(slot, name, capture.frequencyMHz, capture.radioId,
                       openrfScratch, count, capture.durationUs, &fingerprint)) {
    sendJsonError(500, "Failed to save slot to LittleFS"); return;
  }
  rawSlotMatcherReload(slot);
  JsonDocument response;
  response["success"] = true;
  response["message"] = "Signal saved to slot " + String(slot);
  response["slot"] = slot;
  response["fingerprint"] = fingerprint;
  response["radio_id"] = capture.radioId;
  response["frequency_mhz"] = serialized(String(capture.frequencyMHz, 4));
  String output; serializeJson(response, output); server.send(200, "application/json", output);
  if (mqttIsConnected() && config.homeAssistantDiscovery) mqttPublishDiscovery();
  Serial.print("SLOT saved: "); Serial.print(slot); Serial.print(", R"); Serial.print(capture.radioId);
  Serial.print(" @ "); Serial.print(capture.frequencyMHz, 4); Serial.print(" MHz, "); Serial.print(count);
  Serial.print(" pulses, fingerprint "); Serial.println(fingerprint, HEX);
}

void handleSlotSendApi() {
  JsonDocument doc; uint8_t slot;
  if (!readSlotRequest(doc, slot)) return;
  SlotInfo info;
  if (!storageLoadSlot(slot, openrfScratch, OPENRF_MAX_RAW_PULSES, info)) { sendJsonError(404, "Slot is empty or invalid"); return; }
  // Step 27: RF Slot TX uses the slot's stored Radio + Learned frequency.
  // The RF core retunes only for the transmission and restores the radio's
  // current Operating frequency (including an active TUNED session) afterwards.
  const uint8_t radioId = (info.radioId == 1 || info.radioId == 2)
                              ? info.radioId
                              : (info.frequencyMHz >= 700.0F ? 2 : 1);
  if (!rfCommandSendRawTuned(openrfScratch, info.pulseCount, config.replayCount,
                             radioId, info.frequencyMHz)) {
    sendJsonError(500, "RF transmission failed");
    return;
  }
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
  rawSlotMatcherClear(slot);
  sendSuccess("Slot " + String(slot) + " deleted");
  if (mqttIsConnected() && config.homeAssistantDiscovery) mqttPublishDiscovery();
  Serial.print("SLOT deleted: "); Serial.println(slot);
}


void handleRxSlotsApi() {
  const ProtocolTxDiagnostics tx = protocolTxDiagnosticsSnapshot();
  const uint32_t txAgeMs = tx.available
                               ? static_cast<uint32_t>(millis() - tx.timestampMs)
                               : 0U;
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  server.sendContent("{\"count\":" + String(OPENRF_RX_SLOT_COUNT) + ",\"used_count\":" + String(rxSlotCountUsed()) +
                     ",\"learn_state\":\"" + String(rxSlotLearnState()) +
                     "\",\"learn_source\":\"" + String(rxSlotLearnSource()) +
                     "\",\"learning_slot\":" + String(rxSlotLearningId()) +
                     ",\"learn_min_rssi\":" + String(config.rxSlotLearnMinRssi) +
                     ",\"weak_rejected\":" + String(rxSlotWeakLearnRejectedCount()) +
                     ",\"last_weak_rssi\":" + String(rxSlotLastWeakLearnRssi(), 1) +
                     ",\"v2_tx_available\":" + String(tx.available ? "true" : "false") +
                     ",\"v2_tx_protocol\":\"" + String(tx.available ? protocolDiagnosticsV2ProtocolName(tx.protocol) : "—") +
                     "\",\"v2_tx_code\":\"" + String(tx.available ? uint64Hex(tx.code) : "—") +
                     "\",\"v2_tx_radio\":" + String(tx.available ? tx.radioId : 0U) +
                     ",\"v2_tx_frequency_mhz\":" + String(tx.available ? tx.frequencyMHz : 0.0F, 4) +
                     ",\"v2_tx_result\":\"" + String(tx.available ? (tx.success ? "SUCCESS" : "FAILURE") : "N/A") +
                     "\",\"v2_tx_repeats\":" + String(tx.available ? tx.repeatCount : 0U) +
                     ",\"v2_tx_failure\":\"" + String(tx.available ? protocolTxFailureReasonName(tx.failureReason) : "NONE") +
                     "\",\"v2_tx_age_ms\":" + String(txAgeMs) +
                     ",\"slots\":[");
  for (uint8_t i=1;i<=OPENRF_RX_SLOT_COUNT;i++) {
    if (i>1) server.sendContent(",");
    RxSlotInfo x=rxSlotGetInfo(i); JsonDocument d;
    d["id"]=i; d["used"]=x.used; d["enabled"]=x.enabled; d["name"]=x.name;
    d["protocol"]=x.protocol; d["symbol_count"]=x.symbolCount;
    d["device_id"]=x.deviceId; d["command"]=x.command; d["code"]=x.code;
    d["match_code"]=x.matchCode; d["pulse_length_us"]=x.pulseLengthUs;
    d["radio_id"]=x.radioId; d["frequency_mhz"]=serialized(String(x.frequencyMHz, 4));
    d["send_supported"]=x.sendSupported;
    d["match_count"]=x.matchCount; d["last_quality"]=x.lastQuality; d["last_rssi"]=x.lastRssi;
    String out; serializeJson(d,out); server.sendContent(out); yield();
  }
  server.sendContent("]}");
}
void handleRxLearnApi() {
  JsonDocument body; if (deserializeJson(body, server.arg("plain"))) { sendJsonError(400,"Invalid JSON"); return; }
  uint8_t slot=body["slot"]|0; String defaultName = String("RX Slot ") + String(slot); String name=body["name"]|defaultName;
  if (!rxSlotStartLearn(slot,name)) { sendJsonError(409,"RX learn is busy or unavailable"); return; }
  JsonDocument d; d["ok"]=true; d["message"]="Waiting for RF signal"; sendJsonDoc(200,d);
}
void handleRxLearnRssiApi() {
  JsonDocument body;
  if (deserializeJson(body, server.arg("plain"))) {
    sendJsonError(400, "Invalid JSON");
    return;
  }

  const int value = body["min_rssi"] | -75;
  if (value < -100 || value > -20) {
    sendJsonError(400, "RX Slot Learn RSSI must be between -100 and -20 dBm");
    return;
  }

  config.rxSlotLearnMinRssi = static_cast<int8_t>(value);
  if (!configSave()) {
    sendJsonError(500, "Failed to save RX Slot Learn RSSI");
    return;
  }

  JsonDocument response;
  response["ok"] = true;
  response["min_rssi"] = config.rxSlotLearnMinRssi;
  response["message"] = "RX Slot Learn RSSI saved";
  sendJsonDoc(200, response);
}
void handleRxDeleteApi(){ JsonDocument b;if(deserializeJson(b,server.arg("plain"))){sendJsonError(400,"Invalid JSON");return;} uint8_t slot=b["slot"]|0; bool ok=rxSlotDelete(slot); if(ok&&config.homeAssistantDiscovery)mqttPublishDiscovery(); JsonDocument d;d["ok"]=ok;d["message"]=ok?"RX slot deleted":"Delete failed";sendJsonDoc(ok?200:400,d);}
void handleRxRenameApi(){ JsonDocument b;if(deserializeJson(b,server.arg("plain"))){sendJsonError(400,"Invalid JSON");return;} uint8_t slot=b["slot"]|0;String name=b["name"]|"";bool ok=rxSlotRename(slot,name);if(ok&&config.homeAssistantDiscovery)mqttPublishDiscovery();JsonDocument d;d["ok"]=ok;d["message"]=ok?"RX slot renamed":"Rename failed";sendJsonDoc(ok?200:400,d);}
void handleRxEnableApi(){ JsonDocument b;if(deserializeJson(b,server.arg("plain"))){sendJsonError(400,"Invalid JSON");return;}uint8_t slot=b["slot"]|0;bool enabled=b["enabled"]|false;bool ok=rxSlotSetEnabled(slot,enabled);if(ok&&config.homeAssistantDiscovery)mqttPublishDiscovery();JsonDocument d;d["ok"]=ok;d["message"]=ok?(enabled?"RX slot enabled":"RX slot disabled"):"Update failed";sendJsonDoc(ok?200:400,d);}
void handleRxSendApi(){ JsonDocument b;if(deserializeJson(b,server.arg("plain"))){sendJsonError(400,"Invalid JSON");return;}uint8_t slot=b["slot"]|0;RxSlotInfo info=rxSlotGetInfo(slot);if(!info.used){sendJsonError(404,"RX slot is empty or invalid");return;}if(!info.sendSupported){sendJsonError(409,"This RX slot protocol cannot be reproduced yet");return;}if(!rxSlotSend(slot,config.replayCount)){sendJsonError(500,"RX slot transmission failed");return;}JsonDocument d;d["ok"]=true;d["message"]=String("RX Slot ")+String(slot)+" transmitted";sendJsonDoc(200,d);}

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
  if (replayCount < 1 || replayCount > 10) {
    sendJsonError(400, "Replay count must be between 1 and 10");
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
 WiFiClient client = server.client();
if (!backupStreamToClient(client, error)) {
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
    rfCommandStopReceive();
    const uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace, U_FLASH)) {
      otaUploadMessage = Update.errorString();
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!Update.hasError() && Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      otaUploadMessage = Update.errorString();
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!Update.hasError() && Update.end(true)) {
      otaUploadOk = true;
      otaUploadMessage = "Firmware installed successfully. Restarting...";
    } else {
      otaUploadMessage = Update.errorString();
      Update.printError(Serial);
      rfCommandStartReceive();
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end(false);
    otaUploadMessage = "Firmware upload was aborted";
    rfCommandStartReceive();
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
  server.on("/api/system/radios", HTTP_POST, handleRadioEnableApi);
  server.on("/api/radio", HTTP_GET, handleRadioApi);
  server.on("/api/radio/frequency-scan", HTTP_GET, handleFrequencyScanApi);
  server.on("/api/radio/frequency-tune", HTTP_POST, handleFrequencyTuneApi);
  server.on("/api/radio/frequency-restore", HTTP_POST, handleFrequencyRestoreApi);
  server.on("/api/radio/raw", HTTP_GET, handleRadioRawApi);
  server.on("/api/radio/learn", HTTP_GET, handleLearnStatusApi);
  server.on("/api/radio/learn/raw", HTTP_GET, handleLearnRawApi);
  server.on("/api/radio/learn/start", HTTP_POST, handleLearnStartApi);
  server.on("/api/radio/learn/accept", HTTP_POST, handleLearnAcceptApi);
  server.on("/api/radio/learn/discard", HTTP_POST, handleLearnDiscardApi);
  server.on("/api/radio/learn/test-send", HTTP_POST, handleLearnTestSendApi);
  server.on("/api/debug/radio", HTTP_GET, handleRadioDebugApi);
  server.on("/api/analyzer", HTTP_GET, handleAnalyzerApi);
  server.on("/api/analyzer/live", HTTP_GET, handleAnalyzerLiveApi);
  server.on("/api/analyzer/settings", HTTP_POST, handleAnalyzerSettingsApi);
  server.on("/api/slots", HTTP_GET, handleSlotsApi);
  server.on("/api/slots/stats", HTTP_GET, handleSlotStatsApi);
  server.on("/api/slots/save", HTTP_POST, handleSlotSaveApi);
  server.on("/api/slots/send", HTTP_POST, handleSlotSendApi);
  server.on("/api/slots/rename", HTTP_POST, handleSlotRenameApi);
  server.on("/api/slots/delete", HTTP_POST, handleSlotDeleteApi);
  server.on("/api/rxslots", HTTP_GET, handleRxSlotsApi);
  server.on("/api/rxslots/learn", HTTP_POST, handleRxLearnApi);
  server.on("/api/rxslots/learn-rssi", HTTP_POST, handleRxLearnRssiApi);
  server.on("/api/rxslots/delete", HTTP_POST, handleRxDeleteApi);
  server.on("/api/rxslots/rename", HTTP_POST, handleRxRenameApi);
  server.on("/api/rxslots/enable", HTTP_POST, handleRxEnableApi);
  server.on("/api/rxslots/send", HTTP_POST, handleRxSendApi);
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
