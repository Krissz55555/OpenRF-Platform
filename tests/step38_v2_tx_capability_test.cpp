#include <array>
#include <cstdint>
#include <cstdio>

#include "protocol_tx.h"
#include "protocols/known_protocol_library.h"

namespace {

bool encodeOne(ProtocolId protocol, uint64_t code, uint8_t symbols,
               uint16_t pulseLengthUs, uint8_t repeats) {
  const ProtocolTxEncoder* tx = knownProtocolLibraryTxEncoder(protocol);
  if (tx == nullptr) return false;

  std::array<int16_t, 128> pulses{};
  ProtocolTxRequest request;
  request.protocol = protocol;
  request.code = code;
  request.symbolCount = symbols;
  request.pulseLengthUs = pulseLengthUs;
  request.requestedRepeats = repeats;

  ProtocolTxPlan plan;
  return tx->encode(request, pulses.data(), pulses.size(), plan) &&
         plan.pulseCount > 0U && plan.transmitRepeats > 0U;
}

bool capabilityMatrix() {
  return knownProtocolLibrarySupportsTx(ProtocolId::EV1527_PRINCETON) &&
         knownProtocolLibrarySupportsTx(ProtocolId::PT2262_TRI_STATE) &&
         knownProtocolLibrarySupportsTx(ProtocolId::HT12E) &&
         !knownProtocolLibrarySupportsTx(ProtocolId::NVKP01_KINETIC) &&
         !knownProtocolLibrarySupportsTx(ProtocolId::UNKNOWN);
}

bool encodersWork() {
  if (!encodeOne(ProtocolId::EV1527_PRINCETON, 0xA5C39EU, 24U, 350U, 3U)) {
    return false;
  }
  if (!encodeOne(ProtocolId::PT2262_TRI_STATE, 0x40DD1U, 12U, 400U, 3U)) {
    return false;
  }
  // HT12E uses deterministic TX timing and ignores the learned pulse length.
  if (!encodeOne(ProtocolId::HT12E, 0xA5BU, 12U, 0U, 1U)) return false;
  return true;
}

}  // namespace

int main() {
  if (!capabilityMatrix()) return 1;
  if (!encodersWork()) return 2;
  std::puts("Step 38 V2-only protocol TX capability tests: PASS");
  return 0;
}
