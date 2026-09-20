#include <array>
#include <cstdint>
#include <cstdio>

#include "protocol_tx.h"
#include "protocol_tx_diagnostics.h"
#include "protocols/ht12e_tx.h"
#include "protocols/known_protocol_library.h"

namespace {

bool waveformA5B() {
  std::array<int16_t, 64> pulses{};
  ProtocolTxRequest request;
  request.protocol = ProtocolId::HT12E;
  request.code = 0xA5BU;
  request.symbolCount = 12U;
  request.requestedRepeats = 1U;

  ProtocolTxPlan plan;
  if (!ht12eTxEncoder().encode(request, pulses.data(), pulses.size(), plan)) {
    return false;
  }
  if (plan.pulseCount != 26U || plan.transmitRepeats != 5U ||
      plan.protocolRepeats != 5U) {
    return false;
  }

  // Pilot LOW 36T and sync HIGH 1T at deterministic T=400 us.
  if (pulses[0] != -14400 || pulses[1] != 400) return false;

  // A5B = 1010 0101 1011, MSB first.
  const uint16_t word = 0xA5BU;
  for (uint8_t bit = 0; bit < 12U; ++bit) {
    const bool one = ((word >> (11U - bit)) & 1U) != 0U;
    const int16_t expectedLow = static_cast<int16_t>(-(one ? 800 : 400));
    const int16_t expectedHigh = static_cast<int16_t>(one ? 400 : 800);
    const size_t off = static_cast<size_t>(2U + bit * 2U);
    if (pulses[off] != expectedLow || pulses[off + 1U] != expectedHigh) {
      return false;
    }
  }
  return true;
}

bool rejectsInvalidWidth() {
  std::array<int16_t, 64> pulses{};
  ProtocolTxPlan plan;
  ProtocolTxRequest request;
  request.protocol = ProtocolId::HT12E;
  request.symbolCount = 12U;
  request.code = 0x1000U;
  if (ht12eTxEncoder().encode(request, pulses.data(), pulses.size(), plan)) {
    return false;
  }
  request.code = 0xA5BU;
  request.symbolCount = 11U;
  return !ht12eTxEncoder().encode(request, pulses.data(), pulses.size(), plan);
}

bool rejectsWrongProtocolAndSmallBuffer() {
  std::array<int16_t, 25> small{};
  ProtocolTxPlan plan;
  ProtocolTxRequest request;
  request.protocol = ProtocolId::EV1527_PRINCETON;
  request.code = 0xA5BU;
  request.symbolCount = 12U;
  if (ht12eTxEncoder().encode(request, small.data(), small.size(), plan)) {
    return false;
  }
  request.protocol = ProtocolId::HT12E;
  return !ht12eTxEncoder().encode(request, small.data(), small.size(), plan);
}


bool diagnosticsRoundTrip() {
  protocolTxDiagnosticsRecord(ProtocolId::HT12E, 0xA5BU, 2U, 868.35F, true,
                              5U, ProtocolTxFailureReason::NONE, 1234U);
  const ProtocolTxDiagnostics d = protocolTxDiagnosticsSnapshot();
  return d.available && d.protocol == ProtocolId::HT12E && d.code == 0xA5BU &&
         d.radioId == 2U && d.success && d.repeatCount == 5U &&
         d.timestampMs == 1234U &&
         d.failureReason == ProtocolTxFailureReason::NONE;
}
bool capabilityDiscovery() {
  if (!knownProtocolLibrarySupportsTx(ProtocolId::HT12E)) return false;
  if (knownProtocolLibraryTxEncoder(ProtocolId::HT12E) != &ht12eTxEncoder()) {
    return false;
  }
  if (knownProtocolLibrarySupportsTx(ProtocolId::NVKP01_KINETIC)) return false;
  if (knownProtocolLibrarySupportsTx(ProtocolId::UNKNOWN)) return false;
  return true;
}

}  // namespace

int main() {
  if (!waveformA5B()) return 1;
  if (!rejectsInvalidWidth()) return 2;
  if (!rejectsWrongProtocolAndSmallBuffer()) return 3;
  if (!capabilityDiscovery()) return 4;
  if (!diagnosticsRoundTrip()) return 5;
  std::puts("Step 34 HT12E modular TX tests: PASS");
  return 0;
}
