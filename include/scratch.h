#pragma once
#include <Arduino.h>
#include "radio.h"

// Core 0 shared working buffer. On ESP32-S3 this points to PSRAM when available.
extern int16_t* openrfScratch;
