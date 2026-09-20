#include "normalized_event_dedup.h"

namespace {

bool haveLastKnown = false;
ProtocolId lastProtocol = ProtocolId::UNKNOWN;
uint64_t lastCode = 0;
uint8_t lastRadioId = 0;
uint32_t lastSeenAtMs = 0;
uint16_t currentCollapsedInBurst = 0;

bool sameEventKey(const NormalizedRfEvent& event) {
  return haveLastKnown && event.protocol == lastProtocol &&
         event.code == lastCode && event.radioId == lastRadioId;
}

void startLogicalEvent(const NormalizedRfEvent& event) {
  haveLastKnown = true;
  lastProtocol = event.protocol;
  lastCode = event.code;
  lastRadioId = event.radioId;
  lastSeenAtMs = event.capturedAtMs;
  currentCollapsedInBurst = 0;
}

}  // namespace

NormalizedEventDedupObservation normalizedEventDedupObserve(
    const NormalizedRfEvent& event) {
  NormalizedEventDedupObservation observation;
  observation.inputAvailable = event.available;
  if (!event.available) return observation;

  if (sameEventKey(event)) {
    // Unsigned subtraction intentionally handles millis()-style wraparound.
    const uint32_t deltaMs = event.capturedAtMs - lastSeenAtMs;
    observation.deltaMs = deltaMs;
    if (deltaMs <= normalizedEventDedupWindowMs(event.protocol)) {
      observation.state = NormalizedEventDedupState::COLLAPSED;
      if (currentCollapsedInBurst < UINT16_MAX) ++currentCollapsedInBurst;
      observation.collapsedInBurst = currentCollapsedInBurst;
      // Sliding inactivity window: repeated frames keep the current RF burst
      // alive. A new logical event appears only after >300 ms of silence for
      // this same protocol/code/radio key.
      lastSeenAtMs = event.capturedAtMs;
      return observation;
    }
  }

  startLogicalEvent(event);
  observation.state = NormalizedEventDedupState::EMIT;
  observation.logicalEventAvailable = true;
  return observation;
}

const char* normalizedEventDedupStateName(NormalizedEventDedupState state) {
  switch (state) {
    case NormalizedEventDedupState::EMIT:
      return "EMIT";
    case NormalizedEventDedupState::COLLAPSED:
      return "COLLAPSED";
    default:
      return "N/A";
  }
}
