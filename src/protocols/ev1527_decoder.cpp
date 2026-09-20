#include "protocols/ev1527_decoder.h"

#include <stddef.h>
#include <stdint.h>

namespace {

constexpr size_t kBitCount = 24;
constexpr size_t kDataPulseCount = kBitCount * 2U;
constexpr size_t kMinimumSegmentPulses = kDataPulseCount;
constexpr size_t kMaximumSegmentPulses = kDataPulseCount + 1U;
constexpr size_t kMaximumEvidenceFrames = 8U;

// Step 29.5/6 deliberately follows the common, proven RC-switch family
// recognition *principles* instead of hard-coding one transmitter's command
// layout: learn short/long pulse centers from the capture, split repeats at a
// protocol-local long gap, decode complementary short/long pulse pairs, and
// require repeat agreement. No third-party source code is embedded here.
constexpr uint32_t kMinimumBasePulseUs = 180U;
constexpr uint32_t kMaximumBasePulseUs = 700U;
constexpr uint32_t kClassificationTolerancePercent = 38U;
constexpr uint16_t kMinimumCenterRatioX100 = 200U;
constexpr uint16_t kMaximumCenterRatioX100 = 450U;
constexpr uint16_t kMinimumSyncLowTX100 = 800U;
constexpr uint16_t kMaximumSyncLowTX100 = 6000U;
constexpr uint32_t kMinimumClusterPulseUs = 140U;
constexpr uint32_t kMaximumClusterPulseUs = 5000U;
constexpr uint32_t kMinimumGapUs = 3000U;
constexpr uint8_t kMinimumMatchingRepeats = 2U;
constexpr uint32_t kRepeatBaseTolerancePercent = 20U;

struct PulseCenters final {
  uint32_t shortUs = 0U;
  uint32_t longUs = 0U;
  bool valid = false;
};

struct FrameEvidence final {
  uint32_t code = 0U;
  uint32_t basePulseUs = 0U;
  size_t frameIndex = 0U;
};

uint32_t pulseWidth(int16_t pulse) {
  const int32_t value = static_cast<int32_t>(pulse);
  return static_cast<uint32_t>(value < 0 ? -value : value);
}

uint16_t ratioX100(uint32_t large, uint32_t small) {
  if (small == 0U) return 0U;
  const uint64_t scaled = static_cast<uint64_t>(large) * 100U;
  const uint64_t value = (scaled + (small / 2U)) / small;
  return static_cast<uint16_t>(value > 0xFFFFU ? 0xFFFFU : value);
}

uint16_t percentDeviationX10(uint32_t actual, uint32_t reference) {
  if (reference == 0U) return 0U;
  const uint32_t difference = actual > reference ? actual - reference
                                                  : reference - actual;
  const uint64_t scaled = static_cast<uint64_t>(difference) * 1000U;
  const uint64_t value = (scaled + (reference / 2U)) / reference;
  return static_cast<uint16_t>(value > 0xFFFFU ? 0xFFFFU : value);
}

bool isNearPercent(uint32_t actual,
                   uint32_t expected,
                   uint32_t tolerancePercent) {
  if (expected == 0U) return false;
  const uint32_t difference = actual > expected ? actual - expected
                                                 : expected - actual;
  return static_cast<uint64_t>(difference) * 100U <=
         static_cast<uint64_t>(expected) * tolerancePercent;
}

void setReject(Ev1527DecodeDiagnostics& diagnostics,
               Ev1527RejectReason reason,
               size_t frameIndex) {
  diagnostics.rejectReason = reason;
  if (diagnostics.firstFailingFrame < 0 && frameIndex <= 127U) {
    diagnostics.firstFailingFrame = static_cast<int8_t>(frameIndex);
  }
}

void noteObservedPair(Ev1527DecodeDiagnostics& diagnostics,
                      uint32_t highWidth,
                      uint32_t lowWidth) {
  const uint32_t shortWidth = highWidth < lowWidth ? highWidth : lowWidth;
  const uint32_t longWidth = highWidth < lowWidth ? lowWidth : highWidth;
  const uint16_t shortUs = static_cast<uint16_t>(shortWidth);
  const uint16_t longUs = static_cast<uint16_t>(longWidth);
  const uint16_t ratio = ratioX100(longWidth, shortWidth);

  if (!diagnostics.pulseRangeAvailable) {
    diagnostics.pulseRangeAvailable = true;
    diagnostics.observedShortMinUs = shortUs;
    diagnostics.observedShortMaxUs = shortUs;
    diagnostics.observedLongMinUs = longUs;
    diagnostics.observedLongMaxUs = longUs;
    diagnostics.observedRatioMinX100 = ratio;
    diagnostics.observedRatioMaxX100 = ratio;
    return;
  }

  if (shortUs < diagnostics.observedShortMinUs)
    diagnostics.observedShortMinUs = shortUs;
  if (shortUs > diagnostics.observedShortMaxUs)
    diagnostics.observedShortMaxUs = shortUs;
  if (longUs < diagnostics.observedLongMinUs)
    diagnostics.observedLongMinUs = longUs;
  if (longUs > diagnostics.observedLongMaxUs)
    diagnostics.observedLongMaxUs = longUs;
  if (ratio < diagnostics.observedRatioMinX100)
    diagnostics.observedRatioMinX100 = ratio;
  if (ratio > diagnostics.observedRatioMaxX100)
    diagnostics.observedRatioMaxX100 = ratio;
}

PulseCenters estimatePulseCenters(const int16_t* pulses, size_t pulseCount) {
  PulseCenters result;
  if (pulses == nullptr || pulseCount == 0U) return result;

  uint32_t minimum = 0xFFFFFFFFU;
  uint32_t maximum = 0U;
  size_t usable = 0U;
  for (size_t index = 0U; index < pulseCount; ++index) {
    const uint32_t value = pulseWidth(pulses[index]);
    if (value < kMinimumClusterPulseUs || value > kMaximumClusterPulseUs)
      continue;
    if (value < minimum) minimum = value;
    if (value > maximum) maximum = value;
    usable++;
  }

  if (usable < 24U || minimum == 0xFFFFFFFFU || maximum <= minimum) {
    return result;
  }

  uint32_t shortCenter = minimum;
  uint32_t longCenter = maximum;
  for (uint8_t pass = 0U; pass < 8U; ++pass) {
    uint64_t shortSum = 0U;
    uint64_t longSum = 0U;
    uint32_t shortCount = 0U;
    uint32_t longCount = 0U;

    for (size_t index = 0U; index < pulseCount; ++index) {
      const uint32_t value = pulseWidth(pulses[index]);
      if (value < kMinimumClusterPulseUs || value > kMaximumClusterPulseUs)
        continue;

      const uint32_t shortDistance = value > shortCenter
                                         ? value - shortCenter
                                         : shortCenter - value;
      const uint32_t longDistance = value > longCenter
                                        ? value - longCenter
                                        : longCenter - value;
      if (shortDistance <= longDistance) {
        shortSum += value;
        shortCount++;
      } else {
        longSum += value;
        longCount++;
      }
    }

    if (shortCount == 0U || longCount == 0U) return result;
    shortCenter = static_cast<uint32_t>((shortSum + shortCount / 2U) /
                                        shortCount);
    longCenter = static_cast<uint32_t>((longSum + longCount / 2U) /
                                       longCount);
    if (shortCenter > longCenter) {
      const uint32_t swap = shortCenter;
      shortCenter = longCenter;
      longCenter = swap;
    }
  }

  if (shortCenter < kMinimumBasePulseUs || shortCenter > kMaximumBasePulseUs)
    return result;

  const uint16_t centerRatio = ratioX100(longCenter, shortCenter);
  if (centerRatio < kMinimumCenterRatioX100 ||
      centerRatio > kMaximumCenterRatioX100) {
    return result;
  }

  result.shortUs = shortCenter;
  result.longUs = longCenter;
  result.valid = true;
  return result;
}

bool classifyPulse(uint32_t value,
                   const PulseCenters& centers,
                   bool& isLong) {
  const uint32_t shortDistance = value > centers.shortUs
                                     ? value - centers.shortUs
                                     : centers.shortUs - value;
  const uint32_t longDistance = value > centers.longUs
                                    ? value - centers.longUs
                                    : centers.longUs - value;

  if (shortDistance <= longDistance) {
    if (!isNearPercent(value, centers.shortUs,
                       kClassificationTolerancePercent)) {
      return false;
    }
    isLong = false;
    return true;
  }

  if (!isNearPercent(value, centers.longUs,
                     kClassificationTolerancePercent)) {
    return false;
  }
  isLong = true;
  return true;
}

// PT2262-style tri-state symbols represented as binary pulse pairs are made
// only from 00, 01 and 11 pairs in the legacy/reference model. A 24-bit word
// composed entirely of those pairs is intentionally left UNKNOWN here so a
// future tri-state decoder can claim it instead of this binary decoder.
bool looksLikeTriStateWord(uint32_t code) {
  for (size_t symbol = 0U; symbol < 12U; ++symbol) {
    const uint32_t shift = static_cast<uint32_t>((11U - symbol) * 2U);
    const uint32_t pair = (code >> shift) & 0x03U;
    if (pair == 0x02U) return false;  // 10 is not a PT2262 trit pattern.
  }
  return true;
}

bool decodeSegment(const int16_t* raw,
                   size_t start,
                   size_t end,
                   const int16_t* syncGapPulse,
                   const PulseCenters& centers,
                   size_t frameIndex,
                   FrameEvidence& evidence,
                   Ev1527DecodeDiagnostics& diagnostics) {
  while (start < end && raw[start] < 0) start++;
  const size_t segmentPulseCount = end > start ? end - start : 0U;
  if (segmentPulseCount < kMinimumSegmentPulses ||
      segmentPulseCount > kMaximumSegmentPulses) {
    setReject(diagnostics, Ev1527RejectReason::PULSE_COUNT, frameIndex);
    return false;
  }

  if (segmentPulseCount == kMaximumSegmentPulses) {
    if (raw[start + kDataPulseCount] <= 0) {
      setReject(diagnostics, Ev1527RejectReason::POLARITY_SEQUENCE, frameIndex);
      return false;
    }
    bool syncHighLong = false;
    if (!classifyPulse(pulseWidth(raw[start + kDataPulseCount]),
                       centers,
                       syncHighLong) ||
        syncHighLong) {
      setReject(diagnostics, Ev1527RejectReason::SYNC_TIMING, frameIndex);
      return false;
    }
  }

  if (syncGapPulse != nullptr) {
    if (*syncGapPulse >= 0) {
      setReject(diagnostics, Ev1527RejectReason::POLARITY_SEQUENCE, frameIndex);
      return false;
    }
    diagnostics.syncLowAvailable = true;
    diagnostics.observedSyncLowTX100 =
        ratioX100(pulseWidth(*syncGapPulse), centers.shortUs);
    if (diagnostics.observedSyncLowTX100 < kMinimumSyncLowTX100 ||
        diagnostics.observedSyncLowTX100 > kMaximumSyncLowTX100) {
      setReject(diagnostics, Ev1527RejectReason::SYNC_TIMING, frameIndex);
      return false;
    }
  }

  uint32_t code = 0U;
  uint64_t shortAccumulator = 0U;
  uint32_t shortSamples = 0U;

  for (size_t bit = 0U; bit < kBitCount; ++bit) {
    const int16_t highPulse = raw[start + bit * 2U];
    const int16_t lowPulse = raw[start + bit * 2U + 1U];
    if (highPulse <= 0 || lowPulse >= 0) {
      setReject(diagnostics, Ev1527RejectReason::POLARITY_SEQUENCE, frameIndex);
      return false;
    }

    const uint32_t highWidth = pulseWidth(highPulse);
    const uint32_t lowWidth = pulseWidth(lowPulse);
    noteObservedPair(diagnostics, highWidth, lowWidth);

    bool highLong = false;
    bool lowLong = false;
    if (!classifyPulse(highWidth, centers, highLong) ||
        !classifyPulse(lowWidth, centers, lowLong)) {
      setReject(diagnostics, Ev1527RejectReason::INVALID_SYMBOL, frameIndex);
      return false;
    }
    if (highLong == lowLong) {
      setReject(diagnostics, Ev1527RejectReason::INVALID_SYMBOL, frameIndex);
      return false;
    }

    const uint32_t shortWidth = highLong ? lowWidth : highWidth;
    shortAccumulator += shortWidth;
    shortSamples++;
    code = (code << 1U) | (highLong ? 1U : 0U);
  }

  diagnostics.codeAvailable = true;
  diagnostics.decodedCode = code;
  diagnostics.addressPattern = Ev1527CheckState::NOT_AVAILABLE;
  diagnostics.commandPattern = Ev1527CheckState::NOT_AVAILABLE;

  if (looksLikeTriStateWord(code)) {
    setReject(diagnostics, Ev1527RejectReason::TRISTATE_LOOKALIKE, frameIndex);
    return false;
  }

  evidence.code = code;
  evidence.basePulseUs = shortSamples == 0U
                             ? centers.shortUs
                             : static_cast<uint32_t>(
                                   (shortAccumulator + shortSamples / 2U) /
                                   shortSamples);
  evidence.frameIndex = frameIndex;
  return true;
}

const Ev1527Decoder decoder;
const Ev1527DecoderLimits limits;

}  // namespace

ProtocolId Ev1527Decoder::protocolId() const {
  return ProtocolId::EV1527_PRINCETON;
}

ProtocolMatchResult Ev1527Decoder::decode(const RawCapture& capture) const {
  Ev1527DecodeDiagnostics diagnostics;
  return decodeDetailed(capture, diagnostics);
}

ProtocolMatchResult Ev1527Decoder::decodeDetailed(
    const RawCapture& capture, Ev1527DecodeDiagnostics& diagnostics) const {
  diagnostics = Ev1527DecodeDiagnostics{};
  diagnostics.available = true;
  diagnostics.totalPulseCount = static_cast<uint16_t>(capture.pulseCount());

  if (capture.empty() || capture.pulseCount() < kMinimumSegmentPulses) {
    diagnostics.rejectReason = Ev1527RejectReason::PULSE_COUNT;
    return {};
  }

  const int16_t* const raw = capture.data();
  const size_t pulseCount = capture.pulseCount();
  const PulseCenters centers = estimatePulseCenters(raw, pulseCount);
  if (!centers.valid) {
    diagnostics.rejectReason = Ev1527RejectReason::CENTER_ESTIMATION;
    return {};
  }

  diagnostics.estimatedBasePulseUs = centers.shortUs;
  diagnostics.observedRatioMinX100 = ratioX100(centers.longUs, centers.shortUs);
  diagnostics.observedRatioMaxX100 = diagnostics.observedRatioMinX100;

  const uint32_t derivedGap = (centers.longUs * 5U + 1U) / 2U;
  const uint32_t gapThresholdUs = derivedGap > kMinimumGapUs
                                      ? derivedGap
                                      : kMinimumGapUs;

  FrameEvidence evidence[kMaximumEvidenceFrames];
  size_t evidenceCount = 0U;
  size_t segmentStart = 0U;
  size_t frameIndex = 0U;
  bool sawFullLengthCandidate = false;
  bool sawTriStateLookalike = false;
  Ev1527RejectReason firstMeaningfulReject = Ev1527RejectReason::NONE;

  for (size_t index = 0U; index <= pulseCount; ++index) {
    const bool atEnd = index == pulseCount;
    const bool boundary = !atEnd && pulseWidth(raw[index]) >= gapThresholdUs;
    if (!atEnd && !boundary) continue;

    size_t trimmedStart = segmentStart;
    while (trimmedStart < index && raw[trimmedStart] < 0) trimmedStart++;
    const size_t segmentPulseCount =
        index > trimmedStart ? index - trimmedStart : 0U;

    if (segmentPulseCount > 0U) {
      if (diagnostics.candidateFrameCount < 0xFFU)
        diagnostics.candidateFrameCount++;
      diagnostics.repeatFrameCount = diagnostics.candidateFrameCount;

      if (segmentPulseCount >= kMinimumSegmentPulses &&
          segmentPulseCount <= kMaximumSegmentPulses) {
        sawFullLengthCandidate = true;
        FrameEvidence current{};
        const int16_t* gapPulse = boundary ? raw + index : nullptr;
        Ev1527DecodeDiagnostics local = diagnostics;
        if (decodeSegment(raw,
                          segmentStart,
                          index,
                          gapPulse,
                          centers,
                          frameIndex,
                          current,
                          local)) {
          diagnostics = local;
          if (evidenceCount < kMaximumEvidenceFrames) {
            evidence[evidenceCount++] = current;
          }
          if (diagnostics.firstValidFrame < 0 && frameIndex <= 127U)
            diagnostics.firstValidFrame = static_cast<int8_t>(frameIndex);
          if (diagnostics.validFrameCount < 0xFFU)
            diagnostics.validFrameCount++;
        } else {
          if (local.rejectReason == Ev1527RejectReason::TRISTATE_LOOKALIKE)
            sawTriStateLookalike = true;
          if (firstMeaningfulReject == Ev1527RejectReason::NONE)
            firstMeaningfulReject = local.rejectReason;
          diagnostics = local;
        }
      } else {
        if (diagnostics.firstFailingFrame < 0 && frameIndex <= 127U)
          diagnostics.firstFailingFrame = static_cast<int8_t>(frameIndex);
      }
      frameIndex++;
    }

    segmentStart = atEnd ? pulseCount : index + 1U;
  }

  if (evidenceCount == 0U) {
    if (sawTriStateLookalike) {
      diagnostics.rejectReason = Ev1527RejectReason::TRISTATE_LOOKALIKE;
    } else if (sawFullLengthCandidate &&
               firstMeaningfulReject != Ev1527RejectReason::NONE) {
      diagnostics.rejectReason = firstMeaningfulReject;
    } else {
      diagnostics.rejectReason = Ev1527RejectReason::PULSE_COUNT;
    }
    return {};
  }

  size_t bestIndex = 0U;
  uint8_t bestRepeats = 0U;
  bool repeatTie = false;
  for (size_t i = 0U; i < evidenceCount; ++i) {
    uint8_t repeats = 0U;
    for (size_t j = 0U; j < evidenceCount; ++j) {
      if (evidence[j].code == evidence[i].code) repeats++;
    }
    if (repeats > bestRepeats) {
      bestRepeats = repeats;
      bestIndex = i;
      repeatTie = false;
    } else if (repeats == bestRepeats && repeats > 0U &&
               evidence[i].code != evidence[bestIndex].code) {
      repeatTie = true;
    }
  }

  if (repeatTie) {
    diagnostics.rejectReason = Ev1527RejectReason::REPEAT_CODE_MISMATCH;
    return {};
  }

  diagnostics.matchingRepeatCount = bestRepeats;
  diagnostics.codeAvailable = true;
  diagnostics.decodedCode = evidence[bestIndex].code;
  diagnostics.estimatedBasePulseUs = evidence[bestIndex].basePulseUs;

  uint16_t maxDeviation = 0U;
  for (size_t index = 0U; index < evidenceCount; ++index) {
    if (evidence[index].code != evidence[bestIndex].code) continue;
    const uint16_t deviation = percentDeviationX10(
        evidence[index].basePulseUs, evidence[bestIndex].basePulseUs);
    if (deviation > maxDeviation) maxDeviation = deviation;
  }
  diagnostics.maximumRepeatBaseDeviationX10Percent = maxDeviation;
  if (maxDeviation > kRepeatBaseTolerancePercent * 10U) {
    diagnostics.rejectReason = Ev1527RejectReason::REPEAT_TIMING_MISMATCH;
    return {};
  }

  if (bestRepeats < kMinimumMatchingRepeats) {
    diagnostics.rejectReason = Ev1527RejectReason::INSUFFICIENT_REPEATS;
    return {};
  }

  diagnostics.rejectReason = Ev1527RejectReason::NONE;
  ProtocolMatchResult result;
  result.status = ProtocolMatchStatus::MATCH;
  return result;
}

const Ev1527Decoder& ev1527Decoder() { return decoder; }
const Ev1527DecoderLimits& ev1527DecoderLimits() { return limits; }

const char* ev1527RejectReasonName(Ev1527RejectReason reason) {
  switch (reason) {
    case Ev1527RejectReason::NONE: return "NONE";
    case Ev1527RejectReason::PULSE_COUNT: return "PULSE_COUNT";
    case Ev1527RejectReason::POLARITY_SEQUENCE: return "POLARITY_SEQUENCE";
    case Ev1527RejectReason::CENTER_ESTIMATION: return "CENTER_ESTIMATION";
    case Ev1527RejectReason::BASE_T_OUT_OF_RANGE: return "BASE_T_OUT_OF_RANGE";
    case Ev1527RejectReason::SYNC_TIMING: return "SYNC_TIMING";
    case Ev1527RejectReason::SHORT_TIMING: return "SHORT_TIMING";
    case Ev1527RejectReason::LONG_TIMING: return "LONG_TIMING";
    case Ev1527RejectReason::SHORT_LONG_RATIO: return "SHORT_LONG_RATIO";
    case Ev1527RejectReason::PAIR_TIMING: return "PAIR_TIMING";
    case Ev1527RejectReason::INVALID_SYMBOL: return "INVALID_SYMBOL";
    case Ev1527RejectReason::TRISTATE_LOOKALIKE: return "TRISTATE_LOOKALIKE";
    case Ev1527RejectReason::INSUFFICIENT_REPEATS: return "INSUFFICIENT_REPEATS";
    case Ev1527RejectReason::REPEAT_CODE_MISMATCH:
      return "REPEAT_CODE_MISMATCH";
    case Ev1527RejectReason::REPEAT_TIMING_MISMATCH:
      return "REPEAT_TIMING_MISMATCH";
    case Ev1527RejectReason::ADDRESS_PATTERN: return "ADDRESS_PATTERN";
    case Ev1527RejectReason::COMMAND_PATTERN: return "COMMAND_PATTERN";
    default: return "UNKNOWN";
  }
}

const char* ev1527CheckStateName(Ev1527CheckState state) {
  switch (state) {
    case Ev1527CheckState::PASS: return "PASS";
    case Ev1527CheckState::FAIL: return "FAIL";
    default: return "N/A";
  }
}
