#include <array>
#include <cstdint>
#include <cstdio>

#include "protocol_tx.h"
#include "protocols/pt2262_tx.h"
#include "protocols/pt2262_decoder.h"
#include "protocols/known_protocol_library.h"

namespace {

uint64_t base3Code(const std::array<uint8_t, 12>& trits) {
  uint64_t code = 0U;
  for (const uint8_t trit : trits) code = code * 3U + trit;
  return code;
}

bool waveformTriState() {
  const std::array<uint8_t, 12> trits = {2, 2, 0, 1, 2, 0, 2, 1, 0, 1, 2, 0};
  std::array<int16_t, 64> pulses{};

  ProtocolTxRequest request;
  request.protocol = ProtocolId::PT2262_TRI_STATE;
  request.code = base3Code(trits);
  request.symbolCount = 12U;
  request.pulseLengthUs = 400U;
  request.requestedRepeats = 4U;

  ProtocolTxPlan plan;
  if (!pt2262TxEncoder().encode(request, pulses.data(), pulses.size(), plan)) {
    return false;
  }
  if (plan.pulseCount != 50U || plan.transmitRepeats != 4U ||
      plan.protocolRepeats != 4U) {
    return false;
  }

  for (size_t index = 0U; index < trits.size(); ++index) {
    const size_t off = index * 4U;
    switch (trits[index]) {
      case 0U:
        if (pulses[off] != 400 || pulses[off + 1U] != -1200 ||
            pulses[off + 2U] != 400 || pulses[off + 3U] != -1200) {
          return false;
        }
        break;
      case 1U:
        if (pulses[off] != 1200 || pulses[off + 1U] != -400 ||
            pulses[off + 2U] != 1200 || pulses[off + 3U] != -400) {
          return false;
        }
        break;
      case 2U:
        if (pulses[off] != 400 || pulses[off + 1U] != -1200 ||
            pulses[off + 2U] != 1200 || pulses[off + 3U] != -400) {
          return false;
        }
        break;
      default:
        return false;
    }
  }

  return pulses[48] == 400 && pulses[49] == -12400;
}

bool rejectsInvalidMetadata() {
  std::array<int16_t, 64> pulses{};
  ProtocolTxPlan plan;
  ProtocolTxRequest request;
  request.protocol = ProtocolId::PT2262_TRI_STATE;
  request.code = 0x40DD1U;
  request.symbolCount = 12U;
  request.pulseLengthUs = 400U;
  request.requestedRepeats = 1U;

  request.symbolCount = 11U;
  if (pt2262TxEncoder().encode(request, pulses.data(), pulses.size(), plan)) return false;
  request.symbolCount = 12U;
  request.pulseLengthUs = 179U;
  if (pt2262TxEncoder().encode(request, pulses.data(), pulses.size(), plan)) return false;
  request.pulseLengthUs = 701U;
  if (pt2262TxEncoder().encode(request, pulses.data(), pulses.size(), plan)) return false;
  request.pulseLengthUs = 400U;
  request.code = pt2262TxProfile().maximumCode + 1ULL;
  if (pt2262TxEncoder().encode(request, pulses.data(), pulses.size(), plan)) return false;
  request.code = 0x40DD1U;
  request.requestedRepeats = 0U;
  return !pt2262TxEncoder().encode(request, pulses.data(), pulses.size(), plan);
}

bool rejectsSmallBufferAndWrongProtocol() {
  std::array<int16_t, 49> small{};
  ProtocolTxPlan plan;
  ProtocolTxRequest request;
  request.protocol = ProtocolId::PT2262_TRI_STATE;
  request.code = 0x40DD1U;
  request.symbolCount = 12U;
  request.pulseLengthUs = 400U;
  request.requestedRepeats = 1U;
  if (pt2262TxEncoder().encode(request, small.data(), small.size(), plan)) return false;
  request.protocol = ProtocolId::EV1527_PRINCETON;
  return !pt2262TxEncoder().encode(request, small.data(), 64U, plan);
}

bool decoderRoundTrip() {
  const std::array<uint8_t, 12> trits = {2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1};
  const uint64_t expectedCode = base3Code(trits);
  std::array<int16_t, 64> frame{};

  ProtocolTxRequest request;
  request.protocol = ProtocolId::PT2262_TRI_STATE;
  request.code = expectedCode;
  request.symbolCount = 12U;
  request.pulseLengthUs = 400U;
  request.requestedRepeats = 3U;

  ProtocolTxPlan plan;
  if (!pt2262TxEncoder().encode(request, frame.data(), frame.size(), plan) ||
      plan.pulseCount != 50U) {
    return false;
  }

  std::array<int16_t, 150> raw{};
  uint32_t durationUs = 0U;
  for (size_t repeat = 0U; repeat < 3U; ++repeat) {
    for (size_t index = 0U; index < plan.pulseCount; ++index) {
      raw[repeat * plan.pulseCount + index] = frame[index];
      const int32_t value = frame[index];
      durationUs += static_cast<uint32_t>(value < 0 ? -value : value);
    }
  }

  const RawCapture capture(raw.data(), static_cast<uint16_t>(raw.size()),
                           durationUs, -35.0F, 433.92F, 1U, 1234U);
  Pt2262DecodeDiagnostics diagnostics;
  const ProtocolMatchResult decoded =
      pt2262Decoder().decodeDetailed(capture, diagnostics);
  return decoded.matched() && diagnostics.codeAvailable &&
         diagnostics.decodedCode == expectedCode &&
         diagnostics.decodedTritCount == 12U &&
         diagnostics.matchingRepeatCount >= 2U;
}

bool capabilityDiscovery() {
  if (!knownProtocolLibrarySupportsTx(ProtocolId::PT2262_TRI_STATE)) return false;
  if (knownProtocolLibraryTxEncoder(ProtocolId::PT2262_TRI_STATE) !=
      &pt2262TxEncoder()) {
    return false;
  }
  if (!knownProtocolLibrarySupportsTx(ProtocolId::EV1527_PRINCETON)) return false;
  if (!knownProtocolLibrarySupportsTx(ProtocolId::HT12E)) return false;
  if (knownProtocolLibrarySupportsTx(ProtocolId::NVKP01_KINETIC)) return false;
  if (knownProtocolLibrarySupportsTx(ProtocolId::UNKNOWN)) return false;
  return true;
}

}  // namespace

int main() {
  if (!waveformTriState()) return 1;
  if (!rejectsInvalidMetadata()) return 2;
  if (!rejectsSmallBufferAndWrongProtocol()) return 3;
  if (!decoderRoundTrip()) return 4;
  if (!capabilityDiscovery()) return 5;
  std::puts("Step 37 PT2262/Tri-State V2-native TX tests: PASS");
  return 0;
}
