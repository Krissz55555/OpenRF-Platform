#pragma once

#include <Arduino.h>

struct RFEventMessage;
struct SlotInfo;

enum class RawSlotMatchState : uint8_t {
  IDLE = 0,
  NO_MATCH,
  MATCH,
  SUPPRESSED_DUPLICATE,
};

struct RawSlotMatchStats {
  bool available = false;
  uint32_t matchCount = 0;
  uint8_t lastSimilarity = 0;
  float lastRssi = -127.0F;
  uint32_t lastMatchedAtMs = 0;
};

struct RawSlotMatcherDiagnostics {
  bool available = false;
  RawSlotMatchState lastState = RawSlotMatchState::IDLE;
  uint8_t lastSlot = 0;
  uint8_t lastSimilarity = 0;
  uint8_t lastTimingSimilarity = 0;
  uint8_t lastCountSimilarity = 0;
  uint8_t lastSignAgreement = 0;
  uint16_t lastComparedPulses = 0;
  uint16_t learnedPatternPulses = 0;
  uint16_t incomingPatternPulses = 0;
  bool learnedRepeatReduced = false;
  bool incomingRepeatReduced = false;
  uint32_t matchCount = 0;
  uint32_t noMatchCount = 0;
  uint32_t duplicateSuppressedCount = 0;
};

bool rawSlotMatcherBegin();
bool rawSlotMatcherReload(uint8_t slot);
void rawSlotMatcherClear(uint8_t slot);
void rawSlotMatcherHandleRFEvent(const RFEventMessage& event);
RawSlotMatchStats rawSlotMatcherGetStats(uint8_t slot);
RawSlotMatcherDiagnostics rawSlotMatcherGetDiagnostics();
const char* rawSlotMatchStateName(RawSlotMatchState state);

// MQTT/HA hook. mqtt.cpp provides the strong implementation.
void mqttPublishRawSlotEvent(uint8_t slot, const SlotInfo& info,
                             const RawSlotMatchStats& stats);
