#pragma once

#include <Arduino.h>

// ESP32-S3 equivalents for a few ESP8266-specific diagnostics/identity APIs.
// These helpers keep the v1.2.0 external behavior and JSON fields unchanged.
String openrfChipIdHex();
uint32_t openrfMaxFreeBlock();
uint8_t openrfHeapFragmentation();
String openrfResetReason();
