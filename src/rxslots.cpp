#include <Arduino.h>
#include "dualcore.h"
#include "config.h"
#include <LittleFS.h>
#include "rxslots.h"
#include "radio.h"
#include "scratch.h"
#include "universal_decoder.h"

namespace {
constexpr uint32_t MAGIC_V3 = 0x52585033UL; // RXP3
constexpr uint32_t MAGIC_V2 = 0x52585032UL; // RXP2
constexpr uint8_t VERSION_V3 = 3;
// ESP32-S3: the generic per-slot lockout used by the ESP8266 build is removed.
// Every independently decoded actionable frame is allowed through immediately.
// Protocol-specific de-duplication/confirmation (for example NVKP01) remains.
constexpr uint16_t LOCKOUT_MS = 0;
constexpr uint16_t NVKP_CONFIRM_WINDOW_MS = 1000;
constexpr uint16_t NVKP_MIN_PULSES = 28;
constexpr uint16_t NVKP_MAX_PULSES = 70;
constexpr uint32_t NVKP_MIN_DURATION_US = 18000UL;
constexpr uint32_t NVKP_MAX_DURATION_US = 60000UL;
constexpr uint16_t NVKP_MARKER_LOW_MIN_US = 1250;
constexpr uint16_t NVKP_MARKER_LOW_MAX_US = 2100;
constexpr uint16_t NVKP_MARKER_HIGH_MIN_US = 280;
constexpr uint16_t NVKP_MARKER_HIGH_MAX_US = 950;
constexpr uint8_t NVKP_MIN_MARKER_PAIRS = 2;

struct HeaderV3 {
  uint32_t magic;
  uint8_t version;
  uint8_t enabled;
  uint8_t matchCode;
  uint8_t reserved;
  uint8_t symbolCount;
  uint16_t pulseLengthUs;
  char name[33];
  char protocol[25];
  char deviceId[25];
  char command[17];
  char code[25];
};

struct HeaderV2 {
  uint32_t magic;
  uint8_t version;
  uint8_t enabled;
  uint8_t protocol;
  uint8_t symbolCount;
  uint64_t code;
  uint16_t pulseLengthUs;
  char name[33];
};

uint32_t lastMatchedAt[OPENRF_RX_SLOT_COUNT + 1] = {};
uint32_t matchCounts[OPENRF_RX_SLOT_COUNT + 1] = {};
float lastRssi[OPENRF_RX_SLOT_COUNT + 1] = {};
uint8_t lastQuality[OPENRF_RX_SLOT_COUNT + 1] = {};
uint8_t learningSlot = 0;
String learningName;
String learnState = "idle";
uint32_t weakLearnRejected = 0;
float lastWeakLearnRssi = -127.0F;
uint32_t nvkpPendingAtMs = 0;
bool nvkpPending = false;


uint32_t pulseWidth(const int16_t pulse) {
  return static_cast<uint32_t>(abs(static_cast<int32_t>(pulse)));
}

bool isStrictNvkpCapture(const int16_t* pulses, uint16_t count,
                         uint32_t durationUs) {
  if (!pulses || count < NVKP_MIN_PULSES || count > NVKP_MAX_PULSES ||
      durationUs < NVKP_MIN_DURATION_US || durationUs > NVKP_MAX_DURATION_US) {
    return false;
  }

  uint8_t markerPairs = 0;
  for (uint16_t i = 0; i + 1 < count; i++) {
    // A cleaned OOK capture must alternate LOW/HIGH throughout.
    if ((pulses[i] > 0) == (pulses[i + 1] > 0)) return false;

    const uint32_t lowUs = pulseWidth(pulses[i]);
    const uint32_t highUs = pulseWidth(pulses[i + 1]);
    if (pulses[i] < 0 && pulses[i + 1] > 0 &&
        lowUs >= NVKP_MARKER_LOW_MIN_US && lowUs <= NVKP_MARKER_LOW_MAX_US &&
        highUs >= NVKP_MARKER_HIGH_MIN_US && highUs <= NVKP_MARKER_HIGH_MAX_US) {
      if (markerPairs < UINT8_MAX) markerPairs++;
    }
  }
  return markerPairs >= NVKP_MIN_MARKER_PAIRS;
}

bool confirmNvkpEvent(const int16_t* pulses, uint16_t count,
                      uint32_t durationUs, uint32_t nowMs) {
  if (!isStrictNvkpCapture(pulses, count, durationUs)) {
    if (nvkpPending && nowMs - nvkpPendingAtMs > NVKP_CONFIRM_WINDOW_MS) {
      nvkpPending = false;
    }
    return false;
  }

  if (!nvkpPending || nowMs - nvkpPendingAtMs > NVKP_CONFIRM_WINDOW_MS) {
    nvkpPending = true;
    nvkpPendingAtMs = nowMs;
    Serial.println(F("NVKP01 candidate pending confirmation"));
    return false;
  }

  nvkpPending = false;
  Serial.println(F("NVKP01 confirmed by second short capture"));
  return true;
}

String path(uint8_t slot) { return "/rxslot" + String(slot) + ".bin"; }
bool valid(uint8_t slot) { return slot >= 1 && slot <= OPENRF_RX_SLOT_COUNT; }
void copyText(char* dst, size_t size, const String& value) {
  value.substring(0, size - 1).toCharArray(dst, size);
}
String hexCode(uint64_t code) {
  char buffer[19];
  const uint32_t hi = static_cast<uint32_t>(code >> 32);
  const uint32_t lo = static_cast<uint32_t>(code);
  if (hi) snprintf(buffer, sizeof(buffer), "%08X%08X", hi, lo);
  else snprintf(buffer, sizeof(buffer), "%08X", lo);
  String s(buffer); while (s.length() > 1 && s[0] == '0') s.remove(0, 1); return s;
}

bool writeHeader(uint8_t slot, const HeaderV3& h) {
  const String tmp = path(slot) + ".tmp";
  File f = LittleFS.open(tmp, "w");
  if (!f) return false;
  const bool ok = f.write(reinterpret_cast<const uint8_t*>(&h), sizeof(h)) == sizeof(h);
  f.close();
  if (!ok) { LittleFS.remove(tmp); return false; }
  LittleFS.remove(path(slot));
  return LittleFS.rename(tmp, path(slot));
}

bool migrateV2(uint8_t slot, const HeaderV2& old, HeaderV3& h) {
  if (old.magic != MAGIC_V2 || old.version != 2 || !old.symbolCount) return false;
  h = HeaderV3{};
  h.magic = MAGIC_V3; h.version = VERSION_V3; h.enabled = old.enabled;
  h.matchCode = 1; h.reserved = 0; h.symbolCount = old.symbolCount;
  h.pulseLengthUs = old.pulseLengthUs;
  copyText(h.name, sizeof(h.name), String(old.name));
  copyText(h.protocol, sizeof(h.protocol), protocolName(static_cast<OpenRfProtocol>(old.protocol)));
  const String code = hexCode(old.code);
  copyText(h.deviceId, sizeof(h.deviceId), code);
  copyText(h.code, sizeof(h.code), code);
  writeHeader(slot, h);
  return true;
}

bool readHeader(uint8_t slot, HeaderV3& h) {
  if (!valid(slot)) return false;
  File f = LittleFS.open(path(slot), "r");
  if (!f) return false;
  uint32_t magic = 0; f.read(reinterpret_cast<uint8_t*>(&magic), sizeof(magic)); f.seek(0);
  bool ok = false;
  if (magic == MAGIC_V3 && f.size() == sizeof(HeaderV3)) {
    ok = f.read(reinterpret_cast<uint8_t*>(&h), sizeof(h)) == sizeof(h) &&
         h.version == VERSION_V3 && h.protocol[0];
  } else if (magic == MAGIC_V2 && f.size() == sizeof(HeaderV2)) {
    HeaderV2 old{};
    ok = f.read(reinterpret_cast<uint8_t*>(&old), sizeof(old)) == sizeof(old);
    f.close();
    return ok && migrateV2(slot, old, h);
  }
  f.close(); return ok;
}

bool duplicateExists(uint8_t exceptSlot, const DecodedRFEvent& d) {
  for (uint8_t i = 1; i <= OPENRF_RX_SLOT_COUNT; i++) {
    if (i == exceptSlot) continue;
    HeaderV3 h{};
    if (readHeader(i, h) && decodedEventMatches(d, h.protocol, h.deviceId, h.command,
                                                h.code, h.matchCode)) return true;
  }
  return false;
}

bool saveDecoded(uint8_t slot, const String& name, const DecodedRFEvent& d) {
  if (!valid(slot) || !d.valid || duplicateExists(slot, d)) return false;
  HeaderV3 h{};
  h.magic = MAGIC_V3; h.version = VERSION_V3; h.enabled = 1;
  h.matchCode = 1; h.reserved = 0;
  h.symbolCount = d.symbolCount; h.pulseLengthUs = d.pulseLengthUs;
  copyText(h.name, sizeof(h.name), name.length() ? name : ("RX Slot " + String(slot)));
  copyText(h.protocol, sizeof(h.protocol), d.protocol);
  copyText(h.deviceId, sizeof(h.deviceId), d.deviceId);
  copyText(h.command, sizeof(h.command), d.command);
  copyText(h.code, sizeof(h.code), d.code);
  return writeHeader(slot, h);
}
}

void __attribute__((weak)) mqttPublishRxSlotEvent(uint8_t, const RxSlotInfo&) {}

void rxSlotsBegin() {
  Serial.print(F("Universal RX slots loaded: ")); Serial.println(rxSlotCountUsed());
}

void rxSlotsLoop() {
  // Step 3: RF processing is event-driven via rxSlotsHandleRFEvent().
}

void rxSlotsHandleRFEvent(const RFEventMessage& event) {
  if (event.type == RFEventType::LEARN_PREVIEW) {
    if (!learningSlot || event.pulseCount == 0) return;

    // RX Slot Learn has its own absolute RSSI floor in addition to the
    // RadioManager noise-floor + 6 dB rule. Weak captures are ignored and
    // Learn is automatically re-armed instead of saving a bad/noisy frame.
    if (event.rssiDbm < static_cast<float>(config.rxSlotLearnMinRssi)) {
      weakLearnRejected++;
      lastWeakLearnRssi = event.rssiDbm;
      learnState = "waiting_for_signal";

      Serial.print(F("RX learn weak signal ignored: "));
      Serial.print(event.rssiDbm, 1);
      Serial.print(F(" dBm < "));
      Serial.print(config.rxSlotLearnMinRssi);
      Serial.println(F(" dBm"));

      const bool discarded = rfCommandDiscardLearn();
      const bool restarted = discarded && rfCommandStartLearn();
      if (!restarted) {
        learnState = "radio_error";
        learningSlot = 0;
        learningName = "";
      }
      return;
    }

    const DecodedRFEvent decoded =
        universalDecode(event.pulses, event.pulseCount);

    if (!decoded.valid) {
      learnState = "unsupported_protocol";
      learningSlot = 0;
      learningName = "";
      rfCommandDiscardLearn();
      return;
    }

    if (duplicateExists(learningSlot, decoded)) {
      learnState = "duplicate_code";
      learningSlot = 0;
      learningName = "";
      rfCommandDiscardLearn();
      return;
    }

    const uint8_t completedSlot = learningSlot;
    const bool ok = saveDecoded(completedSlot, learningName, decoded);
    learnState = ok ? "saved" : "save_error";

    Serial.print(F("RX learn slot ")); Serial.print(completedSlot);
    Serial.print(ok ? F(" saved: ") : F(" failed: "));
    Serial.print(decoded.protocol); Serial.print(F(" device="));
    Serial.print(decoded.deviceId); Serial.print(F(" command="));
    Serial.println(decoded.command);

    learningSlot = 0;
    learningName = "";
    rfCommandDiscardLearn();
    if (ok) mqttPublishDiscovery();
    return;
  }

  if (event.type != RFEventType::RX_FRAME || event.pulseCount == 0) return;

  const DecodedRFEvent decoded =
      universalDecode(event.pulses, event.pulseCount);
  if (!decoded.valid) return;

  if (decoded.kinetic &&
      decoded.protocol.equalsIgnoreCase("NVKP01 Kinetic") &&
      !confirmNvkpEvent(event.pulses, event.pulseCount,
                        event.durationUs, millis())) {
    return;
  }

  for (uint8_t slot = 1; slot <= OPENRF_RX_SLOT_COUNT; slot++) {
    HeaderV3 h{};
    if (!readHeader(slot, h) || !h.enabled) continue;
    if (!decodedEventMatches(decoded, h.protocol, h.deviceId, h.command,
                             h.code, h.matchCode)) continue;
    if (LOCKOUT_MS && millis() - lastMatchedAt[slot] < LOCKOUT_MS) continue;

    lastMatchedAt[slot] = millis();
    matchCounts[slot]++;
    lastRssi[slot] = event.rssiDbm;
    lastQuality[slot] = decoded.quality;

    const RxSlotInfo info = rxSlotGetInfo(slot);
    Serial.print(F("RX universal match: slot ")); Serial.print(slot);
    Serial.print(F(", ")); Serial.print(decoded.protocol);
    Serial.print(F(", device=")); Serial.print(decoded.deviceId);
    Serial.print(F(", command=")); Serial.println(decoded.command);
    mqttPublishRxSlotEvent(slot, info);
    return;
  }
}

bool rxSlotStartLearn(uint8_t slot, const String& name) {
  if (!valid(slot) || learningSlot || !rfCommandStartLearn()) return false;
  learningSlot = slot;
  learningName = name.length() ? name : ("RX Slot " + String(slot));
  learnState = "waiting_for_signal";
  weakLearnRejected = 0;
  lastWeakLearnRssi = -127.0F;
  return true;
}
bool rxSlotDelete(uint8_t slot) { return valid(slot) && (!LittleFS.exists(path(slot)) || LittleFS.remove(path(slot))); }
bool rxSlotRename(uint8_t slot, const String& name) {
  HeaderV3 h{}; if (!readHeader(slot, h)) return false;
  copyText(h.name, sizeof(h.name), name.length() ? name : ("RX Slot " + String(slot)));
  return writeHeader(slot, h);
}
bool rxSlotSetEnabled(uint8_t slot, bool enabled) {
  HeaderV3 h{}; if (!readHeader(slot, h)) return false; h.enabled = enabled ? 1 : 0; return writeHeader(slot, h);
}
RxSlotInfo rxSlotGetInfo(uint8_t slot) {
  RxSlotInfo info{}; info.id = slot; HeaderV3 h{};
  if (!readHeader(slot, h)) { info.name = "RX Slot " + String(slot); return info; }
  info.used = true; info.enabled = h.enabled; info.name = h.name;
  info.protocol = h.protocol; info.deviceId = h.deviceId; info.command = h.command; info.code = h.code;
  info.matchCode = h.matchCode; info.symbolCount = h.symbolCount;
  info.pulseLengthUs = h.pulseLengthUs; info.matchCount = matchCounts[slot];
  info.lastRssi = lastRssi[slot]; info.lastQuality = lastQuality[slot]; return info;
}
uint8_t rxSlotCountUsed() { uint8_t n=0; for(uint8_t i=1;i<=OPENRF_RX_SLOT_COUNT;i++){HeaderV3 h{};if(readHeader(i,h))n++;}return n; }
const char* rxSlotLearnState() { return learnState.c_str(); }
uint8_t rxSlotLearningId() { return learningSlot; }

uint32_t rxSlotWeakLearnRejectedCount() { return weakLearnRejected; }
float rxSlotLastWeakLearnRssi() { return lastWeakLearnRssi; }
