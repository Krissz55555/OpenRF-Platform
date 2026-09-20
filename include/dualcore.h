#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "radio.h"

// Step 3: bidirectional queue boundary.
// Core 0 -> Core 1: rfCommandQueue
// Core 1 -> Core 0: rfEventQueue

enum class RFCommandType : uint8_t {
  NONE = 0,
  START_LEARN,
  ACCEPT_LEARN,
  DISCARD_LEARN,
  TEST_LEARN_TX,
  SEND_RAW,
  SEND_RAW_TUNED,
  START_RECEIVE,
  STOP_RECEIVE
};

enum class RFEventType : uint8_t {
  NONE = 0,
  RX_FRAME,
  LEARN_PREVIEW,
  TX_COMPLETE,
  RADIO_ERROR,
  // A frame which passed the common RF validity checks but was intentionally
  // hidden from normal RX telemetry by the short-background filter.  Core 0
  // may offer it only to the Learned RAW matcher.
  RAW_MATCH_CANDIDATE
};

struct RFCommandMessage {
  RFCommandType type = RFCommandType::NONE;
  uint8_t repeats = 0;
  uint16_t pulseCount = 0;
  float frequencyMhz = 433.92F;
  uint8_t radioId = 0;
  int16_t pulses[OPENRF_MAX_RAW_PULSES] = {0};
  TaskHandle_t replyTask = nullptr;
  bool* result = nullptr;
};

// Compact Step 31 V2 actionable payload. This remains POD so the existing
// Core 1 -> Core 0 queue can carry the normalized event without a second queue
// or any heap allocation.
struct V2ActionPayload {
  bool available = false;
  uint16_t protocolId = 0;
  uint64_t code = 0;
  uint8_t symbolCount = 0;
  uint8_t repeats = 0;
};

// Step 33 V2-native RX Slot Learn payload. Unlike V2ActionPayload this is
// attached to LEARN_PREVIEW events and therefore does not imply an action.
// The timing field preserves enough protocol metadata for the existing V4
// RX-slot storage format without copying RAW data or invoking the legacy
// decoder on a V2 KNOWN capture.
struct V2LearnPayload {
  bool available = false;
  uint16_t protocolId = 0;
  uint64_t code = 0;
  uint8_t symbolCount = 0;
  uint8_t repeats = 0;
  uint16_t pulseLengthUs = 0;
};

// POD-only snapshot so FreeRTOS Queue can safely copy it by value.
// No String or pointer is stored in the queue.
struct RFEventMessage {
  RFEventType type = RFEventType::NONE;
  uint32_t sequence = 0;
  uint16_t pulseCount = 0;
  float frequencyMhz = 433.92F;
  uint8_t radioId = 0;
  uint32_t durationUs = 0;
  float rssiDbm = -127.0F;
  uint32_t timestampMs = 0;
  bool success = false;
  int16_t errorCode = 0;

  // Step 31 per-capture action routing. Raw telemetry can still be published
  // while legacy protocol actions are selectively suppressed for a capture.
  bool legacyProtocolActionAllowed = true;
  // Step 40: only captures that the V2 Known Protocol Engine classified as
  // UNKNOWN may enter Learned RAW matching. KNOWN (including dedup-suppressed)
  // and AMBIGUOUS captures never fall through to RAW slots.
  bool rawMatchEligible = false;
  V2ActionPayload v2Action;
  V2LearnPayload v2Learn;

  int16_t pulses[OPENRF_MAX_RAW_PULSES] = {0};
};

extern QueueHandle_t rfCommandQueue;
extern QueueHandle_t rfEventQueue;

bool dualCoreBegin();

// Core 0 -> Core 1 command API.
bool rfCommandStartLearn();
bool rfCommandAcceptLearn();
bool rfCommandDiscardLearn();
bool rfCommandTestLearnTx(uint8_t repeats);
bool rfCommandSendRaw(const int16_t* pulses, uint16_t pulseCount, uint8_t repeats,
                      float frequencyMhz);
bool rfCommandSendRawTuned(const int16_t* pulses, uint16_t pulseCount,
                           uint8_t repeats, uint8_t radioId,
                           float frequencyMhz);
bool rfCommandStartReceive();
bool rfCommandStopReceive();

// Core 1 -> Core 0 event API. These are non-blocking by design: RF capture
// must never stall because the System side is temporarily busy.
bool rfEventPublishFrame(RFEventType type, uint32_t sequence,
                         const int16_t* pulses, uint16_t pulseCount,
                         uint32_t durationUs, float rssiDbm,
                         float frequencyMhz, uint8_t radioId,
                         uint32_t timestampMs,
                         bool legacyProtocolActionAllowed = true,
                         const V2ActionPayload* v2Action = nullptr,
                         const V2LearnPayload* v2Learn = nullptr,
                         bool rawMatchEligible = false);
bool rfEventPublishStatus(RFEventType type, bool success, int16_t errorCode = 0);
uint32_t rfEventDroppedCount();
uint32_t rfEventProcessedCount();
uint32_t rfEventQueueDepth();

void dualCoreMetricsLoop();
uint8_t dualCoreLoad(uint8_t core);
