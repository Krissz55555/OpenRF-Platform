#include <cassert>
#include <cstdint>
#include <iostream>

#include "protocols/nvkp01_decoder.h"
#include "v2_authoritative_action.h"

namespace {

NormalizedRfEvent nvEvent() {
  NormalizedRfEvent e;
  e.available = true;
  e.protocol = ProtocolId::NVKP01_KINETIC;
  e.code = 1;
  e.symbolCount = 1;
  e.repeats = 2;
  e.radioId = 1;
  e.capturedAtMs = 1000;
  e.frequencyMHz = 433.92F;
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
  const auto e = nvEvent();
  assert(v2AuthoritativeProtocolApproved(ProtocolId::NVKP01_KINETIC));
  assert(!v2FirstNeedsLegacyEvaluation(true, e));

  const auto route = v2AuthoritativeDecide(true, e, emitGate());
  assert(route.state == V2AuthoritativeRouteState::V2_EMIT);
  assert(route.suppressLegacyAction);
  assert(route.emitV2Action);
  assert(!route.fallbackToLegacy);
  assert(route.event.protocol == ProtocolId::NVKP01_KINETIC);
  assert(route.event.code == 1);

  std::cout << "Step 39.1 NVKP01 V2-authoritative policy tests: PASS\n";
  return 0;
}
