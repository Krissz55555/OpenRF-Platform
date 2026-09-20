#include <Arduino.h>
#include "dualcore.h"
#include "config.h"
#include <LittleFS.h>
#include "rxslots.h"
#include "radio.h"
#include "scratch.h"
#include "protocol_decoder.h"  // HeaderV2 on-disk migration compatibility only
#include "protocol_decoder_v2.h"
#include "protocol_tx.h"
#include "protocol_tx_diagnostics.h"
#include "protocols/known_protocol_library.h"

namespace {
constexpr uint32_t MAGIC_V4 = 0x52585034UL; // RXP4
constexpr uint32_t MAGIC_V3 = 0x52585033UL; // RXP3
constexpr uint32_t MAGIC_V2 = 0x52585032UL; // RXP2
constexpr uint8_t VERSION_V4 = 4;
// ESP32-S3: the generic per-slot lockout used by the ESP8266 build is removed.
// Every independently decoded actionable frame is allowed through immediately.
// Protocol-specific de-duplication/confirmation (for example NVKP01) remains.
constexpr uint16_t LOCKOUT_MS = 0;
constexpr uint32_t RX_LEARN_TIMEOUT_MS = 45000UL;

struct HeaderV4 {
  uint32_t magic;
  uint8_t version;
  uint8_t enabled;
  uint8_t matchCode;
  uint8_t radioId;
  uint8_t symbolCount;
  uint16_t pulseLengthUs;
  uint32_t frequencyHz;
  char name[33];
  char protocol[25];
  char deviceId[25];
  char command[17];
  char code[25];
};

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
uint32_t learningStartedAtMs = 0;
String learningName;
String learnState = "idle";
String learnSource;
uint32_t weakLearnRejected = 0;
float lastWeakLearnRssi = -127.0F;

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

bool storedProtocolMatchesV2(const char* storedProtocol, ProtocolId protocol) {
  if (!storedProtocol || !storedProtocol[0]) return false;
  const String stored(storedProtocol);
  switch (protocol) {
    case ProtocolId::EV1527_PRINCETON:
      return stored.equalsIgnoreCase("EV1527/Princeton");
    case ProtocolId::PT2262_TRI_STATE:
      // Existing RX slots learned through the legacy path store "PT2262".
      // Accept the V2 display name too for future Step 31+ slot writers.
      return stored.equalsIgnoreCase("PT2262") ||
             stored.equalsIgnoreCase("PT2262/Tri-State");
    case ProtocolId::NVKP01_KINETIC:
      return stored.equalsIgnoreCase("NVKP01 Kinetic") ||
             stored.equalsIgnoreCase("NVKP01");
    case ProtocolId::HT12E:
      return stored.equalsIgnoreCase("HT12E");
    default:
      return false;
  }
}

const char* storedProtocolNameForV2(ProtocolId protocol) {
  switch (protocol) {
    case ProtocolId::EV1527_PRINCETON:
      return "EV1527/Princeton";
    case ProtocolId::PT2262_TRI_STATE:
      // Keep the long-standing on-disk name so older slot files/tooling remain
      // compatible while TX itself is now V2-native.
      return "PT2262";
    case ProtocolId::NVKP01_KINETIC:
      return "NVKP01 Kinetic";
    case ProtocolId::HT12E:
      return "HT12E";
    default:
      return nullptr;
  }
}

ProtocolId v2ProtocolFromStoredName(const String& name) {
  if (name.equalsIgnoreCase("EV1527/Princeton")) return ProtocolId::EV1527_PRINCETON;
  if (name.equalsIgnoreCase("PT2262") ||
      name.equalsIgnoreCase("PT2262/Tri-State")) return ProtocolId::PT2262_TRI_STATE;
  if (name.equalsIgnoreCase("NVKP01 Kinetic") ||
      name.equalsIgnoreCase("NVKP01")) return ProtocolId::NVKP01_KINETIC;
  if (name.equalsIgnoreCase("HT12E")) return ProtocolId::HT12E;
  return ProtocolId::UNKNOWN;
}

bool parseHex64(const String& text, uint64_t& value) {
  if (!text.length() || text.length() > 16) return false;
  value = 0;
  for (size_t i = 0; i < text.length(); ++i) {
    const char c = text[i];
    uint8_t nibble = 0;
    if (c >= '0' && c <= '9') nibble = static_cast<uint8_t>(c - '0');
    else if (c >= 'A' && c <= 'F') nibble = static_cast<uint8_t>(c - 'A' + 10);
    else if (c >= 'a' && c <= 'f') nibble = static_cast<uint8_t>(c - 'a' + 10);
    else return false;
    value = (value << 4) | nibble;
  }
  return true;
}

bool writeHeader(uint8_t slot, const HeaderV4& h) {
  const String tmp = path(slot) + ".tmp";
  File f = LittleFS.open(tmp, "w");
  if (!f) return false;
  const bool ok = f.write(reinterpret_cast<const uint8_t*>(&h), sizeof(h)) == sizeof(h);
  f.close();
  if (!ok) { LittleFS.remove(tmp); return false; }
  LittleFS.remove(path(slot));
  return LittleFS.rename(tmp, path(slot));
}

bool migrateV3(uint8_t slot, const HeaderV3& old, HeaderV4& h) {
  if (old.magic != MAGIC_V3 || old.version != 3 || !old.symbolCount) return false;
  h = HeaderV4{};
  h.magic = MAGIC_V4;
  h.version = VERSION_V4;
  h.enabled = old.enabled;
  h.matchCode = old.matchCode;
  h.radioId = 0;
  h.symbolCount = old.symbolCount;
  h.pulseLengthUs = old.pulseLengthUs;
  h.frequencyHz = 0;
  copyText(h.name, sizeof(h.name), String(old.name));
  copyText(h.protocol, sizeof(h.protocol), String(old.protocol));
  copyText(h.deviceId, sizeof(h.deviceId), String(old.deviceId));
  copyText(h.command, sizeof(h.command), String(old.command));
  copyText(h.code, sizeof(h.code), String(old.code));
  writeHeader(slot, h);
  return true;
}

bool migrateV2(uint8_t slot, const HeaderV2& old, HeaderV4& h) {
  if (old.magic != MAGIC_V2 || old.version != 2 || !old.symbolCount) return false;
  h = HeaderV4{};
  h.magic = MAGIC_V4;
  h.version = VERSION_V4;
  h.enabled = old.enabled;
  h.matchCode = 1;
  h.radioId = 0;
  h.symbolCount = old.symbolCount;
  h.pulseLengthUs = old.pulseLengthUs;
  h.frequencyHz = 0;
  copyText(h.name, sizeof(h.name), String(old.name));
  copyText(h.protocol, sizeof(h.protocol),
           protocolName(static_cast<OpenRfProtocol>(old.protocol)));
  const String code = hexCode(old.code);
  copyText(h.deviceId, sizeof(h.deviceId), code);
  copyText(h.code, sizeof(h.code), code);
  writeHeader(slot, h);
  return true;
}

bool readHeader(uint8_t slot, HeaderV4& h) {
  if (!valid(slot)) return false;
  File f = LittleFS.open(path(slot), "r");
  if (!f) return false;

  uint32_t magic = 0;
  f.read(reinterpret_cast<uint8_t*>(&magic), sizeof(magic));
  f.seek(0);

  bool ok = false;

  if (magic == MAGIC_V4 && f.size() == sizeof(HeaderV4)) {
    ok = f.read(reinterpret_cast<uint8_t*>(&h), sizeof(h)) == sizeof(h) &&
         h.version == VERSION_V4 && h.protocol[0];
  } else if (magic == MAGIC_V3 && f.size() == sizeof(HeaderV3)) {
    HeaderV3 old{};
    ok = f.read(reinterpret_cast<uint8_t*>(&old), sizeof(old)) == sizeof(old);
    f.close();
    return ok && migrateV3(slot, old, h);
  } else if (magic == MAGIC_V2 && f.size() == sizeof(HeaderV2)) {
    HeaderV2 old{};
    ok = f.read(reinterpret_cast<uint8_t*>(&old), sizeof(old)) == sizeof(old);
    f.close();
    return ok && migrateV2(slot, old, h);
  }

  f.close();
  return ok;
}

bool duplicateExistsV2(uint8_t exceptSlot, ProtocolId protocol, uint64_t code) {
  const String codeText = hexCode(code);
  for (uint8_t i = 1; i <= OPENRF_RX_SLOT_COUNT; ++i) {
    if (i == exceptSlot) continue;
    HeaderV4 h{};
    if (!readHeader(i, h)) continue;
    if (!storedProtocolMatchesV2(h.protocol, protocol)) continue;
    // NVKP01 currently exposes one normalized physical control. Any existing
    // NVKP01 slot (including a legacy-learned nvkp01/button/PRESS slot) is the
    // same identity as V2 normalized code 1.
    if (protocol == ProtocolId::NVKP01_KINETIC) return true;
    if (h.code[0] && String(h.code).equalsIgnoreCase(codeText)) return true;
    if (h.deviceId[0] && String(h.deviceId).equalsIgnoreCase(codeText)) return true;
  }
  return false;
}

bool saveV2Learn(uint8_t slot, const String& name,
                 const V2LearnPayload& learned,
                 float frequencyMHz, uint8_t radioId) {
  const ProtocolId protocol = static_cast<ProtocolId>(learned.protocolId);
  const char* const protocolName = storedProtocolNameForV2(protocol);
  if (!valid(slot) || !learned.available || protocolName == nullptr ||
      learned.symbolCount == 0 || duplicateExistsV2(slot, protocol, learned.code)) {
    return false;
  }

  const String codeText = hexCode(learned.code);
  HeaderV4 h{};
  h.magic = MAGIC_V4;
  h.version = VERSION_V4;
  h.enabled = 1;
  h.matchCode = 1;
  h.radioId = radioId;
  h.symbolCount = learned.symbolCount;
  h.pulseLengthUs = learned.pulseLengthUs;
  h.frequencyHz = static_cast<uint32_t>(frequencyMHz * 1000000.0F + 0.5F);
  copyText(h.name, sizeof(h.name),
           name.length() ? name : ("RX Slot " + String(slot)));
  copyText(h.protocol, sizeof(h.protocol), String(protocolName));
  copyText(h.deviceId, sizeof(h.deviceId), codeText);
  copyText(h.command, sizeof(h.command), String());
  copyText(h.code, sizeof(h.code), codeText);
  return writeHeader(slot, h);
}
}

void __attribute__((weak)) mqttPublishRxSlotEvent(uint8_t, const RxSlotInfo&) {}

void rxSlotsBegin() {
  Serial.print(F("Universal RX slots loaded: ")); Serial.println(rxSlotCountUsed());
}

void rxSlotsLoop() {
  // RF processing itself remains event-driven. The loop only owns the
  // RX-slot Learn watchdog so a Capture session cannot remain armed forever
  // when no usable frame arrives.
  if (!learningSlot || learnState != "waiting_for_signal" ||
      learningStartedAtMs == 0U) {
    return;
  }

  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - learningStartedAtMs) < RX_LEARN_TIMEOUT_MS) {
    return;
  }

  const uint8_t timedOutSlot = learningSlot;
  const bool discarded = rfCommandDiscardLearn();
  learningSlot = 0;
  learningStartedAtMs = 0;
  learningName = "";
  learnSource = "";
  learnState = discarded ? "timeout" : "radio_error";

  Serial.print(F("RX learn timeout: slot "));
  Serial.print(timedOutSlot);
  Serial.print(F(", discard="));
  Serial.println(discarded ? F("OK") : F("FAILED"));
}

void rxSlotsHandleRFEvent(const RFEventMessage& event) {
  // Step 39.2: RX_FRAME protocol actions are V2-only and are handled by
  // rxSlotsHandleV2Action(). This handler now owns only RX Slot Learn preview
  // bookkeeping; it never invokes the retired legacy protocol decoder.
  if (event.type == RFEventType::RX_FRAME) return;

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
        learningStartedAtMs = 0;
        learningName = "";
      }
      return;
    }

    const uint8_t completedSlot = learningSlot;
    // Step 23: persist the authoritative Operating frequency from Step 22.
    float learnedFrequencyMHz = event.frequencyMhz;
    if (event.radioId == 1 || event.radioId == 2) {
      const float operatingMHz = Radio.getOperatingFrequency(event.radioId);
      if (operatingMHz > 0.0F) learnedFrequencyMHz = operatingMHz;
    }

    bool ok = false;
    if (event.v2Learn.available) {
      const ProtocolId protocol =
          static_cast<ProtocolId>(event.v2Learn.protocolId);
      if (duplicateExistsV2(learningSlot, protocol, event.v2Learn.code)) {
        learnState = "duplicate_code";
        learnSource = "v2";
        learningSlot = 0;
        learningStartedAtMs = 0;
        learningName = "";
        rfCommandDiscardLearn();
        return;
      }

      ok = saveV2Learn(completedSlot, learningName, event.v2Learn,
                       learnedFrequencyMHz, event.radioId);
      learnSource = "v2";
      learnState = ok ? "saved" : "save_error";

      Serial.print(F("RX V2-native learn slot "));
      Serial.print(completedSlot);
      Serial.print(ok ? F(" saved: ") : F(" failed: "));
      Serial.print(storedProtocolNameForV2(protocol));
      Serial.print(F(" code="));
      Serial.print(hexCode(event.v2Learn.code));
    } else {
      // Step 39.2: no protocol fallback remains. Unknown/ambiguous captures
      // cannot create protocol RX Slots; Step 40 will add explicit Learned RAW
      // storage/matching as the supported non-protocol path.
      learnState = "unsupported_protocol";
      learnSource = "v2";
      learningSlot = 0;
      learningStartedAtMs = 0;
      learningName = "";
      rfCommandDiscardLearn();
      Serial.println(F("RX V2 learn ignored: UNKNOWN/AMBIGUOUS protocol"));
      return;
    }

    if (ok) {
      Serial.print(F(" radio=R")); Serial.print(event.radioId);
      Serial.print(F(" frequency=")); Serial.print(learnedFrequencyMHz, 4);
      Serial.print(F(" MHz"));
    }
    Serial.println();

    learningSlot = 0;
    learningStartedAtMs = 0;
    learningName = "";
    rfCommandDiscardLearn();
    if (ok) mqttPublishDiscovery();
    return;
  }

  return;
}

void rxSlotsHandleV2Action(const RFEventMessage& event) {
  if (event.type != RFEventType::RX_FRAME || !event.v2Action.available) return;

  const ProtocolId protocol =
      static_cast<ProtocolId>(event.v2Action.protocolId);
  if (protocol != ProtocolId::EV1527_PRINCETON &&
      protocol != ProtocolId::PT2262_TRI_STATE &&
      protocol != ProtocolId::NVKP01_KINETIC &&
      protocol != ProtocolId::HT12E) {
    return;
  }

  const String code = hexCode(event.v2Action.code);

  for (uint8_t slot = 1; slot <= OPENRF_RX_SLOT_COUNT; slot++) {
    HeaderV4 h{};
    if (!readHeader(slot, h) || !h.enabled) continue;

    if (h.radioId && event.radioId && h.radioId != event.radioId) continue;
    if (h.frequencyHz && event.frequencyMhz > 0.0F) {
      const float learnedMHz = h.frequencyHz / 1000000.0F;
      if (fabsf(learnedMHz - event.frequencyMhz) > 0.500F) continue;
    }

    if (!storedProtocolMatchesV2(h.protocol, protocol)) continue;

    if (protocol == ProtocolId::NVKP01_KINETIC) {
      // Compatibility with slots learned before Step 39.1. The legacy Kinetic
      // path stored nvkp01/button/PRESS, while V2-native Learn stores the
      // protocol-local normalized code 1. Both represent the same one-button
      // NVKP01 control and must continue to match without re-learning.
      const bool legacyIdentity =
          String(h.deviceId).equalsIgnoreCase("nvkp01") &&
          String(h.command).equalsIgnoreCase("button") &&
          (!h.matchCode || !h.code[0] ||
           String(h.code).equalsIgnoreCase("PRESS"));
      const bool v2Identity =
          (!h.deviceId[0] || String(h.deviceId).equalsIgnoreCase(code)) &&
          !h.command[0] &&
          (!h.matchCode || !h.code[0] ||
           String(h.code).equalsIgnoreCase(code));
      if (!legacyIdentity && !v2Identity) continue;
    } else {
      // Preserve the existing RX Slot identity semantics. Legacy EV1527/PT2262
      // slots normally store the numeric code in both deviceId and code.
      if (h.deviceId[0] &&
          !String(h.deviceId).equalsIgnoreCase(code)) {
        continue;
      }
      // V2 fixed-code modules currently expose no separate command field.
      if (h.command[0]) continue;
      if (h.matchCode && h.code[0] &&
          !String(h.code).equalsIgnoreCase(code)) {
        continue;
      }
    }

    lastMatchedAt[slot] = millis();
    matchCounts[slot]++;
    lastRssi[slot] = event.rssiDbm;
    lastQuality[slot] = 100;

    const RxSlotInfo info = rxSlotGetInfo(slot);
    Serial.print(F("RX V2 authoritative match: slot "));
    Serial.print(slot);
    Serial.print(F(", protocol="));
    Serial.print(h.protocol);
    Serial.print(F(", code="));
    Serial.println(code);
    mqttPublishRxSlotEvent(slot, info);
    return;
  }
}

bool rxSlotStartLearn(uint8_t slot, const String& name) {
  if (!valid(slot) || learningSlot || !rfCommandStartLearn()) return false;
  learningSlot = slot;
  learningStartedAtMs = millis();
  learningName = name.length() ? name : ("RX Slot " + String(slot));
  learnState = "waiting_for_signal";
  learnSource = "";
  weakLearnRejected = 0;
  lastWeakLearnRssi = -127.0F;
  return true;
}
bool rxSlotDelete(uint8_t slot) { return valid(slot) && (!LittleFS.exists(path(slot)) || LittleFS.remove(path(slot))); }
bool rxSlotRename(uint8_t slot, const String& name) {
  HeaderV4 h{}; if (!readHeader(slot, h)) return false;
  copyText(h.name, sizeof(h.name), name.length() ? name : ("RX Slot " + String(slot)));
  return writeHeader(slot, h);
}
bool rxSlotSetEnabled(uint8_t slot, bool enabled) {
  HeaderV4 h{}; if (!readHeader(slot, h)) return false; h.enabled = enabled ? 1 : 0; return writeHeader(slot, h);
}
bool rxSlotSend(uint8_t slot, uint8_t repeats) {
  HeaderV4 h{};
  if (!readHeader(slot, h) || !h.radioId || !h.frequencyHz || !h.symbolCount) {
    protocolTxDiagnosticsRecord(ProtocolId::UNKNOWN, 0, 0, 0.0F, false, 0,
                                ProtocolTxFailureReason::INVALID_SLOT, millis());
    return false;
  }

  uint64_t numericCode = 0;
  const ProtocolId v2Protocol = v2ProtocolFromStoredName(String(h.protocol));
  if (!parseHex64(String(h.code), numericCode)) {
    protocolTxDiagnosticsRecord(v2Protocol, 0, h.radioId,
                                h.frequencyHz / 1000000.0F, false, 0,
                                ProtocolTxFailureReason::INVALID_CODE, millis());
    return false;
  }

  // Step 38: protocol TX is V2-native only. EV1527/Princeton,
  // PT2262/Tri-State and HT12E are provided by the modular Known Protocol
  // Library. RX-only/unknown protocols are deliberately not routed through
  // the retired generic legacy protocol encoder.
  const ProtocolTxEncoder* const tx = knownProtocolLibraryTxEncoder(v2Protocol);
  if (tx == nullptr) {
    protocolTxDiagnosticsRecord(v2Protocol, numericCode, h.radioId,
                                h.frequencyHz / 1000000.0F, false, 0,
                                ProtocolTxFailureReason::UNSUPPORTED_PROTOCOL,
                                millis());
    return false;
  }

  ProtocolTxRequest request;
  request.protocol = v2Protocol;
  request.code = numericCode;
  request.symbolCount = h.symbolCount;
  request.pulseLengthUs = h.pulseLengthUs;
  request.requestedRepeats = repeats;

  ProtocolTxPlan plan;
  if (!tx->encode(request, openrfScratch, OPENRF_MAX_RAW_PULSES, plan) ||
      plan.pulseCount == 0U || plan.transmitRepeats == 0U) {
    protocolTxDiagnosticsRecord(v2Protocol, numericCode, h.radioId,
                                h.frequencyHz / 1000000.0F, false, 0,
                                ProtocolTxFailureReason::ENCODE_FAILED, millis());
    return false;
  }

  Serial.print(F("RX Slot V2 TX ")); Serial.print(slot);
  Serial.print(F(": ")); Serial.print(h.protocol);
  Serial.print(F(", R")); Serial.print(h.radioId);
  Serial.print(F(", ")); Serial.print(h.frequencyHz / 1000000.0F, 4);
  Serial.print(F(" MHz, pulses=")); Serial.print(plan.pulseCount);
  Serial.print(F(", protocol repeats=")); Serial.println(plan.protocolRepeats);

  const bool ok = rfCommandSendRawTuned(
      openrfScratch, plan.pulseCount, plan.transmitRepeats, h.radioId,
      h.frequencyHz / 1000000.0F);
  protocolTxDiagnosticsRecord(
      v2Protocol, numericCode, h.radioId, h.frequencyHz / 1000000.0F, ok,
      plan.protocolRepeats,
      ok ? ProtocolTxFailureReason::NONE : ProtocolTxFailureReason::RF_TX_FAILED,
      millis());
  return ok;
}

RxSlotInfo rxSlotGetInfo(uint8_t slot) {
  RxSlotInfo info{}; info.id = slot; HeaderV4 h{};
  if (!readHeader(slot, h)) { info.name = "RX Slot " + String(slot); return info; }
  info.used = true; info.enabled = h.enabled; info.name = h.name;
  info.protocol = h.protocol; info.deviceId = h.deviceId; info.command = h.command; info.code = h.code;
  info.matchCode = h.matchCode; info.symbolCount = h.symbolCount;
  info.pulseLengthUs = h.pulseLengthUs;
  info.frequencyMHz = h.frequencyHz / 1000000.0F;
  info.radioId = h.radioId;
  const ProtocolId v2Protocol = v2ProtocolFromStoredName(String(h.protocol));
  info.sendSupported = h.radioId && h.frequencyHz && h.symbolCount &&
                       knownProtocolLibrarySupportsTx(v2Protocol);
  info.matchCount = matchCounts[slot];
  info.lastRssi = lastRssi[slot]; info.lastQuality = lastQuality[slot]; return info;
}
uint8_t rxSlotCountUsed() { uint8_t n=0; for(uint8_t i=1;i<=OPENRF_RX_SLOT_COUNT;i++){HeaderV4 h{};if(readHeader(i,h))n++;}return n; }
const char* rxSlotLearnState() { return learnState.c_str(); }
const char* rxSlotLearnSource() { return learnSource.c_str(); }
uint8_t rxSlotLearningId() { return learningSlot; }

uint32_t rxSlotWeakLearnRejectedCount() { return weakLearnRejected; }
float rxSlotLastWeakLearnRssi() { return lastWeakLearnRssi; }
