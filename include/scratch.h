#pragma once

#include <Arduino.h>
#include "radio.h"

// Shared working buffer for non-interrupt code.
// ESP8266 runs web, MQTT and storage handlers sequentially in loop(), so a
// single buffer avoids several duplicate 4 kB allocations.
extern int16_t openrfScratch[OPENRF_MAX_RAW_PULSES];
