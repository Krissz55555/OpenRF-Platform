#pragma once
#include <Arduino.h>

struct RFEventMessage;

constexpr uint8_t OPENRF_RX_SLOT_COUNT = 10;

struct RxSlotInfo {
  uint8_t id = 0;
  bool used = false;
  bool enabled = true;
  String name;
  String protocol;
  String deviceId;
  String command;
  String code;
  bool matchCode = true;
  uint8_t symbolCount = 0;
  uint16_t pulseLengthUs = 0;
  uint32_t matchCount = 0;
  float lastRssi = -127.0F;
  uint8_t lastQuality = 0;
};

void rxSlotsBegin();
void rxSlotsLoop();
bool rxSlotStartLearn(uint8_t slot, const String& name);
bool rxSlotDelete(uint8_t slot);
bool rxSlotRename(uint8_t slot, const String& name);
bool rxSlotSetEnabled(uint8_t slot, bool enabled);
RxSlotInfo rxSlotGetInfo(uint8_t slot);
uint8_t rxSlotCountUsed();
const char* rxSlotLearnState();
uint8_t rxSlotLearningId();

void mqttPublishRxSlotEvent(uint8_t slot, const RxSlotInfo& info);
void mqttPublishDiscovery();

void rxSlotsHandleRFEvent(const RFEventMessage& event);

uint32_t rxSlotWeakLearnRejectedCount();
float rxSlotLastWeakLearnRssi();
