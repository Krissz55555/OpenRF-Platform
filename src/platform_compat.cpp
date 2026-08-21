#include <Arduino.h>
#include <esp_system.h>

#include "platform_compat.h"

String openrfChipIdHex() {
  const uint64_t mac = ESP.getEfuseMac();
  char buffer[13];
  snprintf(buffer, sizeof(buffer), "%04X%08X",
           static_cast<unsigned int>((mac >> 32) & 0xFFFFULL),
           static_cast<unsigned int>(mac & 0xFFFFFFFFULL));
  return String(buffer);
}

uint32_t openrfMaxFreeBlock() {
  return ESP.getMaxAllocHeap();
}

uint8_t openrfHeapFragmentation() {
  const uint32_t freeHeap = ESP.getFreeHeap();
  if (freeHeap == 0) return 0;
  const uint32_t maxBlock = openrfMaxFreeBlock();
  const uint32_t percent = 100UL - ((maxBlock * 100UL) / freeHeap);
  return static_cast<uint8_t>(percent > 100UL ? 100UL : percent);
}

String openrfResetReason() {
  switch (esp_reset_reason()) {
    case ESP_RST_UNKNOWN:   return "Unknown";
    case ESP_RST_POWERON:   return "Power on";
    case ESP_RST_EXT:       return "External reset";
    case ESP_RST_SW:        return "Software reset";
    case ESP_RST_PANIC:     return "Exception/panic";
    case ESP_RST_INT_WDT:   return "Interrupt watchdog";
    case ESP_RST_TASK_WDT:  return "Task watchdog";
    case ESP_RST_WDT:       return "Other watchdog";
    case ESP_RST_DEEPSLEEP: return "Deep sleep";
    case ESP_RST_BROWNOUT:  return "Brownout";
    case ESP_RST_SDIO:      return "SDIO reset";
    default:                return "Other reset";
  }
}
