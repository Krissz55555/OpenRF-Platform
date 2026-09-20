#pragma once

#include <stddef.h>
#include <stdint.h>

namespace OpenRfRawMatch {

static const uint16_t kMaxPatternPulses = 512U;
static const uint16_t kMinimumUsefulPulses = 12U;
static const uint16_t kMinimumRepeatPeriodPulses = 16U;
static const uint8_t kMatchThreshold = 90U;
static const uint8_t kMinimumSignAgreement = 95U;
static const uint8_t kMinimumCountSimilarity = 80U;

struct PreparedPattern {
  uint16_t pulseCount = 0U;
  bool repeatReduced = false;
};

struct MatchResult {
  bool matched = false;
  uint8_t similarity = 0U;
  uint8_t timingSimilarity = 0U;
  uint8_t countSimilarity = 0U;
  uint8_t signAgreement = 0U;
  uint16_t comparedPulses = 0U;
};

// Merge adjacent pulses with the same sign and retain at most capacity pulses.
// The function is deterministic and performs no allocation.
uint16_t normalize(const int16_t* input, uint16_t inputCount,
                   int16_t* output, uint16_t capacity);

// Convert an arbitrary RAW burst into a stable matching pattern. Repeated
// bursts are reduced to their shortest strongly repeated pulse period so that
// different button-hold lengths still compare as the same learned signal.
PreparedPattern prepare(const int16_t* input, uint16_t inputCount,
                        int16_t* output, uint16_t capacity);

// Compare two already prepared patterns with small start-edge tolerance and
// relative timing tolerance. Polarity remains part of the signature.
MatchResult comparePrepared(const int16_t* learned, uint16_t learnedCount,
                            const int16_t* incoming, uint16_t incomingCount);

}  // namespace OpenRfRawMatch
