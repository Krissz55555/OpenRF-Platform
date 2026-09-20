#include <cassert>
#include <cstdint>
#include <iostream>

#include "v2_authoritative_action.h"

namespace {

NormalizedRfEvent makeEvent(ProtocolId protocol, uint64_t code) {
  NormalizedRfEvent event;
  event.available = true;
  event.protocol = protocol;
  event.code = code;
  event.symbolCount = protocol == ProtocolId::PT2262_TRI_STATE ? 12 : 24;
  event.repeats = 4;
  event.radioId = 1;
  event.capturedAtMs = 1000;
  event.frequencyMHz = 433.92F;
  return event;
}

NormalizedEventDedupObservation emitGate() {
  NormalizedEventDedupObservation d;
  d.state = NormalizedEventDedupState::EMIT;
  d.inputAvailable = true;
  d.logicalEventAvailable = true;
  return d;
}

NormalizedEventDedupObservation collapsedGate() {
  NormalizedEventDedupObservation d;
  d.state = NormalizedEventDedupState::COLLAPSED;
  d.inputAvailable = true;
  d.logicalEventAvailable = false;
  d.collapsedInBurst = 1;
  return d;
}

}  // namespace

int main() {
  const auto ev = makeEvent(ProtocolId::EV1527_PRINCETON, 0xFFFF09);
  const auto pt = makeEvent(ProtocolId::PT2262_TRI_STATE, 0x40DD2);
  const auto ht = makeEvent(ProtocolId::HT12E, 0xA5B);
  const auto nv = makeEvent(ProtocolId::NVKP01_KINETIC, 1);

  // Legacy-only rollback still owns everything when V2-first is disabled.
  auto r = v2AuthoritativeDecide(false, ev, emitGate());
  assert(r.state == V2AuthoritativeRouteState::LEGACY_ONLY);
  assert(!r.suppressLegacyAction);
  assert(!r.emitV2Action);
  assert(r.fallbackToLegacy);


  // Step 32: V2-first mode must avoid legacy for approved KNOWN events and
  // request legacy only for UNKNOWN/unsupported captures.
  assert(!v2FirstNeedsLegacyEvaluation(true, ev));
  assert(!v2FirstNeedsLegacyEvaluation(true, pt));
  assert(!v2FirstNeedsLegacyEvaluation(true, ht));
  assert(!v2FirstNeedsLegacyEvaluation(true, nv));
  NormalizedRfEvent unavailableForRouting;
  assert(v2FirstNeedsLegacyEvaluation(true, unavailableForRouting));
  assert(v2FirstNeedsLegacyEvaluation(false, ev));

  // Approved modules may emit authoritatively when the dedup gate says EMIT.
  for (const auto& event : {ev, pt, ht, nv}) {
    r = v2AuthoritativeDecide(true, event, emitGate());
    assert(r.state == V2AuthoritativeRouteState::V2_EMIT);
    assert(r.suppressLegacyAction);
    assert(r.emitV2Action);
    assert(!r.fallbackToLegacy);
    assert(r.event.available);
    assert(r.event.protocol == event.protocol);
    assert(r.event.code == event.code);
  }

  // The same approved event inside the 300 ms burst is suppressed and must
  // not fall through to legacy, otherwise Step 31 would re-introduce duplicates.
  r = v2AuthoritativeDecide(true, ev, collapsedGate());
  assert(r.state ==
         V2AuthoritativeRouteState::V2_SUPPRESSED_DUPLICATE);
  assert(r.suppressLegacyAction);
  assert(!r.emitV2Action);
  assert(!r.fallbackToLegacy);

  // Step 39.1 promotes NVKP01 to the same V2 authoritative policy. The
  // runtime wrapper adds its two-capture confirmation gate before this pure
  // decision helper is reached.

  // UNKNOWN / AMBIGUOUS produce no normalized event and always fall back.
  NormalizedRfEvent unavailable;
  r = v2AuthoritativeDecide(true, unavailable, {});
  assert(r.state == V2AuthoritativeRouteState::LEGACY_FALLBACK);
  assert(!r.suppressLegacyAction);
  assert(!r.emitV2Action);
  assert(r.fallbackToLegacy);

  std::cout << "Step 39.1 V2 authoritative route policy tests: PASS\n";
  return 0;
}
