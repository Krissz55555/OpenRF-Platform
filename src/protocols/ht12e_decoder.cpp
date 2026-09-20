#include "protocols/ht12e_decoder.h"

#include <limits.h>

namespace {

const Ht12eDecoder kDecoder;
const Ht12eDecoderLimits kLimits;

uint32_t widthOf(int16_t pulse) {
  const int32_t v = static_cast<int32_t>(pulse);
  return static_cast<uint32_t>(v < 0 ? -v : v);
}

bool pctRange(uint32_t value, uint32_t base, uint16_t minPct, uint16_t maxPct) {
  if (base == 0U) return false;
  const uint32_t pct = (value * 100U + base / 2U) / base;
  return pct >= minPct && pct <= maxPct;
}

bool alternating(const int16_t* p, uint16_t n) {
  if (p == nullptr || n < 2U) return false;
  for (uint16_t i = 0; i + 1U < n; ++i) {
    if ((p[i] > 0) == (p[i + 1U] > 0)) return false;
  }
  return true;
}

void trackMinMax(uint16_t value, uint16_t& minValue, uint16_t& maxValue) {
  if (minValue == 0U || value < minValue) minValue = value;
  if (value > maxValue) maxValue = value;
}

bool decodeWordAt(const int16_t* p,
                  uint16_t n,
                  uint16_t start,
                  uint16_t& word,
                  uint16_t& tUs,
                  Ht12eDecodeDiagnostics& d) {
  // One HT12E word as observed at the demodulated DATA output:
  //   pilot LOW (~36T), sync HIGH (~T), then 12 LOW/HIGH data pairs.
  static constexpr uint16_t kPulsesPerWord = 26U;
  if (start + kPulsesPerWord > n) return false;
  if (p[start] >= 0 || p[start + 1U] <= 0) return false;

  const uint32_t pilot = widthOf(p[start]);
  const uint32_t sync = widthOf(p[start + 1U]);
  if (sync < kLimits.minTUs || sync > kLimits.maxTUs) return false;
  if (!pctRange(pilot, sync, kLimits.pilotMinT * 100U,
                kLimits.pilotMaxT * 100U)) return false;

  tUs = static_cast<uint16_t>(sync);
  word = 0U;
  trackMinMax(static_cast<uint16_t>(pilot), d.pilotMinUs, d.pilotMaxUs);

  for (uint8_t bit = 0; bit < kLimits.expectedBits; ++bit) {
    const uint16_t off = static_cast<uint16_t>(start + 2U + bit * 2U);
    if (p[off] >= 0 || p[off + 1U] <= 0) return false;
    const uint32_t low = widthOf(p[off]);
    const uint32_t high = widthOf(p[off + 1U]);
    const uint32_t pair = low + high;
    if (!pctRange(pair, sync, kLimits.pairMinPct, kLimits.pairMaxPct)) return false;

    const bool zero = pctRange(low, sync, kLimits.shortMinPct, kLimits.shortMaxPct) &&
                      pctRange(high, sync, kLimits.longMinPct, kLimits.longMaxPct);
    const bool one = pctRange(low, sync, kLimits.longMinPct, kLimits.longMaxPct) &&
                     pctRange(high, sync, kLimits.shortMinPct, kLimits.shortMaxPct);
    if (zero == one) return false;

    const uint16_t shortValue = static_cast<uint16_t>(zero ? low : high);
    const uint16_t longValue = static_cast<uint16_t>(zero ? high : low);
    trackMinMax(shortValue, d.shortMinUs, d.shortMaxUs);
    trackMinMax(longValue, d.longMinUs, d.longMaxUs);

    word = static_cast<uint16_t>((word << 1U) | (one ? 1U : 0U));
  }
  return true;
}

}  // namespace

ProtocolMatchResult Ht12eDecoder::decode(const RawCapture& capture) const {
  Ht12eDecodeDiagnostics d;
  return decodeDetailed(capture, d);
}

ProtocolMatchResult Ht12eDecoder::decodeDetailed(
    const RawCapture& capture,
    Ht12eDecodeDiagnostics& d) const {
  d = Ht12eDecodeDiagnostics{};
  d.pulseCount = capture.pulseCount();
  d.durationUs = capture.durationUs();
  const int16_t* const p = capture.data();
  const uint16_t n = capture.pulseCount();

  if (p == nullptr || n < 26U) {
    d.rejectReason = Ht12eRejectReason::CAPTURE_ENVELOPE;
    return {};
  }
  d.alternating = alternating(p, n);
  if (!d.alternating) {
    d.rejectReason = Ht12eRejectReason::POLARITY_SEQUENCE;
    return {};
  }

  bool sawPilot = false;
  bool sawSymbolFailure = false;
  uint16_t referenceWord = 0U;
  uint16_t referenceT = 0U;
  bool haveReference = false;

  for (uint16_t start = 0; start + 25U < n; ++start) {
    if (p[start] >= 0 || p[start + 1U] <= 0) continue;
    const uint32_t pilot = widthOf(p[start]);
    const uint32_t sync = widthOf(p[start + 1U]);
    if (sync < kLimits.minTUs || sync > kLimits.maxTUs) continue;
    if (!pctRange(pilot, sync, kLimits.pilotMinT * 100U,
                  kLimits.pilotMaxT * 100U)) continue;
    sawPilot = true;
    if (d.candidateWords < UINT8_MAX) d.candidateWords++;

    Ht12eDecodeDiagnostics local;
    uint16_t word = 0U;
    uint16_t tUs = 0U;
    if (!decodeWordAt(p, n, start, word, tUs, local)) {
      sawSymbolFailure = true;
      continue;
    }
    if (d.validWords < UINT8_MAX) d.validWords++;
    trackMinMax(local.pilotMinUs, d.pilotMinUs, d.pilotMaxUs);
    if (local.pilotMaxUs > d.pilotMaxUs) d.pilotMaxUs = local.pilotMaxUs;
    trackMinMax(local.shortMinUs, d.shortMinUs, d.shortMaxUs);
    if (local.shortMaxUs > d.shortMaxUs) d.shortMaxUs = local.shortMaxUs;
    trackMinMax(local.longMinUs, d.longMinUs, d.longMaxUs);
    if (local.longMaxUs > d.longMaxUs) d.longMaxUs = local.longMaxUs;

    if (!haveReference) {
      referenceWord = word;
      referenceT = tUs;
      d.matchingWords = 1U;
      haveReference = true;
    } else if (word == referenceWord) {
      if (d.matchingWords < UINT8_MAX) d.matchingWords++;
      referenceT = static_cast<uint16_t>((static_cast<uint32_t>(referenceT) + tUs) / 2U);
    }

    // Skip over the decoded word. The outer ++start then lands on the next
    // possible pulse without retaining or mutating the RAW capture.
    start = static_cast<uint16_t>(start + 25U);
  }

  if (!sawPilot) {
    d.rejectReason = Ht12eRejectReason::PILOT_SYNC;
    return {};
  }
  if (!haveReference) {
    d.rejectReason = sawSymbolFailure ? Ht12eRejectReason::SYMBOL_TIMING
                                      : Ht12eRejectReason::PILOT_SYNC;
    return {};
  }
  if (d.matchingWords < kLimits.minMatchingWords) {
    d.rejectReason = Ht12eRejectReason::REPEAT_MISMATCH;
    return {};
  }

  d.estimatedTUs = referenceT;
  d.codeAvailable = true;
  d.decodedWord = referenceWord;
  d.address = static_cast<uint8_t>((referenceWord >> 4U) & 0xFFU);
  d.data = static_cast<uint8_t>(referenceWord & 0x0FU);
  d.repeatCount = d.matchingWords;
  d.rejectReason = Ht12eRejectReason::NONE;

  ProtocolMatchResult result;
  result.status = ProtocolMatchStatus::MATCH;
  return result;
}

const Ht12eDecoder& ht12eDecoder() { return kDecoder; }
const Ht12eDecoderLimits& ht12eDecoderLimits() { return kLimits; }

const char* ht12eRejectReasonName(Ht12eRejectReason reason) {
  switch (reason) {
    case Ht12eRejectReason::CAPTURE_ENVELOPE: return "CAPTURE_ENVELOPE";
    case Ht12eRejectReason::POLARITY_SEQUENCE: return "POLARITY_SEQUENCE";
    case Ht12eRejectReason::PILOT_SYNC: return "PILOT_SYNC";
    case Ht12eRejectReason::SYMBOL_TIMING: return "SYMBOL_TIMING";
    case Ht12eRejectReason::REPEAT_MISMATCH: return "REPEAT_MISMATCH";
    default: return "NONE";
  }
}
