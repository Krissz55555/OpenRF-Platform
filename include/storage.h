#pragma once

#include <Arduino.h>
#include "radio.h"

constexpr uint8_t OPENRF_SLOT_COUNT = 30;
constexpr uint8_t OPENRF_SLOT_NAME_MAX = 32;

struct SlotInfo {
  uint8_t id = 0;
  bool used = false;
  String name;
  float frequencyMHz = 433.92F;
  uint16_t pulseCount = 0;
  uint32_t durationUs = 0;
  uint32_t fingerprint = 0;
};

bool storageSlotExists(uint8_t slot);
SlotInfo storageGetSlotInfo(uint8_t slot);
bool storageSaveSlot(uint8_t slot, const String& name, float frequencyMHz,
                     const int16_t* pulses, uint16_t pulseCount,
                     uint32_t durationUs, uint32_t* fingerprintOut = nullptr);
bool storageLoadSlot(uint8_t slot, int16_t* pulses, uint16_t capacity,
                     SlotInfo& info);
bool storageRenameSlot(uint8_t slot, const String& name);
bool storageDeleteSlot(uint8_t slot);
uint32_t storageFingerprint(const int16_t* pulses, uint16_t pulseCount);

uint8_t storageCountUsedSlots();
