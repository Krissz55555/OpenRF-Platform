#pragma once

#include <Arduino.h>
#include "universal_decoder.h"

constexpr uint16_t OPENRF_ANALYZER_RAW_PREVIEW = 192;


struct AnalyzerLiveState {
  bool available = false;
  uint32_t sequence = 0;
  uint32_t capturedAtMs = 0;
  bool candidateAvailable = false;
  uint32_t candidateSequence = 0;
  uint32_t candidateCapturedAtMs = 0;
  float currentPeakRssiDbm = -127.0F;
  uint32_t weakRssiFrames = 0;
};

struct AnalyzerCandidateSnapshot {
  bool available = false;
  uint32_t sequence = 0;
  uint32_t capturedAtMs = 0;
  float frequencyMHz = 0.0F;
  float rssiDbm = -127.0F;
  uint16_t pulseCount = 0;
  uint32_t durationUs = 0;
  String rejectReason;
  uint16_t minPulseUs = 0;
  uint16_t maxPulseUs = 0;
  uint16_t rawPulseCount = 0;
  int16_t* rawPulses = nullptr;
  uint8_t alternationRatio = 0;
  uint16_t sameSignPairs = 0;
  uint8_t longestSameSignRun = 0;
  uint16_t normalizedPulseCount = 0;
  int16_t* normalizedPulses = nullptr;
};

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
  int16_t* rawPulses = nullptr;
  uint16_t rawPulseCount = 0;
  uint16_t minPulseUs = 0;
  uint16_t maxPulseUs = 0;
  uint16_t averagePulseUs = 0;
  uint16_t shortestClassUs = 0;
  float classRatio = 0.0F;
  uint32_t decodedFrames = 0;
  uint32_t unknownFrames = 0;
  bool structuredSignal = false;
  uint8_t occurrences = 0;
  uint8_t similarity = 0;
  uint32_t weakRssiFrames = 0;
  float peakRssiDbm = -127.0F;
};

void analyzerReset();
void analyzerRecordCandidate(const int16_t* pulses, uint16_t count, uint32_t durationUs,
                             float frequencyMHz, float rssiDbm,
                             const String& rejectReason);
AnalyzerCandidateSnapshot analyzerGetLastCandidate();
void analyzerProcess(const int16_t* pulses, uint16_t count, uint32_t durationUs,
                     float frequencyMHz, float rssiDbm, bool accepted,
                     const String& rejectReason);
bool analyzerConsiderRejected(const int16_t* pulses, uint16_t count, uint32_t durationUs,
                              float frequencyMHz, float rssiDbm,
                              const String& rejectReason);
AnalyzerSnapshot analyzerGetSnapshot();
bool analyzerRssiPasses(float rssiDbm);
void analyzerRecordWeakRssi(float rssiDbm);

AnalyzerLiveState analyzerGetLiveState();

bool analyzerBegin();
size_t analyzerPsramAllocatedBytes();
bool analyzerUsingExternalRam();
