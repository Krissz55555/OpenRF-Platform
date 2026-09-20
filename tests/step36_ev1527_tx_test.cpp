#include <array>
#include <cstdint>
#include <cstdio>

#include "protocol_tx.h"
#include "protocols/ev1527_tx.h"
#include "protocols/ev1527_decoder.h"
#include "protocols/known_protocol_library.h"

namespace {

bool waveformFfff09() {
  std::array<int16_t, 64> pulses{};
  ProtocolTxRequest request;
  request.protocol = ProtocolId::EV1527_PRINCETON;
  request.code = 0xFFFF09U;
  request.symbolCount = 24U;
  request.pulseLengthUs = 350U;
  request.requestedRepeats = 3U;

  ProtocolTxPlan plan;
  if (!ev1527TxEncoder().encode(request, pulses.data(), pulses.size(), plan)) {
    return false;
  }
  if (plan.pulseCount != 50U || plan.transmitRepeats != 3U ||
      plan.protocolRepeats != 3U) {
    return false;
  }

  // FFFF09 starts with 16 ones: each one is HIGH 3T, LOW 1T.
  for (uint8_t bit = 0U; bit < 16U; ++bit) {
    const size_t off = static_cast<size_t>(bit) * 2U;
    if (pulses[off] != 1050 || pulses[off + 1U] != -350) return false;
  }

  // Low byte 0x09 = 00001001, MSB first.
  const uint8_t tail = 0x09U;
  for (uint8_t bit = 0U; bit < 8U; ++bit) {
    const bool one = ((tail >> (7U - bit)) & 1U) != 0U;
    const size_t off = static_cast<size_t>(16U + bit) * 2U;
    const int16_t expectedHigh = static_cast<int16_t>(one ? 1050 : 350);
    const int16_t expectedLow = static_cast<int16_t>(one ? -350 : -1050);
    if (pulses[off] != expectedHigh || pulses[off + 1U] != expectedLow) {
      return false;
    }
  }

  // Sync pair: HIGH 1T, LOW 31T.
  return pulses[48] == 350 && pulses[49] == -10850;
}

bool rejectsInvalidMetadata() {
  std::array<int16_t, 64> pulses{};
  ProtocolTxPlan plan;
  ProtocolTxRequest request;
  request.protocol = ProtocolId::EV1527_PRINCETON;
  request.code = 0xFFFF09U;
  request.symbolCount = 24U;
  request.pulseLengthUs = 350U;
  request.requestedRepeats = 1U;

  request.symbolCount = 23U;
  if (ev1527TxEncoder().encode(request, pulses.data(), pulses.size(), plan)) {
    return false;
  }
  request.symbolCount = 24U;
  request.pulseLengthUs = 179U;
  if (ev1527TxEncoder().encode(request, pulses.data(), pulses.size(), plan)) {
    return false;
  }
  request.pulseLengthUs = 701U;
  if (ev1527TxEncoder().encode(request, pulses.data(), pulses.size(), plan)) {
    return false;
  }
  request.pulseLengthUs = 350U;
  request.code = 0x1000000ULL;
  if (ev1527TxEncoder().encode(request, pulses.data(), pulses.size(), plan)) {
    return false;
  }
  request.code = 0xFFFF09U;
  request.requestedRepeats = 0U;
  return !ev1527TxEncoder().encode(request, pulses.data(), pulses.size(), plan);
}

bool rejectsSmallBufferAndWrongProtocol() {
  std::array<int16_t, 49> small{};
  ProtocolTxPlan plan;
  ProtocolTxRequest request;
  request.protocol = ProtocolId::EV1527_PRINCETON;
  request.code = 0xFFFF09U;
  request.symbolCount = 24U;
  request.pulseLengthUs = 350U;
  request.requestedRepeats = 1U;
  if (ev1527TxEncoder().encode(request, small.data(), small.size(), plan)) {
    return false;
  }
  request.protocol = ProtocolId::HT12E;
  return !ev1527TxEncoder().encode(request, small.data(), 64U, plan);
}


bool decoderRoundTrip() {
  std::array<int16_t, 64> frame{};
  ProtocolTxRequest request;
  request.protocol = ProtocolId::EV1527_PRINCETON;
  request.code = 0xFFFF09U;
  request.symbolCount = 24U;
  request.pulseLengthUs = 350U;
  request.requestedRepeats = 2U;

  ProtocolTxPlan plan;
  if (!ev1527TxEncoder().encode(request, frame.data(), frame.size(), plan) ||
      plan.pulseCount != 50U) {
    return false;
  }

  std::array<int16_t, 100> raw{};
  uint32_t durationUs = 0U;
  for (size_t repeat = 0U; repeat < 2U; ++repeat) {
    for (size_t i = 0U; i < plan.pulseCount; ++i) {
      raw[repeat * plan.pulseCount + i] = frame[i];
      const int32_t v = frame[i];
      durationUs += static_cast<uint32_t>(v < 0 ? -v : v);
    }
  }

  const RawCapture capture(raw.data(), static_cast<uint16_t>(raw.size()),
                           durationUs, -35.0F, 433.92F, 1U, 1234U);
  Ev1527DecodeDiagnostics diagnostics;
  const ProtocolMatchResult decoded =
      ev1527Decoder().decodeDetailed(capture, diagnostics);
  return decoded.matched() && diagnostics.codeAvailable &&
         diagnostics.decodedCode == 0xFFFF09U &&
         diagnostics.matchingRepeatCount >= 2U;
}

bool capabilityDiscovery() {
  if (!knownProtocolLibrarySupportsTx(ProtocolId::EV1527_PRINCETON)) {
    return false;
  }
  if (knownProtocolLibraryTxEncoder(ProtocolId::EV1527_PRINCETON) !=
      &ev1527TxEncoder()) {
    return false;
  }
  if (!knownProtocolLibrarySupportsTx(ProtocolId::HT12E)) return false;
  if (knownProtocolLibrarySupportsTx(ProtocolId::NVKP01_KINETIC)) return false;
  return true;
}

}  // namespace

int main() {
  if (!waveformFfff09()) return 1;
  if (!rejectsInvalidMetadata()) return 2;
  if (!rejectsSmallBufferAndWrongProtocol()) return 3;
  if (!decoderRoundTrip()) return 4;
  if (!capabilityDiscovery()) return 5;
  std::puts("Step 36 EV1527 V2-native TX tests: PASS");
  return 0;
}
