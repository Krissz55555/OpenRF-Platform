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
  START_RECEIVE,
  STOP_RECEIVE
};

enum class RFEventType : uint8_t {
  NONE = 0,
  RX_FRAME,
  LEARN_PREVIEW,
  TX_COMPLETE,
  RADIO_ERROR
};

struct RFCommandMessage {
  RFCommandType type = RFCommandType::NONE;
  uint8_t repeats = 0;
  uint16_t pulseCount = 0;
  int16_t pulses[OPENRF_MAX_RAW_PULSES] = {0};
  TaskHandle_t replyTask = nullptr;
  bool* result = nullptr;
};

// POD-only snapshot so FreeRTOS Queue can safely copy it by value.
// No String or pointer is stored in the queue.
struct RFEventMessage {
  RFEventType type = RFEventType::NONE;
  uint32_t sequence = 0;
  uint16_t pulseCount = 0;
  uint32_t durationUs = 0;
  float rssiDbm = -127.0F;
  float frequencyMhz = 0.0F;
  uint32_t timestampMs = 0;
  bool success = false;
  int16_t errorCode = 0;
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
bool rfCommandSendRaw(const int16_t* pulses, uint16_t pulseCount, uint8_t repeats);
bool rfCommandStartReceive();
bool rfCommandStopReceive();

// Core 1 -> Core 0 event API. These are non-blocking by design: RF capture
// must never stall because the System side is temporarily busy.
bool rfEventPublishFrame(RFEventType type, uint32_t sequence,
                         const int16_t* pulses, uint16_t pulseCount,
                         uint32_t durationUs, float rssiDbm,
                         float frequencyMhz, uint32_t timestampMs);
bool rfEventPublishStatus(RFEventType type, bool success, int16_t errorCode = 0);
uint32_t rfEventDroppedCount();
uint32_t rfEventProcessedCount();
uint32_t rfEventQueueDepth();

void dualCoreMetricsLoop();
uint8_t dualCoreLoad(uint8_t core);
