#include "protocols/pt2262_decoder.h"

#include <stddef.h>
#include <stdint.h>

namespace {

constexpr size_t kTritCount = 12U;
constexpr size_t kDataPulseCount = kTritCount * 4U;
constexpr size_t kMinimumSegmentPulses = kDataPulseCount;
constexpr size_t kMaximumSegmentPulses = kDataPulseCount + 1U;
constexpr size_t kMaximumEvidenceFrames = 8U;

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
  uint64_t code = 0U;
  uint32_t basePulseUs = 0U;
  uint8_t zeroCount = 0U;
  uint8_t oneCount = 0U;
  uint8_t floatingCount = 0U;
  size_t frameIndex = 0U;
};

uint32_t pulseWidth(int16_t pulse) {
  const int32_t value = static_cast<int32_t>(pulse);
  return static_cast<uint32_t>(value < 0 ? -value : value);
}

uint16_t ratioX100(uint32_t large, uint32_t small) {
  if (small == 0U) return 0U;
  const uint64_t value =
      (static_cast<uint64_t>(large) * 100U + small / 2U) / small;
  return static_cast<uint16_t>(value > 0xFFFFU ? 0xFFFFU : value);
}

uint16_t percentDeviationX10(uint32_t actual, uint32_t reference) {
  if (reference == 0U) return 0U;
  const uint32_t difference = actual > reference ? actual - reference
                                                  : reference - actual;
  const uint64_t value =
      (static_cast<uint64_t>(difference) * 1000U + reference / 2U) / reference;
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

void setReject(Pt2262DecodeDiagnostics& diagnostics,
               Pt2262RejectReason reason,
               size_t frameIndex) {
  diagnostics.rejectReason = reason;
  if (diagnostics.firstFailingFrame < 0 && frameIndex <= 127U) {
    diagnostics.firstFailingFrame = static_cast<int8_t>(frameIndex);
  }
}

void notePulsePair(Pt2262DecodeDiagnostics& diagnostics,
                   uint32_t first,
                   uint32_t second) {
  const uint32_t shortWidth = first < second ? first : second;
  const uint32_t longWidth = first < second ? second : first;
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
  if (usable < 24U || minimum == 0xFFFFFFFFU || maximum <= minimum)
    return result;

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
      centerRatio > kMaximumCenterRatioX100)
    return result;

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
                       kClassificationTolerancePercent))
      return false;
    isLong = false;
    return true;
  }
  if (!isNearPercent(value, centers.longUs,
                     kClassificationTolerancePercent))
    return false;
  isLong = true;
  return true;
}

bool decodeSegment(const int16_t* raw,
                   size_t start,
                   size_t end,
                   const int16_t* syncGapPulse,
                   const PulseCenters& centers,
                   size_t frameIndex,
                   FrameEvidence& evidence,
                   Pt2262DecodeDiagnostics& diagnostics) {
  while (start < end && raw[start] < 0) start++;
  const size_t count = end > start ? end - start : 0U;
  if (count < kMinimumSegmentPulses || count > kMaximumSegmentPulses) {
    setReject(diagnostics, Pt2262RejectReason::PULSE_COUNT, frameIndex);
    return false;
  }

  if (count == kMaximumSegmentPulses) {
    if (raw[start + kDataPulseCount] <= 0) {
      setReject(diagnostics, Pt2262RejectReason::POLARITY_SEQUENCE, frameIndex);
      return false;
    }
    bool syncHighLong = false;
    if (!classifyPulse(pulseWidth(raw[start + kDataPulseCount]), centers,
                       syncHighLong) ||
        syncHighLong) {
      setReject(diagnostics, Pt2262RejectReason::SYNC_TIMING, frameIndex);
      return false;
    }
  }

  if (syncGapPulse != nullptr) {
    if (*syncGapPulse >= 0) {
      setReject(diagnostics, Pt2262RejectReason::POLARITY_SEQUENCE, frameIndex);
      return false;
    }
    diagnostics.syncLowAvailable = true;
    diagnostics.observedSyncLowTX100 =
        ratioX100(pulseWidth(*syncGapPulse), centers.shortUs);
    if (diagnostics.observedSyncLowTX100 < kMinimumSyncLowTX100 ||
        diagnostics.observedSyncLowTX100 > kMaximumSyncLowTX100) {
      setReject(diagnostics, Pt2262RejectReason::SYNC_TIMING, frameIndex);
      return false;
    }
  }

  uint64_t code = 0U;
  uint64_t shortAccumulator = 0U;
  uint32_t shortSamples = 0U;
  uint8_t zeroCount = 0U;
  uint8_t oneCount = 0U;
  uint8_t floatingCount = 0U;

  for (size_t trit = 0U; trit < kTritCount; ++trit) {
    const size_t offset = start + trit * 4U;
    if (!(raw[offset] > 0 && raw[offset + 1U] < 0 &&
          raw[offset + 2U] > 0 && raw[offset + 3U] < 0)) {
      setReject(diagnostics, Pt2262RejectReason::POLARITY_SEQUENCE, frameIndex);
      return false;
    }

    bool isLong[4] = {false, false, false, false};
    for (size_t pulse = 0U; pulse < 4U; ++pulse) {
      if (!classifyPulse(pulseWidth(raw[offset + pulse]), centers,
                         isLong[pulse])) {
        setReject(diagnostics, Pt2262RejectReason::INVALID_TRISTATE_SYMBOL,
                  frameIndex);
        return false;
      }
    }
    notePulsePair(diagnostics, pulseWidth(raw[offset]),
                  pulseWidth(raw[offset + 1U]));
    notePulsePair(diagnostics, pulseWidth(raw[offset + 2U]),
                  pulseWidth(raw[offset + 3U]));

    uint8_t symbol = 0U;
    if (!isLong[0] && isLong[1] && !isLong[2] && isLong[3]) {
      symbol = 0U;
      zeroCount++;
    } else if (isLong[0] && !isLong[1] && isLong[2] && !isLong[3]) {
      symbol = 1U;
      oneCount++;
    } else if (!isLong[0] && isLong[1] && isLong[2] && !isLong[3]) {
      symbol = 2U;
      floatingCount++;
    } else {
      setReject(diagnostics, Pt2262RejectReason::INVALID_TRISTATE_SYMBOL,
                frameIndex);
      return false;
    }

    code = code * 3U + symbol;
    for (size_t pulse = 0U; pulse < 4U; ++pulse) {
      if (!isLong[pulse]) {
        shortAccumulator += pulseWidth(raw[offset + pulse]);
        shortSamples++;
      }
    }
  }

  evidence.code = code;
  evidence.basePulseUs = shortSamples == 0U
                             ? centers.shortUs
                             : static_cast<uint32_t>(
                                   (shortAccumulator + shortSamples / 2U) /
                                   shortSamples);
  evidence.zeroCount = zeroCount;
  evidence.oneCount = oneCount;
  evidence.floatingCount = floatingCount;
  evidence.frameIndex = frameIndex;
  return true;
}

const Pt2262Decoder decoder;
const Pt2262DecoderLimits limits;

}  // namespace

ProtocolId Pt2262Decoder::protocolId() const {
  return ProtocolId::PT2262_TRI_STATE;
}

ProtocolMatchResult Pt2262Decoder::decode(const RawCapture& capture) const {
  Pt2262DecodeDiagnostics diagnostics;
  return decodeDetailed(capture, diagnostics);
}

ProtocolMatchResult Pt2262Decoder::decodeDetailed(
    const RawCapture& capture, Pt2262DecodeDiagnostics& diagnostics) const {
  diagnostics = Pt2262DecodeDiagnostics{};
  diagnostics.available = true;
  diagnostics.totalPulseCount = static_cast<uint16_t>(capture.pulseCount());

  if (capture.empty() || capture.pulseCount() < kMinimumSegmentPulses) {
    diagnostics.rejectReason = Pt2262RejectReason::PULSE_COUNT;
    return {};
  }

  const int16_t* const raw = capture.data();
  const size_t pulseCount = capture.pulseCount();
  const PulseCenters centers = estimatePulseCenters(raw, pulseCount);
  if (!centers.valid) {
    diagnostics.rejectReason = Pt2262RejectReason::CENTER_ESTIMATION;
    return {};
  }
  diagnostics.estimatedBasePulseUs = centers.shortUs;
  diagnostics.observedRatioMinX100 = ratioX100(centers.longUs, centers.shortUs);
  diagnostics.observedRatioMaxX100 = diagnostics.observedRatioMinX100;

  const uint32_t derivedGap = (centers.longUs * 5U + 1U) / 2U;
  const uint32_t gapThresholdUs =
      derivedGap > kMinimumGapUs ? derivedGap : kMinimumGapUs;

  FrameEvidence evidence[kMaximumEvidenceFrames];
  size_t evidenceCount = 0U;
  size_t segmentStart = 0U;
  size_t frameIndex = 0U;
  bool sawFullLengthCandidate = false;
  Pt2262RejectReason firstMeaningfulReject = Pt2262RejectReason::NONE;

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
        Pt2262DecodeDiagnostics local = diagnostics;
        if (decodeSegment(raw, segmentStart, index, gapPulse, centers,
                          frameIndex, current, local)) {
          diagnostics = local;
          if (evidenceCount < kMaximumEvidenceFrames)
            evidence[evidenceCount++] = current;
          if (diagnostics.firstValidFrame < 0 && frameIndex <= 127U)
            diagnostics.firstValidFrame = static_cast<int8_t>(frameIndex);
          if (diagnostics.validFrameCount < 0xFFU)
            diagnostics.validFrameCount++;
        } else {
          if (firstMeaningfulReject == Pt2262RejectReason::NONE)
            firstMeaningfulReject = local.rejectReason;
          diagnostics = local;
        }
      } else if (diagnostics.firstFailingFrame < 0 && frameIndex <= 127U) {
        diagnostics.firstFailingFrame = static_cast<int8_t>(frameIndex);
      }
      frameIndex++;
    }
    segmentStart = atEnd ? pulseCount : index + 1U;
  }

  if (evidenceCount == 0U) {
    diagnostics.rejectReason =
        sawFullLengthCandidate &&
                firstMeaningfulReject != Pt2262RejectReason::NONE
            ? firstMeaningfulReject
            : Pt2262RejectReason::PULSE_COUNT;
    return {};
  }

  size_t bestIndex = 0U;
  uint8_t bestRepeats = 0U;
  bool repeatTie = false;
  for (size_t i = 0U; i < evidenceCount; ++i) {
    uint8_t repeats = 0U;
    for (size_t j = 0U; j < evidenceCount; ++j)
      if (evidence[j].code == evidence[i].code) repeats++;
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
    diagnostics.rejectReason = Pt2262RejectReason::REPEAT_CODE_MISMATCH;
    return {};
  }

  diagnostics.matchingRepeatCount = bestRepeats;
  diagnostics.codeAvailable = true;
  diagnostics.decodedCode = evidence[bestIndex].code;
  diagnostics.estimatedBasePulseUs = evidence[bestIndex].basePulseUs;
  diagnostics.decodedTritCount = static_cast<uint8_t>(kTritCount);
  diagnostics.zeroTritCount = evidence[bestIndex].zeroCount;
  diagnostics.oneTritCount = evidence[bestIndex].oneCount;
  diagnostics.floatingTritCount = evidence[bestIndex].floatingCount;

  uint16_t maxDeviation = 0U;
  for (size_t index = 0U; index < evidenceCount; ++index) {
    if (evidence[index].code != evidence[bestIndex].code) continue;
    const uint16_t deviation = percentDeviationX10(
        evidence[index].basePulseUs, evidence[bestIndex].basePulseUs);
    if (deviation > maxDeviation) maxDeviation = deviation;
  }
  diagnostics.maximumRepeatBaseDeviationX10Percent = maxDeviation;
  if (maxDeviation > kRepeatBaseTolerancePercent * 10U) {
    diagnostics.rejectReason = Pt2262RejectReason::REPEAT_TIMING_MISMATCH;
    return {};
  }
  if (bestRepeats < kMinimumMatchingRepeats) {
    diagnostics.rejectReason = Pt2262RejectReason::INSUFFICIENT_REPEATS;
    return {};
  }

  diagnostics.rejectReason = Pt2262RejectReason::NONE;
  ProtocolMatchResult result;
  result.status = ProtocolMatchStatus::MATCH;
  return result;
}

const Pt2262Decoder& pt2262Decoder() { return decoder; }
const Pt2262DecoderLimits& pt2262DecoderLimits() { return limits; }

const char* pt2262RejectReasonName(Pt2262RejectReason reason) {
  switch (reason) {
    case Pt2262RejectReason::NONE: return "NONE";
    case Pt2262RejectReason::PULSE_COUNT: return "PULSE_COUNT";
    case Pt2262RejectReason::POLARITY_SEQUENCE: return "POLARITY_SEQUENCE";
    case Pt2262RejectReason::CENTER_ESTIMATION: return "CENTER_ESTIMATION";
    case Pt2262RejectReason::SYNC_TIMING: return "SYNC_TIMING";
    case Pt2262RejectReason::INVALID_TRISTATE_SYMBOL:
      return "INVALID_TRISTATE_SYMBOL";
    case Pt2262RejectReason::INSUFFICIENT_REPEATS:
      return "INSUFFICIENT_REPEATS";
    case Pt2262RejectReason::REPEAT_CODE_MISMATCH:
      return "REPEAT_CODE_MISMATCH";
    case Pt2262RejectReason::REPEAT_TIMING_MISMATCH:
      return "REPEAT_TIMING_MISMATCH";
    default: return "UNKNOWN";
  }
}
