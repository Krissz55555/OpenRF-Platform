#include <array>
#include <cstdint>
#include <cstdio>

#include "protocols/nvkp01_decoder.h"

namespace {

template <size_t N>
RawCapture captureFor(const std::array<int16_t, N>& pulses) {
  uint32_t duration = 0U;
  for (const int16_t pulse : pulses) {
    duration += static_cast<uint32_t>(pulse < 0 ? -pulse : pulse);
  }
  return RawCapture(pulses.data(), static_cast<uint16_t>(pulses.size()),
                    duration, -35.0F, 433.92F, 1U, 1000U);
}

bool measuredSampleMatches() {
  const std::array<int16_t, 23> pulses = {
      -1502, 708, -5709, 684, -5722, 653, -959, 448, -353, 448,
      -960, 250, -167, 626, -1550, 644, -162, 357, -654, 960,
      -245, 336, -464};
  Nvkp01DecodeDiagnostics d;
  const auto r = nvkp01V2Decoder().decodeDetailed(captureFor(pulses), d);
  return r.matched() && d.fullLeader && d.markerPairs >= 1U &&
         d.syncPulses >= 1U && d.codeAvailable && d.normalizedCode == 1U;
}

bool secondMeasuredSampleMatches() {
  const std::array<int16_t, 35> pulses = {
      -1513,681,-5731,680,-3538,646,-1556,652,-966,430,-160,638,
      -155,650,-159,248,-349,438,-1570,640,-165,348,-243,567,-242,
      564,-217,175,-628,186,-635,172,-2023,166,-258};
  return nvkp01V2Decoder().decode(captureFor(pulses)).matched();
}

bool classicLongBurstRejected() {
  std::array<int16_t, 150> pulses{};
  for (size_t i = 0; i < pulses.size(); ++i) {
    pulses[i] = (i % 2U) == 0U ? 400 : -1200;
  }
  Nvkp01DecodeDiagnostics d;
  const auto r = nvkp01V2Decoder().decodeDetailed(captureFor(pulses), d);
  return !r.matched() && d.rejectReason == Nvkp01RejectReason::CAPTURE_ENVELOPE;
}

bool noiseRejected() {
  const std::array<int16_t, 24> pulses = {
      -110,80,-220,140,-90,70,-250,180,-120,60,-330,110,
      -100,90,-260,130,-75,55,-180,120,-90,60,-210,100};
  return !nvkp01V2Decoder().decode(captureFor(pulses)).matched();
}

bool brokenPolarityRejected() {
  const std::array<int16_t, 23> pulses = {
      -1502,708,-5709,684,-5722,653,-959,448,-353,448,-960,250,
      167,626,-1550,644,-162,357,-654,960,-245,336,-464};
  Nvkp01DecodeDiagnostics d;
  const auto r = nvkp01V2Decoder().decodeDetailed(captureFor(pulses), d);
  // FIX3 merges same-sign edges locally; this mutation loses Q evidence.
  return !r.matched() && d.rejectReason == Nvkp01RejectReason::MARKER_STRUCTURE;
}

}  // namespace

int main() {
  if (!measuredSampleMatches()) return 1;
  if (!secondMeasuredSampleMatches()) return 2;
  if (!classicLongBurstRejected()) return 3;
  if (!noiseRejected()) return 4;
  if (!brokenPolarityRejected()) return 5;
  std::puts("Step 29.12 NVKP01 third-module tests: PASS");
  return 0;
}
