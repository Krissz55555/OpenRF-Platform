#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "normalized_event_dedup.h"
#include "v2_actionable_dry_run.h"

namespace {

NormalizedRfEvent makeEvent(uint32_t atMs, uint64_t code = 0xFFFF09ULL) {
  NormalizedRfEvent event;
  event.available = true;
  event.protocol = ProtocolId::EV1527_PRINCETON;
  event.code = code;
  event.symbolCount = 24;
  event.repeats = 4;
  event.radioId = 1;
  event.capturedAtMs = atMs;
  event.frequencyMHz = 433.92F;
  event.rssiDbm = -35.0F;
  return event;
}

}  // namespace

int main() {
  NormalizedRfEvent unavailable;
  NormalizedEventDedupObservation none;
  auto dry = v2ActionableDryRunObserve(unavailable, none);
  assert(dry.state == V2ActionableDryRunState::NOT_APPLICABLE);
  assert(!dry.available);

  const NormalizedRfEvent first = makeEvent(1000U);
  const auto firstDedup = normalizedEventDedupObserve(first);
  assert(firstDedup.state == NormalizedEventDedupState::EMIT);
  dry = v2ActionableDryRunObserve(first, firstDedup);
  assert(dry.state == V2ActionableDryRunState::WOULD_EMIT);
  assert(dry.available);
  assert(dry.rxSlotCandidate && dry.mqttCandidate && dry.homeAssistantCandidate);
  assert(dry.event.code == first.code);

  const NormalizedRfEvent duplicate = makeEvent(1200U);
  const auto duplicateDedup = normalizedEventDedupObserve(duplicate);
  assert(duplicateDedup.state == NormalizedEventDedupState::COLLAPSED);
  dry = v2ActionableDryRunObserve(duplicate, duplicateDedup);
  assert(dry.state == V2ActionableDryRunState::SUPPRESSED_DUPLICATE);
  assert(!dry.available);
  assert(!dry.rxSlotCandidate && !dry.mqttCandidate && !dry.homeAssistantCandidate);

  const NormalizedRfEvent later = makeEvent(1601U);
  const auto laterDedup = normalizedEventDedupObserve(later);
  assert(laterDedup.state == NormalizedEventDedupState::EMIT);
  dry = v2ActionableDryRunObserve(later, laterDedup);
  assert(dry.state == V2ActionableDryRunState::WOULD_EMIT);
  assert(dry.event.code == later.code);

  printf("Step 29.11 actionable event dry-run tests: PASS\n");
  return 0;
}
