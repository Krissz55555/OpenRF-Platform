#include <array>
#include <cstdint>
#include <cstdio>

#include "protocols/ev1527_decoder.h"
#include "protocols/pt2262_decoder.h"

namespace {

constexpr uint32_t kT = 400U;
constexpr size_t kFramePulses = 50U;  // 48 data + sync high + sync low.

int16_t neg(uint32_t value) { return static_cast<int16_t>(-static_cast<int32_t>(value)); }

template <size_t N>
void appendPtFrame(std::array<int16_t, N>& out,
                   size_t base,
                   const std::array<uint8_t, 12>& trits,
                   uint32_t t = kT) {
  for (size_t i = 0; i < trits.size(); ++i) {
    const size_t o = base + i * 4U;
    const uint8_t s = trits[i];
    if (s == 0U) {
      out[o] = static_cast<int16_t>(t); out[o+1] = neg(t*3U);
      out[o+2] = static_cast<int16_t>(t); out[o+3] = neg(t*3U);
    } else if (s == 1U) {
      out[o] = static_cast<int16_t>(t*3U); out[o+1] = neg(t);
      out[o+2] = static_cast<int16_t>(t*3U); out[o+3] = neg(t);
    } else {
      out[o] = static_cast<int16_t>(t); out[o+1] = neg(t*3U);
      out[o+2] = static_cast<int16_t>(t*3U); out[o+3] = neg(t);
    }
  }
  out[base + 48U] = static_cast<int16_t>(t);
  out[base + 49U] = neg(t * 31U);
}

template <size_t N>
RawCapture captureFor(const std::array<int16_t, N>& pulses) {
  uint32_t duration = 0U;
  for (const int16_t p : pulses) duration += static_cast<uint32_t>(p < 0 ? -p : p);
  return RawCapture(pulses.data(), static_cast<uint16_t>(pulses.size()),
                    duration, -30.0F, 433.92F, 1U, 1U);
}

uint64_t base3Code(const std::array<uint8_t, 12>& trits) {
  uint64_t code = 0U;
  for (const uint8_t t : trits) code = code * 3U + t;
  return code;
}

bool repeatedTriStateMatches() {
  const std::array<uint8_t, 12> trits = {2,2,0,1,2,0,2,1,0,1,2,0};
  std::array<int16_t, kFramePulses * 4U> pulses{};
  for (size_t f = 0; f < 4U; ++f) appendPtFrame(pulses, f*kFramePulses, trits);
  Pt2262DecodeDiagnostics d;
  const auto r = pt2262Decoder().decodeDetailed(captureFor(pulses), d);
  return r.matched() && d.rejectReason == Pt2262RejectReason::NONE &&
         d.matchingRepeatCount == 4U && d.decodedTritCount == 12U &&
         d.decodedCode == base3Code(trits);
}

bool realStyleAllFloatingMatchesAndEvRejects() {
  std::array<uint8_t, 12> trits{};
  trits.fill(2U);
  std::array<int16_t, kFramePulses * 3U> pulses{};
  for (size_t f = 0; f < 3U; ++f) appendPtFrame(pulses, f*kFramePulses, trits);
  const RawCapture c = captureFor(pulses);
  Pt2262DecodeDiagnostics pt;
  Ev1527DecodeDiagnostics ev;
  const bool ptMatch = pt2262Decoder().decodeDetailed(c, pt).matched();
  const bool evMatch = ev1527Decoder().decodeDetailed(c, ev).matched();
  return ptMatch && !evMatch &&
         ev.rejectReason == Ev1527RejectReason::TRISTATE_LOOKALIKE;
}

bool ev1527PatternIsNotPt2262() {
  constexpr uint32_t code = 0xFFFF09U;
  std::array<int16_t, kFramePulses * 3U> pulses{};
  for (size_t frame = 0; frame < 3U; ++frame) {
    const size_t base = frame * kFramePulses;
    for (size_t bit = 0; bit < 24U; ++bit) {
      const bool one = ((code >> (23U-bit)) & 1U) != 0U;
      pulses[base + bit*2U] = static_cast<int16_t>(one ? kT*3U : kT);
      pulses[base + bit*2U + 1U] = neg(one ? kT : kT*3U);
    }
    pulses[base + 48U] = static_cast<int16_t>(kT);
    pulses[base + 49U] = neg(kT*31U);
  }
  return !pt2262Decoder().decode(captureFor(pulses)).matched();
}

bool singleFrameRejected() {
  const std::array<uint8_t, 12> trits = {0,1,2,0,1,2,0,1,2,0,1,2};
  std::array<int16_t, kFramePulses> pulses{};
  appendPtFrame(pulses, 0U, trits);
  Pt2262DecodeDiagnostics d;
  const auto r = pt2262Decoder().decodeDetailed(captureFor(pulses), d);
  return !r.matched() && d.rejectReason == Pt2262RejectReason::INSUFFICIENT_REPEATS;
}

bool malformedSymbolRejected() {
  const std::array<uint8_t, 12> trits = {0,1,2,0,1,2,0,1,2,0,1,2};
  std::array<int16_t, kFramePulses * 2U> pulses{};
  appendPtFrame(pulses, 0U, trits);
  appendPtFrame(pulses, kFramePulses, trits);
  // Turn one trit into S,S,S,S: not 0/1/F.
  pulses[8] = static_cast<int16_t>(kT);
  pulses[9] = neg(kT);
  pulses[10] = static_cast<int16_t>(kT);
  pulses[11] = neg(kT);
  pulses[kFramePulses + 8] = static_cast<int16_t>(kT);
  pulses[kFramePulses + 9] = neg(kT);
  pulses[kFramePulses + 10] = static_cast<int16_t>(kT);
  pulses[kFramePulses + 11] = neg(kT);
  Pt2262DecodeDiagnostics d;
  const auto r = pt2262Decoder().decodeDetailed(captureFor(pulses), d);
  return !r.matched() && d.rejectReason == Pt2262RejectReason::INVALID_TRISTATE_SYMBOL;
}

bool partialBoundariesIgnored() {
  const std::array<uint8_t, 12> trits = {2,0,1,2,0,1,2,0,1,2,0,1};
  constexpr size_t prefix = 13U;
  constexpr size_t tail = 9U;
  std::array<int16_t, prefix + kFramePulses*3U + tail> pulses{};
  for (size_t i=0;i<prefix-1U;++i) pulses[i] = (i%2U)==0U ? static_cast<int16_t>(kT) : neg(kT*3U);
  pulses[prefix-1U] = neg(kT*31U);
  for (size_t f=0;f<3U;++f) appendPtFrame(pulses, prefix+f*kFramePulses, trits);
  for (size_t i=prefix+kFramePulses*3U;i<pulses.size();++i) pulses[i] = ((i-(prefix+kFramePulses*3U))%2U)==0U ? static_cast<int16_t>(kT) : neg(kT*3U);
  return pt2262Decoder().decode(captureFor(pulses)).matched();
}

}  // namespace

int main() {
  if (!repeatedTriStateMatches()) return 1;
  if (!realStyleAllFloatingMatchesAndEvRejects()) return 2;
  if (!ev1527PatternIsNotPt2262()) return 3;
  if (!singleFrameRejected()) return 4;
  if (!malformedSymbolRejected()) return 5;
  if (!partialBoundariesIgnored()) return 6;
  std::puts("Step 29.6 PT2262/Tri-State decoder tests: PASS");
  return 0;
}
