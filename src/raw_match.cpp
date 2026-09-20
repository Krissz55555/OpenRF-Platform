#include "raw_match.h"

#include <limits.h>

namespace OpenRfRawMatch {
namespace {

uint32_t magnitude(int16_t value) {
  const int32_t wide = static_cast<int32_t>(value);
  return static_cast<uint32_t>(wide < 0 ? -wide : wide);
}

bool sameSign(int16_t a, int16_t b) {
  return (a < 0) == (b < 0);
}

uint8_t relativeSimilarity(uint32_t a, uint32_t b) {
  if (a == 0U || b == 0U) return 0U;
  const uint32_t larger = a > b ? a : b;
  const uint32_t diff = a > b ? a - b : b - a;
  if (diff >= larger) return 0U;
  const uint32_t score = 100U - ((diff * 100U) / larger);
  return static_cast<uint8_t>(score > 100U ? 100U : score);
}

uint8_t sequenceTimingScore(const int16_t* reference,
                            const int16_t* candidate,
                            uint16_t count,
                            uint8_t& signAgreementOut) {
  if (!reference || !candidate || count == 0U) {
    signAgreementOut = 0U;
    return 0U;
  }

  uint32_t timingSum = 0U;
  uint16_t signMatches = 0U;
  for (uint16_t i = 0U; i < count; ++i) {
    if (sameSign(reference[i], candidate[i])) ++signMatches;
    timingSum += relativeSimilarity(magnitude(reference[i]),
                                    magnitude(candidate[i]));
  }

  signAgreementOut = static_cast<uint8_t>(
      (static_cast<uint32_t>(signMatches) * 100U) / count);
  return static_cast<uint8_t>(timingSum / count);
}

uint16_t detectRepeatPeriod(const int16_t* pulses, uint16_t count) {
  if (!pulses || count < kMinimumRepeatPeriodPulses * 2U) return count;

  uint16_t maximumPeriod = static_cast<uint16_t>(count / 2U);
  if (maximumPeriod > 256U) maximumPeriod = 256U;

  for (uint16_t period = kMinimumRepeatPeriodPulses;
       period <= maximumPeriod; ++period) {
    const uint16_t fullBlocks = static_cast<uint16_t>(count / period);
    if (fullBlocks < 2U) continue;

    const uint16_t blocksToCheck = fullBlocks > 5U ? 5U : fullBlocks;
    uint32_t timingSum = 0U;
    uint8_t worstTiming = 100U;
    uint8_t worstSign = 100U;

    for (uint16_t block = 1U; block < blocksToCheck; ++block) {
      uint8_t signAgreement = 0U;
      const uint8_t timing = sequenceTimingScore(
          pulses, pulses + block * period, period, signAgreement);
      timingSum += timing;
      if (timing < worstTiming) worstTiming = timing;
      if (signAgreement < worstSign) worstSign = signAgreement;
    }

    const uint8_t averageTiming = static_cast<uint8_t>(
        timingSum / static_cast<uint32_t>(blocksToCheck - 1U));

    // Two-block evidence is accepted only when it is very strong. Three or
    // more repeats may tolerate normal CC1101/OOK timing jitter.
    const uint8_t requiredAverage = blocksToCheck >= 3U ? 88U : 93U;
    const uint8_t requiredWorst = blocksToCheck >= 3U ? 78U : 88U;
    if (averageTiming >= requiredAverage && worstTiming >= requiredWorst &&
        worstSign >= 98U) {
      return period;
    }
  }

  return count;
}

MatchResult compareWithShift(const int16_t* learned, uint16_t learnedCount,
                             const int16_t* incoming, uint16_t incomingCount,
                             int8_t shift) {
  MatchResult result;
  if (!learned || !incoming || learnedCount == 0U || incomingCount == 0U) {
    return result;
  }

  uint16_t learnedStart = 0U;
  uint16_t incomingStart = 0U;
  if (shift > 0) {
    incomingStart = static_cast<uint16_t>(shift);
  } else if (shift < 0) {
    learnedStart = static_cast<uint16_t>(-shift);
  }
  if (learnedStart >= learnedCount || incomingStart >= incomingCount) {
    return result;
  }

  const uint16_t learnedRemaining =
      static_cast<uint16_t>(learnedCount - learnedStart);
  const uint16_t incomingRemaining =
      static_cast<uint16_t>(incomingCount - incomingStart);
  const uint16_t compared = learnedRemaining < incomingRemaining
                                ? learnedRemaining
                                : incomingRemaining;
  if (compared < kMinimumUsefulPulses) return result;

  const uint16_t smallerCount = learnedCount < incomingCount
                                    ? learnedCount
                                    : incomingCount;
  const uint16_t largerCount = learnedCount > incomingCount
                                   ? learnedCount
                                   : incomingCount;
  const uint8_t countSimilarity = static_cast<uint8_t>(
      (static_cast<uint32_t>(smallerCount) * 100U) / largerCount);

  // Do not let a start shift discard a meaningful fraction of the shorter
  // signature. This keeps the edge tolerance from turning into substring
  // matching against unrelated signals.
  if (static_cast<uint32_t>(compared) * 100U <
      static_cast<uint32_t>(smallerCount) * 90U) {
    return result;
  }

  uint8_t signAgreement = 0U;
  const uint8_t timingSimilarity = sequenceTimingScore(
      learned + learnedStart, incoming + incomingStart, compared,
      signAgreement);

  const uint8_t combined = static_cast<uint8_t>(
      (static_cast<uint32_t>(timingSimilarity) * 85U +
       static_cast<uint32_t>(countSimilarity) * 15U) /
      100U);

  result.similarity = combined;
  result.timingSimilarity = timingSimilarity;
  result.countSimilarity = countSimilarity;
  result.signAgreement = signAgreement;
  result.comparedPulses = compared;
  result.matched = combined >= kMatchThreshold &&
                   signAgreement >= kMinimumSignAgreement &&
                   countSimilarity >= kMinimumCountSimilarity;
  return result;
}

}  // namespace

uint16_t normalize(const int16_t* input, uint16_t inputCount,
                   int16_t* output, uint16_t capacity) {
  if (!input || !output || inputCount == 0U || capacity == 0U) return 0U;

  uint16_t outCount = 0U;
  for (uint16_t i = 0U; i < inputCount; ++i) {
    const int16_t value = input[i];
    if (value == 0) continue;

    if (outCount > 0U && sameSign(output[outCount - 1U], value)) {
      int32_t merged = static_cast<int32_t>(output[outCount - 1U]) +
                       static_cast<int32_t>(value);
      if (merged > INT16_MAX) merged = INT16_MAX;
      if (merged < INT16_MIN + 1) merged = INT16_MIN + 1;
      output[outCount - 1U] = static_cast<int16_t>(merged);
      continue;
    }

    if (outCount >= capacity) break;
    output[outCount++] = value;
  }
  return outCount;
}

PreparedPattern prepare(const int16_t* input, uint16_t inputCount,
                        int16_t* output, uint16_t capacity) {
  PreparedPattern prepared;
  if (!input || !output || capacity == 0U) return prepared;

  int16_t normalized[kMaxPatternPulses];
  const uint16_t normalizedCount = normalize(
      input, inputCount, normalized,
      capacity < kMaxPatternPulses ? capacity : kMaxPatternPulses);
  if (normalizedCount == 0U) return prepared;

  const uint16_t period = detectRepeatPeriod(normalized, normalizedCount);
  uint16_t copyCount = period;
  if (copyCount > capacity) copyCount = capacity;
  for (uint16_t i = 0U; i < copyCount; ++i) output[i] = normalized[i];

  prepared.pulseCount = copyCount;
  prepared.repeatReduced = period < normalizedCount;
  return prepared;
}

MatchResult comparePrepared(const int16_t* learned, uint16_t learnedCount,
                            const int16_t* incoming, uint16_t incomingCount) {
  MatchResult best;
  if (!learned || !incoming || learnedCount < kMinimumUsefulPulses ||
      incomingCount < kMinimumUsefulPulses) {
    return best;
  }

  for (int8_t shift = -2; shift <= 2; ++shift) {
    const MatchResult candidate = compareWithShift(
        learned, learnedCount, incoming, incomingCount, shift);
    if (candidate.similarity > best.similarity) best = candidate;
  }
  return best;
}

}  // namespace OpenRfRawMatch
