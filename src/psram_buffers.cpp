#include <Arduino.h>
#include <esp_heap_caps.h>
#include "psram_buffers.h"

namespace {
int16_t* systemScratch = nullptr;
int16_t* radioLastRaw = nullptr;
int16_t* radioLearnRaw = nullptr;
bool external = false;
size_t allocatedBytes = 0;

int16_t* allocPulseBuffer(bool preferPsram) {
  const size_t bytes = OPENRF_MAX_RAW_PULSES * sizeof(int16_t);
  void* ptr = nullptr;
  if (preferPsram && ESP.getPsramSize()) {
    ptr = heap_caps_calloc(OPENRF_MAX_RAW_PULSES, sizeof(int16_t),
                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
  if (!ptr) {
    ptr = heap_caps_calloc(OPENRF_MAX_RAW_PULSES, sizeof(int16_t),
                           MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  if (ptr) allocatedBytes += bytes;
  return static_cast<int16_t*>(ptr);
}
}

bool psramBuffersBegin() {
  if (systemScratch && radioLastRaw && radioLearnRaw) return true;

  const bool preferPsram = ESP.getPsramSize() > 0;
  systemScratch = allocPulseBuffer(preferPsram);
  radioLastRaw = allocPulseBuffer(preferPsram);
  radioLearnRaw = allocPulseBuffer(preferPsram);

  if (!systemScratch || !radioLastRaw || !radioLearnRaw) {
    Serial.println(F("FATAL: OpenRF working buffer allocation failed"));
    return false;
  }

  external =
      esp_ptr_external_ram(systemScratch) &&
      esp_ptr_external_ram(radioLastRaw) &&
      esp_ptr_external_ram(radioLearnRaw);

  Serial.print(F("OpenRF large buffers: "));
  Serial.print(allocatedBytes);
  Serial.print(F(" bytes, "));
  Serial.println(external ? F("PSRAM") : F("internal RAM fallback"));
  return true;
}

bool psramBuffersUsingExternalRam() { return external; }
int16_t* psramSystemScratch() { return systemScratch; }
int16_t* psramRadioLastRaw() { return radioLastRaw; }
int16_t* psramRadioLearnRaw() { return radioLearnRaw; }
size_t psramOpenRFAllocatedBytes() { return allocatedBytes; }
