#pragma once
#include <Arduino.h>
#include "radio.h"

// OpenRF large, non-ISR working buffers.
// Allocation is attempted in external PSRAM first and falls back to internal
// heap if PSRAM is unavailable. Radio ISR capture remains in internal RAM.
bool psramBuffersBegin();
bool psramBuffersUsingExternalRam();
int16_t* psramSystemScratch();
int16_t* psramRadioLastRaw();
int16_t* psramRadioLearnRaw();
size_t psramOpenRFAllocatedBytes();
