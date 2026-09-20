#include <cassert>
#include <cstdint>
#include <iostream>

#include "v2_authoritative_action.h"

namespace {
NormalizedRfEvent known(ProtocolId protocol, uint64_t code) {
  NormalizedRfEvent e;
  e.available = true;
  e.protocol = protocol;
  e.code = code;
  e.symbolCount = 24;
  e.radioId = 1;
  e.capturedAtMs = 1000;
  return e;
}

NormalizedEventDedupObservation emitGate() {
  NormalizedEventDedupObservation d;
  d.state = NormalizedEventDedupState::EMIT;
  d.inputAvailable = true;
  d.logicalEventAvailable = true;
  return d;
}
}  // namespace

int main() {
  NormalizedRfEvent unknown;
  assert(!v2FirstNeedsLegacyEvaluation(true, unknown));
  assert(!v2FirstNeedsLegacyEvaluation(false, unknown));

  auto route = v2AuthoritativeDecide(true, unknown,
                                     NormalizedEventDedupObservation{});
  assert(route.state == V2AuthoritativeRouteState::V2_UNKNOWN_NO_ACTION);
  assert(route.suppressLegacyAction);
  assert(!route.emitV2Action);
  assert(!route.fallbackToLegacy);

  auto ev = known(ProtocolId::EV1527_PRINCETON, 0xFFFF09U);
  route = v2AuthoritativeDecide(true, ev, emitGate());
  assert(route.state == V2AuthoritativeRouteState::V2_EMIT);
  assert(route.emitV2Action);
  assert(!route.fallbackToLegacy);

  auto unsupported = known(static_cast<ProtocolId>(0x7F), 1U);
  route = v2AuthoritativeDecide(true, unsupported,
                                NormalizedEventDedupObservation{});
  assert(route.state == V2AuthoritativeRouteState::V2_UNSUPPORTED_NO_ACTION);
  assert(!route.emitV2Action);
  assert(!route.fallbackToLegacy);

  std::cout << "Step 39.2 legacy RX fallback retirement tests: PASS\n";
  return 0;
}
