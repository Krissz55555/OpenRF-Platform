#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_freertos_hooks.h>

#include "dualcore.h"
#include "openrf_wifi.h"
#include "radio.h"
#include "mqtt.h"
#include "rxslots.h"
#include "web.h"
#include "platform_compat.h"

namespace {
constexpr uint32_t SYSTEM_TASK_STACK = 16384;
constexpr uint32_t RADIO_TASK_STACK = 16384;
constexpr UBaseType_t SYSTEM_TASK_PRIORITY = 1;
constexpr UBaseType_t RADIO_TASK_PRIORITY = 2;
constexpr uint8_t RF_COMMAND_QUEUE_LENGTH = 4;
constexpr uint8_t RF_EVENT_QUEUE_LENGTH = 8;

TaskHandle_t systemTaskHandle = nullptr;
TaskHandle_t radioTaskHandle = nullptr;

volatile uint32_t idleCounter[2] = {0, 0};
uint32_t lastIdleCounter[2] = {0, 0};
uint32_t idleReference[2] = {0, 0};
volatile uint8_t coreLoadPercent[2] = {0, 0};
uint32_t lastMetricsMs = 0;

volatile uint32_t eventDropped = 0;
volatile uint32_t eventProcessed = 0;

bool idleHookCore0() {
  idleCounter[0]++;
  return true;
}

bool idleHookCore1() {
  idleCounter[1]++;
  return true;
}


void systemTask(void* parameter) {
  (void)parameter;
  Serial.print(F("SystemTask started on Core "));
  Serial.println(xPortGetCoreID());

  uint32_t lastHealthLogMs = 0;
  uint32_t minimumObservedHeap = UINT32_MAX;

  for (;;) {
    wifiLoop();

    // Drain Radio -> System snapshots first. One event is fanned out to every
    // interested System-side consumer, so MQTT and RX Slots see the same frame.
    RFEventMessage event;
    uint8_t eventsThisPass = 0;
    while (eventsThisPass < 8 &&
           xQueueReceive(rfEventQueue, &event, 0) == pdTRUE) {
      rxSlotsHandleRFEvent(event);
      mqttHandleRFEvent(event);
      eventProcessed++;
      eventsThisPass++;
    }

    rxSlotsLoop();
    mqttLoop();
    webLoop();
    dualCoreMetricsLoop();

    const uint32_t now = millis();
    const uint32_t currentHeap = ESP.getFreeHeap();
    if (currentHeap < minimumObservedHeap) minimumObservedHeap = currentHeap;

    if (now - lastHealthLogMs >= 60000UL) {
      lastHealthLogMs = now;
      Serial.print(F("Health: heap="));
      Serial.print(currentHeap);
      Serial.print(F(", min_heap="));
      Serial.print(minimumObservedHeap);
      Serial.print(F(", max_block="));
      Serial.print(openrfMaxFreeBlock());
      Serial.print(F(", fragmentation="));
      Serial.print(openrfHeapFragmentation());
      Serial.print(F("%, system_core="));
      Serial.print(xPortGetCoreID());
      Serial.print(F(", rfq_depth="));
      Serial.print(rfEventQueueDepth());
      Serial.print(F(", rfq_processed="));
      Serial.print(rfEventProcessedCount());
      Serial.print(F(", rfq_dropped="));
      Serial.print(rfEventDroppedCount());
      Serial.println();
    }

    // Preserve the responsive Arduino-loop behavior while giving the idle task
    // and the ESP-IDF networking stack regular scheduling opportunities.
    vTaskDelay(1);
  }
}

void radioTask(void* parameter) {
  (void)parameter;
  Serial.print(F("RadioTask started on Core "));
  Serial.println(xPortGetCoreID());

  for (;;) {
    RFCommandMessage command;
    while (xQueueReceive(rfCommandQueue, &command, 0) == pdTRUE) {
      bool ok = false;
      switch (command.type) {
        case RFCommandType::START_LEARN:    ok = Radio.startLearning(); break;
        case RFCommandType::ACCEPT_LEARN:   ok = Radio.acceptLearnCapture(); break;
        case RFCommandType::DISCARD_LEARN:  ok = Radio.discardLearnCapture(); break;
        case RFCommandType::TEST_LEARN_TX:  ok = Radio.testSendLearnCapture(command.repeats); break;
        case RFCommandType::SEND_RAW:
          ok = Radio.sendRaw(command.pulses, command.pulseCount, command.repeats);
          break;
        case RFCommandType::START_RECEIVE:  ok = Radio.startReceive(); break;
        case RFCommandType::STOP_RECEIVE:   ok = Radio.stopReceive(); break;
        default: break;
      }
      if (command.type == RFCommandType::SEND_RAW ||
          command.type == RFCommandType::TEST_LEARN_TX) {
        rfEventPublishStatus(RFEventType::TX_COMPLETE, ok,
                             ok ? 0 : Radio.getLastError());
      } else if (!ok &&
                 (command.type == RFCommandType::START_RECEIVE ||
                  command.type == RFCommandType::STOP_RECEIVE)) {
        rfEventPublishStatus(RFEventType::RADIO_ERROR, false,
                             Radio.getLastError());
      }

      if (command.result) *command.result = ok;
      if (command.replyTask) xTaskNotifyGive(command.replyTask);
    }

    Radio.loop();
    vTaskDelay(1);
  }
}
}  // namespace

QueueHandle_t rfCommandQueue = nullptr;
QueueHandle_t rfEventQueue = nullptr;

bool dualCoreBegin() {
  // Per-core idle hooks let us estimate total core utilization without
  // instrumenting RF/Web/MQTT code paths individually.
  esp_register_freertos_idle_hook_for_cpu(idleHookCore0, 0);
  esp_register_freertos_idle_hook_for_cpu(idleHookCore1, 1);
  rfCommandQueue = xQueueCreate(RF_COMMAND_QUEUE_LENGTH, sizeof(RFCommandMessage));
  rfEventQueue = xQueueCreate(RF_EVENT_QUEUE_LENGTH, sizeof(RFEventMessage));

  if (!rfCommandQueue || !rfEventQueue) {
    Serial.println(F("Dual-core queue creation failed"));
    return false;
  }

  BaseType_t result = xTaskCreatePinnedToCore(
      systemTask, "OpenRF-System", SYSTEM_TASK_STACK, nullptr,
      SYSTEM_TASK_PRIORITY, &systemTaskHandle, 0);
  if (result != pdPASS) {
    Serial.println(F("SystemTask creation failed"));
    return false;
  }

  result = xTaskCreatePinnedToCore(
      radioTask, "OpenRF-Radio", RADIO_TASK_STACK, nullptr,
      RADIO_TASK_PRIORITY, &radioTaskHandle, 1);
  if (result != pdPASS) {
    Serial.println(F("RadioTask creation failed"));
    vTaskDelete(systemTaskHandle);
    systemTaskHandle = nullptr;
    return false;
  }

  Serial.println(F("Dual-core Step 3 active: System=Core0, Radio=Core1"));
  Serial.println(F("Bidirectional queues active: rfCommandQueue + rfEventQueue"));
  return true;
}


void dualCoreMetricsLoop() {
  const uint32_t now = millis();
  if (now - lastMetricsMs < 1000UL) return;
  lastMetricsMs = now;

  for (uint8_t core = 0; core < 2; ++core) {
    const uint32_t current = idleCounter[core];
    const uint32_t idleDelta = current - lastIdleCounter[core];
    lastIdleCounter[core] = current;

    // Learn the largest observed one-second idle count as the 0% load
    // reference. This keeps the monitor independent of CPU frequency.
    if (idleDelta > idleReference[core]) idleReference[core] = idleDelta;

    if (idleReference[core] == 0) {
      coreLoadPercent[core] = 0;
      continue;
    }

    uint32_t idlePct = (idleDelta * 100UL) / idleReference[core];
    if (idlePct > 100UL) idlePct = 100UL;
    coreLoadPercent[core] = static_cast<uint8_t>(100UL - idlePct);
  }
}

uint8_t dualCoreLoad(uint8_t core) {
  return core < 2 ? coreLoadPercent[core] : 0;
}


namespace {
bool executeRFCommand(RFCommandMessage& command, TickType_t timeout = pdMS_TO_TICKS(5000)) {
  if (!rfCommandQueue) return false;

  bool result = false;
  command.replyTask = xTaskGetCurrentTaskHandle();
  command.result = &result;

  // Clear a stale notification before submitting a new synchronous command.
  ulTaskNotifyTake(pdTRUE, 0);

  if (xQueueSend(rfCommandQueue, &command, timeout) != pdTRUE) return false;
  if (ulTaskNotifyTake(pdTRUE, timeout) == 0) return false;
  return result;
}
}

bool rfCommandStartLearn() {
  RFCommandMessage c; c.type = RFCommandType::START_LEARN;
  return executeRFCommand(c);
}
bool rfCommandAcceptLearn() {
  RFCommandMessage c; c.type = RFCommandType::ACCEPT_LEARN;
  return executeRFCommand(c);
}
bool rfCommandDiscardLearn() {
  RFCommandMessage c; c.type = RFCommandType::DISCARD_LEARN;
  return executeRFCommand(c);
}
bool rfCommandTestLearnTx(uint8_t repeats) {
  RFCommandMessage c; c.type = RFCommandType::TEST_LEARN_TX; c.repeats = repeats;
  return executeRFCommand(c);
}
bool rfCommandSendRaw(const int16_t* pulses, uint16_t pulseCount, uint8_t repeats) {
  if (!pulses || pulseCount == 0 || pulseCount > OPENRF_MAX_RAW_PULSES) return false;
  RFCommandMessage c; c.type = RFCommandType::SEND_RAW; c.repeats = repeats; c.pulseCount = pulseCount;
  memcpy(c.pulses, pulses, pulseCount * sizeof(int16_t));
  return executeRFCommand(c);
}
bool rfCommandStartReceive() {
  RFCommandMessage c; c.type = RFCommandType::START_RECEIVE;
  return executeRFCommand(c);
}
bool rfCommandStopReceive() {
  RFCommandMessage c; c.type = RFCommandType::STOP_RECEIVE;
  return executeRFCommand(c);
}


bool rfEventPublishFrame(RFEventType type, uint32_t sequence,
                         const int16_t* pulses, uint16_t pulseCount,
                         uint32_t durationUs, float rssiDbm,
                         float frequencyMhz, uint32_t timestampMs) {
  if (!rfEventQueue || !pulses || pulseCount == 0 ||
      pulseCount > OPENRF_MAX_RAW_PULSES) {
    return false;
  }

  RFEventMessage event;
  event.type = type;
  event.sequence = sequence;
  event.pulseCount = pulseCount;
  event.durationUs = durationUs;
  event.rssiDbm = rssiDbm;
  event.frequencyMhz = frequencyMhz;
  event.timestampMs = timestampMs;
  memcpy(event.pulses, pulses, pulseCount * sizeof(int16_t));

  if (xQueueSend(rfEventQueue, &event, 0) != pdTRUE) {
    eventDropped++;
    return false;
  }
  return true;
}

bool rfEventPublishStatus(RFEventType type, bool success, int16_t errorCode) {
  if (!rfEventQueue) return false;
  RFEventMessage event;
  event.type = type;
  event.success = success;
  event.errorCode = errorCode;
  event.timestampMs = millis();

  if (xQueueSend(rfEventQueue, &event, 0) != pdTRUE) {
    eventDropped++;
    return false;
  }
  return true;
}

uint32_t rfEventDroppedCount() {
  return eventDropped;
}

uint32_t rfEventProcessedCount() {
  return eventProcessed;
}

uint32_t rfEventQueueDepth() {
  return rfEventQueue ? static_cast<uint32_t>(uxQueueMessagesWaiting(rfEventQueue)) : 0;
}
