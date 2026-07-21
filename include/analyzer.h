#pragma once

#include <Arduino.h>
#include "universal_decoder.h"

constexpr uint16_t OPENRF_ANALYZER_RAW_PREVIEW = 192;

struct AnalyzerSnapshot {
  bool available = false;
  uint32_t sequence = 0;
  uint32_t capturedAtMs = 0;
  float frequencyMHz = 0.0F;
  float rssiDbm = -127.0F;
  uint16_t pulseCount = 0;
  uint32_t durationUs = 0;
  bool accepted = false;
  String status;
  String rejectReason;
  String protocol;
  String encoding;
  String deviceId;
  String command;
  uint8_t symbolCount = 0;
  uint64_t code = 0;
  uint16_t basePulseUs = 0;
  uint8_t frameCount = 0;
  uint8_t quality = 0;
  uint16_t pulseClasses[6] = {0};
  uint8_t pulseClassCount = 0;
  String bitstream;
  int16_t rawPulses[OPENRF_ANALYZER_RAW_PREVIEW] = {0};
  uint16_t rawPulseCount = 0;
  uint16_t minPulseUs = 0;
  uint16_t maxPulseUs = 0;
  uint16_t averagePulseUs = 0;
  uint16_t shortestClassUs = 0;
  float classRatio = 0.0F;
  uint32_t decodedFrames = 0;
  uint32_t unknownFrames = 0;
};

void analyzerReset();
void analyzerProcess(const int16_t* pulses, uint16_t count, uint32_t durationUs,
                     float frequencyMHz, float rssiDbm, bool accepted,
                     const String& rejectReason);
AnalyzerSnapshot analyzerGetSnapshot();
