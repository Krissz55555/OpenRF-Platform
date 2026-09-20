#include <array>
#include <stddef.h>
#include <stdint.h>

#include "protocol_engine.h"
#include "protocol_registry.h"
#include "protocols/ev1527_decoder.h"

// Host build from the project root:
// c++ -std=c++11 -Wall -Wextra -Werror -Iinclude tests/step29_5_ev1527_decoder_test.cpp src/protocols/ev1527_decoder.cpp src/protocol_engine.cpp src/protocol_registry.cpp -o ev1527_decoder_test

namespace {

constexpr size_t kFramePulseCount = 50;
constexpr uint16_t kBasePulseUs = 350;
constexpr uint32_t kValidCode = 0xABCDE1U;

int16_t negativePulse(uint32_t width) {
  return static_cast<int16_t>(-static_cast<int32_t>(width));
}

template <size_t N>
void appendEv1527Frame(std::array<int16_t, N>& pulses,
                       size_t offset,
                       uint32_t code,
                       uint16_t basePulseUs) {
  for (size_t bit = 0; bit < 24; ++bit) {
    const bool one = (code & (1UL << (23U - bit))) != 0U;
    pulses[offset + bit * 2] =
        static_cast<int16_t>(basePulseUs * (one ? 3U : 1U));
    pulses[offset + bit * 2 + 1] =
        negativePulse(basePulseUs * (one ? 1U : 3U));
  }
  pulses[offset + 48] = static_cast<int16_t>(basePulseUs);
  pulses[offset + 49] = negativePulse(basePulseUs * 31U);
}

template <size_t N>
Ev1527DecodeDiagnostics diagnosticsFor(const std::array<int16_t, N>& pulses,
                                       uint16_t pulseCount,
                                       bool* matched = nullptr) {
  uint32_t durationUs = 0;
  for (size_t index = 0; index < pulseCount; ++index) {
    const int32_t pulse = pulses[index];
    durationUs += static_cast<uint32_t>(pulse < 0 ? -pulse : pulse);
  }

  const RawCapture capture(pulses.data(), pulseCount, durationUs, -60.0F,
                           433.92F, 1U, 1000U);
  Ev1527DecodeDiagnostics diagnostics;
  const ProtocolMatchResult result =
      ev1527Decoder().decodeDetailed(capture, diagnostics);
  if (matched != nullptr) *matched = result.matched();
  return diagnostics;
}

template <size_t N>
bool matches(const std::array<int16_t, N>& pulses, uint16_t pulseCount) {
  bool matched = false;
  (void)diagnosticsFor(pulses, pulseCount, &matched);
  return matched;
}

bool registryContainsExactlyOneDecoder() {
  if (!protocolEngineBegin() || !protocolEngineBegin()) return false;
  ProtocolRegistry& registry = protocolEngineRegistry();
  return registry.count() == 1U && registry.at(0) == &ev1527Decoder() &&
         registry.at(1) == nullptr;
}

bool singleFrameIsConservativelyRejected() {
  std::array<int16_t, kFramePulseCount> pulses{};
  appendEv1527Frame(pulses, 0U, kValidCode, kBasePulseUs);
  bool matched = true;
  const Ev1527DecodeDiagnostics diagnostics =
      diagnosticsFor(pulses, static_cast<uint16_t>(pulses.size()), &matched);
  return !matched && diagnostics.codeAvailable &&
         diagnostics.decodedCode == kValidCode &&
         diagnostics.rejectReason == Ev1527RejectReason::INSUFFICIENT_REPEATS;
}

bool repeatedEv1527Matches(uint32_t code = kValidCode) {
  std::array<int16_t, kFramePulseCount * 3> pulses{};
  appendEv1527Frame(pulses, 0U, code, kBasePulseUs);
  appendEv1527Frame(pulses, kFramePulseCount, code, kBasePulseUs);
  appendEv1527Frame(pulses, kFramePulseCount * 2U, code, kBasePulseUs);
  bool matched = false;
  const Ev1527DecodeDiagnostics diagnostics =
      diagnosticsFor(pulses, static_cast<uint16_t>(pulses.size()), &matched);
  return matched && diagnostics.validFrameCount == 3U &&
         diagnostics.matchingRepeatCount == 3U &&
         diagnostics.decodedCode == code;
}

bool realWhiteAndGreenPatternsMatch() {
  return repeatedEv1527Matches(0xFFFF09U) &&
         repeatedEv1527Matches(0xFFFF0BU);
}

bool leadingAndTrailingPartialAreIgnored() {
  constexpr size_t kPrefix = 19U;
  constexpr size_t kTail = 13U;
  std::array<int16_t, kPrefix + kFramePulseCount * 3U + kTail> pulses{};
  for (size_t i = 0U; i < kPrefix - 1U; ++i) {
    pulses[i] = (i & 1U) == 0U ? static_cast<int16_t>(kBasePulseUs)
                                : negativePulse(kBasePulseUs * 3U);
  }
  pulses[kPrefix - 1U] = negativePulse(kBasePulseUs * 31U);
  appendEv1527Frame(pulses, kPrefix, kValidCode, kBasePulseUs);
  appendEv1527Frame(pulses, kPrefix + kFramePulseCount, kValidCode,
                    kBasePulseUs);
  appendEv1527Frame(pulses, kPrefix + kFramePulseCount * 2U, kValidCode,
                    kBasePulseUs);
  for (size_t i = kPrefix + kFramePulseCount * 3U; i < pulses.size(); ++i) {
    pulses[i] = ((i - (kPrefix + kFramePulseCount * 3U)) & 1U) == 0U
                    ? static_cast<int16_t>(kBasePulseUs)
                    : negativePulse(kBasePulseUs * 3U);
  }
  return matches(pulses, static_cast<uint16_t>(pulses.size()));
}

bool pt2262LikeRepeatedCaptureIsRejected() {
  std::array<int16_t, kFramePulseCount * 3U> pulses{};
  for (size_t frame = 0U; frame < 3U; ++frame) {
    const size_t base = frame * kFramePulseCount;
    for (size_t trit = 0U; trit < 12U; ++trit) {
      const size_t offset = base + trit * 4U;
      // PT2262 floating symbol: short/long then long/short -> binary 01.
      pulses[offset] = static_cast<int16_t>(kBasePulseUs);
      pulses[offset + 1U] = negativePulse(kBasePulseUs * 3U);
      pulses[offset + 2U] = static_cast<int16_t>(kBasePulseUs * 3U);
      pulses[offset + 3U] = negativePulse(kBasePulseUs);
    }
    pulses[base + 48U] = static_cast<int16_t>(kBasePulseUs);
    pulses[base + 49U] = negativePulse(kBasePulseUs * 31U);
  }
  bool matched = true;
  const Ev1527DecodeDiagnostics diagnostics =
      diagnosticsFor(pulses, static_cast<uint16_t>(pulses.size()), &matched);
  return !matched &&
         diagnostics.rejectReason == Ev1527RejectReason::TRISTATE_LOOKALIKE;
}

bool malformedPulseFamilyIsRejected() {
  std::array<int16_t, kFramePulseCount * 2U> pulses{};
  for (size_t frame = 0U; frame < 2U; ++frame) {
    const size_t base = frame * kFramePulseCount;
    for (size_t bit = 0U; bit < 24U; ++bit) {
      pulses[base + bit * 2U] = static_cast<int16_t>(kBasePulseUs * 2U);
      pulses[base + bit * 2U + 1U] = negativePulse(kBasePulseUs * 2U);
    }
    pulses[base + 48U] = static_cast<int16_t>(kBasePulseUs);
    pulses[base + 49U] = negativePulse(kBasePulseUs * 31U);
  }
  return !matches(pulses, static_cast<uint16_t>(pulses.size()));
}

bool noiseIsRejected() {
  const std::array<int16_t, 20> pulses = {
      91, -72, 1300, -45, 80, -2100, 320, -51, 620, -77,
      44, -98, 2700, -63, 57, -810, 102, -39, 430, -120};
  return !matches(pulses, static_cast<uint16_t>(pulses.size()));
}

bool repeatCodeMismatchIsRejected() {
  std::array<int16_t, kFramePulseCount * 2U> pulses{};
  appendEv1527Frame(pulses, 0U, 0xFFFF09U, kBasePulseUs);
  appendEv1527Frame(pulses, kFramePulseCount, 0xFFFF0BU, kBasePulseUs);
  return !matches(pulses, static_cast<uint16_t>(pulses.size()));
}

}  // namespace

int main() {
  if (!registryContainsExactlyOneDecoder()) return 1;
  if (!singleFrameIsConservativelyRejected()) return 2;
  if (!repeatedEv1527Matches()) return 3;
  if (!realWhiteAndGreenPatternsMatch()) return 4;
  if (!leadingAndTrailingPartialAreIgnored()) return 5;
  if (!pt2262LikeRepeatedCaptureIsRejected()) return 6;
  if (!malformedPulseFamilyIsRejected()) return 7;
  if (!noiseIsRejected()) return 8;
  if (!repeatCodeMismatchIsRejected()) return 9;
  return 0;
}
