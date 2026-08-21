#include <Arduino.h>
#include <LittleFS.h>

#include "version.h"
#include "config.h"
#include "openrf_wifi.h"
#include "web.h"
#include "radio.h"
#include "mqtt.h"
#include "rxslots.h"
#include "dualcore.h"
#include "psram_buffers.h"
#include "analyzer.h"
#include "scratch.h"

namespace {
bool delayedMemoryReportPrinted = false;
uint32_t bootStartedAtMs = 0;

void printMemoryDiagnostics() {
  Serial.println(F("=== OpenRF ESP32-S3 Memory ==="));
  Serial.print(F("Flash total: ")); Serial.print(ESP.getFlashChipSize() / (1024.0 * 1024.0), 2); Serial.println(F(" MB"));
  Serial.print(F("PSRAM total: ")); Serial.print(ESP.getPsramSize() / (1024.0 * 1024.0), 2); Serial.println(F(" MB"));
  Serial.print(F("PSRAM free:  ")); Serial.print(ESP.getFreePsram() / (1024.0 * 1024.0), 2); Serial.println(F(" MB"));
  Serial.print(F("Heap total:  ")); Serial.print(ESP.getHeapSize() / 1024.0, 1); Serial.println(F(" KB"));
  Serial.print(F("Heap free:   ")); Serial.print(ESP.getFreeHeap() / 1024.0, 1); Serial.println(F(" KB"));
  Serial.print(F("OpenRF PSRAM buffers: ")); Serial.print(psramOpenRFAllocatedBytes() / 1024.0, 1);
  Serial.print(F(" KB (")); Serial.print(psramBuffersUsingExternalRam() ? F("external") : F("internal fallback")); Serial.println(F(")"));
  Serial.print(F("Analyzer PSRAM stores: ")); Serial.print(analyzerPsramAllocatedBytes() / 1024.0, 1);
  Serial.print(F(" KB (")); Serial.print(analyzerUsingExternalRam() ? F("external") : F("internal fallback")); Serial.println(F(")"));
  Serial.println(ESP.getPsramSize() ? F("PSRAM detected and available") : F("WARNING: PSRAM not detected"));
  Serial.println(F("==============================="));
}
}

void setup() {
  Serial.begin(115200);
  bootStartedAtMs = millis();
  delay(1000);

  Serial.println();
  Serial.println(FW_NAME);
  Serial.println(FW_VERSION);

  Serial.println(F("ESP32-S3 memory configuration:"));
  Serial.print(F("  Flash: "));
  Serial.print(ESP.getFlashChipSize() / (1024UL * 1024UL));
  Serial.println(F(" MB"));
  Serial.print(F("  PSRAM: "));
  Serial.print(ESP.getPsramSize() / (1024UL * 1024UL));
  Serial.println(F(" MB"));
  Serial.print(F("  Free PSRAM: "));
  Serial.print(ESP.getFreePsram() / 1024UL);
  Serial.println(F(" KB"));
  if (ESP.getPsramSize() == 0) {
    Serial.println(F("  WARNING: PSRAM not detected"));
  } else {
    Serial.println(F("  PSRAM detected and available"));
  }

  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS mount failed"));
  } else {
    Serial.println(F("LittleFS mounted"));
  }

  // Keep startup order identical to the stable ESP32-S3 port. Only the
  // recurring loop work is split between the two pinned FreeRTOS tasks.
  if (!psramBuffersBegin()) {
    Serial.println(F("FATAL: PSRAM/OpenRF buffer initialization failed"));
    return;
  }
  openrfScratch = psramSystemScratch();
  if (!analyzerBegin()) {
    Serial.println(F("FATAL: Analyzer PSRAM initialization failed"));
    return;
  }

  configBegin();
  wifiBegin();
  Radio.begin();
  mqttBegin();
  rxSlotsBegin();
  webBegin();

  Serial.print(F("Free heap after startup: "));
  Serial.println(ESP.getFreeHeap());

  if (!dualCoreBegin()) {
    Serial.println(F("FATAL: dual-core runtime failed to start"));
  }
}

void loop() {
  if (!delayedMemoryReportPrinted && millis() - bootStartedAtMs >= 5000UL) {
    delayedMemoryReportPrinted = true;
    printMemoryDiagnostics();
  }

  // OpenRF recurring work runs in pinned FreeRTOS tasks after setup(). Keep the
  // Arduino loop task dormant so it does not compete with RadioTask on Core 1.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
