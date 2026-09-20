#include "v2_actionable_dry_run.h"

V2ActionableDryRunObservation v2ActionableDryRunObserve(
    const NormalizedRfEvent& event,
    const NormalizedEventDedupObservation& dedup) {
  V2ActionableDryRunObservation observation;

  if (!event.available) return observation;

  if (dedup.state == NormalizedEventDedupState::COLLAPSED) {
    observation.state = V2ActionableDryRunState::SUPPRESSED_DUPLICATE;
    return observation;
  }

  if (dedup.state != NormalizedEventDedupState::EMIT ||
      !dedup.logicalEventAvailable) {
    return observation;
  }

  observation.state = V2ActionableDryRunState::WOULD_EMIT;
  observation.available = true;
  observation.rxSlotCandidate = true;
  observation.mqttCandidate = true;
  observation.homeAssistantCandidate = true;
  observation.event = event;
  return observation;
}

const char* v2ActionableDryRunStateName(V2ActionableDryRunState state) {
  switch (state) {
    case V2ActionableDryRunState::WOULD_EMIT:
      return "WOULD_EMIT";
    case V2ActionableDryRunState::SUPPRESSED_DUPLICATE:
      return "SUPPRESSED_DUPLICATE";
    default:
      return "N/A";
  }
}
