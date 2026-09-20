#pragma once

#include <stdint.h>

#include "normalized_event_dedup.h"
#include "normalized_rf_event.h"

// Step 29.11 shadow-only dry-run boundary. This models the exact point where a
// deduplicated V2 logical RF event could later become actionable, but it must
// never call RX Slots, MQTT, Home Assistant, queues, callbacks or any legacy
// action path in this step.
enum class V2ActionableDryRunState : uint8_t {
  NOT_APPLICABLE = 0,
  WOULD_EMIT = 1,
  SUPPRESSED_DUPLICATE = 2,
};

struct V2ActionableDryRunObservation final {
  V2ActionableDryRunState state = V2ActionableDryRunState::NOT_APPLICABLE;
  bool available = false;

  // These flags are diagnostic intent only. No target is called by Step 29.11.
  bool rxSlotCandidate = false;
  bool mqttCandidate = false;
  bool homeAssistantCandidate = false;

  // Compact copy of the logical event that WOULD be offered to the action
  // layer. Kept only for same-capture diagnostics.
  NormalizedRfEvent event;
};

V2ActionableDryRunObservation v2ActionableDryRunObserve(
    const NormalizedRfEvent& event,
    const NormalizedEventDedupObservation& dedup);

const char* v2ActionableDryRunStateName(V2ActionableDryRunState state);
