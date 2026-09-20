#pragma once

#include <stdint.h>

#include "normalized_rf_event.h"

// Step 29.10 shadow-only event burst collapse.
// A KNOWN normalized RF event is treated as a duplicate when the same
// protocol + numeric code + radio is observed again within this inactivity
// window. UNKNOWN / unavailable captures never touch this state.
static constexpr uint32_t kNormalizedEventDedupWindowMs = 300U;
static constexpr uint32_t kNvkp01EventDedupWindowMs = 400U;
constexpr uint32_t normalizedEventDedupWindowMs(ProtocolId protocol) {
  return protocol == ProtocolId::NVKP01_KINETIC
      ? kNvkp01EventDedupWindowMs : kNormalizedEventDedupWindowMs;
}

enum class NormalizedEventDedupState : uint8_t {
  NOT_APPLICABLE = 0,
  EMIT = 1,
  COLLAPSED = 2,
};

struct NormalizedEventDedupObservation final {
  NormalizedEventDedupState state = NormalizedEventDedupState::NOT_APPLICABLE;
  bool inputAvailable = false;
  bool logicalEventAvailable = false;
  uint32_t deltaMs = 0;
  uint16_t collapsedInBurst = 0;
};

// Stateful, fixed-memory, shadow-only dedup gate. No heap, queues or I/O.
// Call only from the Protocol Engine's single Core 1 RF path.
NormalizedEventDedupObservation normalizedEventDedupObserve(
    const NormalizedRfEvent& event);

const char* normalizedEventDedupStateName(NormalizedEventDedupState state);
