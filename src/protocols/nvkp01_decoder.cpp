#include "protocols/nvkp01_decoder.h"

#include <stddef.h>

namespace {

const Nvkp01V2Decoder kDecoder;
const Nvkp01DecoderLimits kLimits;

uint32_t widthOf(int16_t pulse) {
  const int32_t value = static_cast<int32_t>(pulse);
  return static_cast<uint32_t>(value < 0 ? -value : value);
}

bool inRange(int16_t pulse, bool positive, uint16_t minUs, uint16_t maxUs) {
  if (positive ? pulse <= 0 : pulse >= 0) return false;
  const uint32_t width = widthOf(pulse);
  return width >= minUs && width <= maxUs;
}

// Ratios are hundredths. Cross multiplication avoids floating-point division
// on the RF path; int16 pulse magnitudes and these constants fit uint32_t.
bool ratioIn(uint32_t numerator, uint32_t denominator,
             uint16_t minHundredths, uint16_t maxHundredths) {
  return denominator != 0U &&
         numerator * 100U >= denominator * minHundredths &&
         numerator * 100U <= denominator * maxHundredths;
}

bool markerPair(int16_t lowPulse, int16_t highPulse) {
  return inRange(lowPulse, false, kLimits.markerLowMinUs, kLimits.markerLowMaxUs) &&
         inRange(highPulse, true, kLimits.markerHighMinUs, kLimits.markerHighMaxUs) &&
         ratioIn(widthOf(lowPulse), widthOf(highPulse), 205U, 380U);
}

bool syncPair(int16_t lowPulse, int16_t highPulse) {
  return inRange(lowPulse, false, kLimits.syncLowMinUs, kLimits.syncLowMaxUs) &&
         inRange(highPulse, true, kLimits.syncHighMinUs, kLimits.syncHighMaxUs) &&
         ratioIn(widthOf(lowPulse), widthOf(highPulse), 750U, 950U);
}

bool shortLongPair(int16_t first, int16_t second) {
  if (first == 0 || second == 0 || ((first > 0) == (second > 0))) return false;
  const uint32_t shortWidth = widthOf(first);
  const uint32_t longWidth = widthOf(second);
  return shortWidth >= kLimits.shortMinUs && shortWidth <= kLimits.shortMaxUs &&
         longWidth >= kLimits.longMinUs && longWidth <= kLimits.longMaxUs &&
         ratioIn(longWidth, shortWidth, 220U, 500U);
}

bool fullyAlternating(const int16_t* pulses, uint16_t count) {
  for (uint16_t i = 0; i < count; ++i) {
    if (pulses[i] == 0) return false;
    if (i > 0U && ((pulses[i] > 0) == (pulses[i - 1U] > 0))) return false;
  }
  return true;
}

bool relatedMarker(const int16_t* pulses, uint16_t sync, uint16_t marker) {
  return markerPair(pulses[marker], pulses[marker + 1U]) &&
         ratioIn(widthOf(pulses[sync]), widthOf(pulses[marker]), 320U, 430U) &&
         ratioIn(widthOf(pulses[sync + 1U]), widthOf(pulses[marker + 1U]),
                 85U, 165U);
}

// A1 and A2 share S + nearby M + short/long evidence. A complete M,S leader
// is useful telemetry, but is neither required nor sufficient for acceptance.
// No marker count or two-capture confirmation may bypass this gate.
bool specificStructure(const int16_t* pulses, uint16_t count) {
  for (uint16_t s = 0; s + 1U < count; ++s) {
    if (!syncPair(pulses[s], pulses[s + 1U])) continue;
    bool hasMarker = s >= 2U && relatedMarker(pulses, s, s - 2U);
    for (uint16_t m = s + 2U;
         !hasMarker && m + 1U < count && m <= s + kLimits.markerMaxOffset; ++m) {
      hasMarker = relatedMarker(pulses, s, m);
    }
    if (!hasMarker) continue;
    for (uint16_t q = s + 2U;
         q + 1U < count && q <= s + kLimits.shortLongMaxOffset; ++q) {
      if (shortLongPair(pulses[q], pulses[q + 1U])) return true;
    }
  }
  return false;
}

// Sync-free sample-derived fingerprint, NOT a general NVKP01 specification.
// Require a ~2.2ms marker pair and a nearby ~1.4ms pair, followed by
// complementary ~0.8ms cells in BOTH polarities. FIX4 adds a reinforced
// two-reverse-cell variant below. No global marker-count fallback.
bool compactStructure(const int16_t* p, uint16_t n) {
  if (n < 24U) return false;
  for (uint16_t m = 0; m + 1U < n && m <= 18U; ++m) {
    if (!inRange(p[m], false, 1300U, 1750U) ||
        !inRange(p[m + 1U], true, 600U, 760U) ||
        !ratioIn(widthOf(p[m]), widthOf(p[m + 1U]), 180U, 285U)) continue;
    const uint32_t markerPeriod = widthOf(p[m]) + widthOf(p[m + 1U]);
    if (markerPeriod < 2000U || markerPeriod > 2350U) continue;
    for (uint16_t h = m > 12U ? m - 12U : 0U;
         h + 1U < n && h <= m + 8U; ++h) {
      if (!inRange(p[h], false, 680U, 1200U) ||
          !inRange(p[h + 1U], true, 200U, 760U)) continue;
      const uint32_t headerPeriod = widthOf(p[h]) + widthOf(p[h + 1U]);
      if (headerPeriod < 1250U || headerPeriod > 1500U ||
          !ratioIn(markerPeriod, headerPeriod, 135U, 180U)) continue;
      uint8_t forward = 0U, reverse = 0U;
      uint16_t firstForward = n, previousReverse = n;
      bool repeatedMarker = false, adjacentReverse = false;
      const uint16_t start = (m < h ? m : h) + 2U;
      for (uint16_t q = start; q + 1U < n; ++q) {
        if (p[q] >= 0 || p[q + 1U] <= 0) continue;
        const uint32_t lo = widthOf(p[q]), hi = widthOf(p[q + 1U]);
        const uint32_t period = lo + hi;
        // FIX4: a second similar marker AFTER forward evidence can support
        // the two-cell variant, but only before reverse evidence starts.
        if (firstForward < q && reverse == 0U && q > m && q <= m + 18U &&
            inRange(p[q], false, 1300U, 1750U) &&
            inRange(p[q + 1U], true, 600U, 760U) &&
            ratioIn(lo, hi, 180U, 285U) && period >= 2000U && period <= 2350U &&
            ratioIn(period, markerPeriod, 90U, 110U) &&
            ratioIn(hi, widthOf(p[m + 1U]), 85U, 115U)) repeatedMarker = true;
        if (period < 730U || period > 930U ||
            !ratioIn(markerPeriod, period, 225U, 310U)) continue;
        if (lo >= 140U && lo <= 260U && hi >= 580U && hi <= 730U &&
            ratioIn(hi, lo, 220U, 500U)) {
          ++forward;
          if (firstForward == n) firstForward = q;
        }
        // Require inverse cells AFTER forward evidence, not an unordered bag.
        if (forward > 0U && lo >= 580U && lo <= 720U &&
            hi >= 140U && hi <= 260U && ratioIn(lo, hi, 220U, 500U)) {
          ++reverse;
          if (previousReverse != n && q == previousReverse + 2U)
            adjacentReverse = true;
          previousReverse = q;
        }
      }
      if (forward >= 1U && reverse >= 3U) return true;
      // Sample-derived alternative: M,H,F,...,M2,...,R,R. Preserve all
      // period/ratio gates. Two reverse cells alone are NOT sufficient.
      if (h == m + 2U && firstForward > h && firstForward < n &&
          repeatedMarker && adjacentReverse && reverse >= 2U) return true;
    }
  }
  return false;
}

// Decoder-local fixed workspace: never mutate FULL RAW. Reject zero or
// int16 overflow rather than saturating, wrapping, dropping or fabricating edges.
bool canonicalize(const int16_t* input, uint16_t n, int16_t* output,
                  uint16_t& outCount) {
  outCount = 0U;
  for (uint16_t i = 0; i < n; ++i) {
    if (input[i] == 0) return false;
    if (outCount && ((output[outCount - 1U] > 0) == (input[i] > 0))) {
      const int32_t sum = int32_t(output[outCount - 1U]) + input[i];
      if (sum < INT16_MIN || sum > INT16_MAX) return false;
      output[outCount - 1U] = static_cast<int16_t>(sum);
    } else {
      output[outCount++] = input[i];
    }
  }
  return true;
}

bool fullLeader(const int16_t* pulses, uint16_t count) {
  for (uint16_t i = 0; i + 3U < count; ++i) {
    if (syncPair(pulses[i + 2U], pulses[i + 3U]) &&
        relatedMarker(pulses, i + 2U, i)) return true;
  }
  return false;
}

}  // namespace

ProtocolMatchResult Nvkp01V2Decoder::decode(const RawCapture& capture) const {
  Nvkp01DecodeDiagnostics diagnostics;
  return decodeDetailed(capture, diagnostics);
}

ProtocolMatchResult Nvkp01V2Decoder::decodeDetailed(
    const RawCapture& capture,
    Nvkp01DecodeDiagnostics& diagnostics) const {
  diagnostics = Nvkp01DecodeDiagnostics{};
  diagnostics.pulseCount = capture.pulseCount();
  diagnostics.durationUs = capture.durationUs();

  const int16_t* const original = capture.data();
  const uint16_t originalCount = capture.pulseCount();
  const uint16_t countLimit = 70U;
  uint16_t count = originalCount;
  const int16_t* pulses = original;
  if (pulses == nullptr ||
      count < kLimits.minCapturePulses || count > kLimits.maxCapturePulses || count > countLimit ||
      capture.durationUs() < kLimits.minDurationUs ||
      capture.durationUs() > kLimits.maxDurationUs) {
    diagnostics.rejectReason = Nvkp01RejectReason::CAPTURE_ENVELOPE;
    return {};
  }

  diagnostics.alternating = fullyAlternating(original, originalCount);
  int16_t local[countLimit];
  if (!canonicalize(original, originalCount, local, count)) {
    diagnostics.rejectReason = Nvkp01RejectReason::POLARITY_SEQUENCE;
    return {};
  }
  diagnostics.canonicalPulseCount = count;
  diagnostics.mergedPulses = originalCount - count;
  if (count < kLimits.minCapturePulses) {
    diagnostics.rejectReason = Nvkp01RejectReason::CAPTURE_ENVELOPE;
    return {};
  }
  pulses = local;
  if (!fullyAlternating(pulses, count)) {
    diagnostics.rejectReason = Nvkp01RejectReason::POLARITY_SEQUENCE;
    return {};
  }

  for (uint16_t i = 0; i < count; ++i) {
    if (i + 1U < count && syncPair(pulses[i], pulses[i + 1U]) &&
        diagnostics.syncPulses < UINT8_MAX) {
      diagnostics.syncPulses++;
    }
    if (i + 1U < count && markerPair(pulses[i], pulses[i + 1U]) &&
        diagnostics.markerPairs < UINT8_MAX) {
      diagnostics.markerPairs++;
    }
  }
  diagnostics.fullLeader = fullLeader(pulses, count);

  diagnostics.syncStructure = specificStructure(pulses, count);
  diagnostics.compactStructure = compactStructure(pulses, count);
  const bool structuralMatch = diagnostics.syncStructure || diagnostics.compactStructure;
  if (!structuralMatch) {
    diagnostics.rejectReason = Nvkp01RejectReason::MARKER_STRUCTURE;
    return {};
  }

  // NVKP01's currently supported legacy model has one normalized physical
  // control. Keep the V2 event key compact and deterministic: code 1 means the
  // single logical button action for compatibility, not a proven mechanical
  // PRESS/RELEASE distinction and not a copied RF code.
  diagnostics.codeAvailable = true;
  diagnostics.normalizedCode = 1U;
  // Retained compatibility hint, not a measured count of repeated RF frames.
  diagnostics.repeatCount = diagnostics.markerPairs > 0U
                                ? diagnostics.markerPairs
                                : 1U;
  diagnostics.rejectReason = Nvkp01RejectReason::NONE;
  ProtocolMatchResult result;
  result.status = ProtocolMatchStatus::MATCH;
  return result;
}

const Nvkp01V2Decoder& nvkp01V2Decoder() { return kDecoder; }
const Nvkp01DecoderLimits& nvkp01DecoderLimits() { return kLimits; }

const char* nvkp01RejectReasonName(Nvkp01RejectReason reason) {
  switch (reason) {
    case Nvkp01RejectReason::CAPTURE_ENVELOPE: return "CAPTURE_ENVELOPE";
    case Nvkp01RejectReason::POLARITY_SEQUENCE: return "POLARITY_SEQUENCE";
    case Nvkp01RejectReason::MARKER_STRUCTURE: return "MARKER_STRUCTURE";
    default: return "NONE";
  }
}
