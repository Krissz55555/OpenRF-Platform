#include "v2_authoritative_action.h"

#include <Arduino.h>

namespace {

portMUX_TYPE routeMux = portMUX_INITIALIZER_UNLOCKED;
V2AuthoritativeStatus status;

bool haveLastAction = false;
ProtocolId lastActionProtocol = ProtocolId::UNKNOWN;
uint64_t lastActionCode = 0;
uint8_t lastActionRadioId = 0;
uint32_t lastActionSeenAtMs = 0;
uint16_t collapsedInBurst = 0;

NormalizedEventDedupObservation authoritativeDedupLocked(
    const NormalizedRfEvent& event) {
  NormalizedEventDedupObservation observation;
  observation.inputAvailable = event.available;
  if (!event.available) return observation;

  const bool sameKey =
      haveLastAction &&
      event.protocol == lastActionProtocol &&
      event.code == lastActionCode &&
      event.radioId == lastActionRadioId;

  if (sameKey) {
    const uint32_t deltaMs = event.capturedAtMs - lastActionSeenAtMs;
    observation.deltaMs = deltaMs;
    if (deltaMs <= normalizedEventDedupWindowMs(event.protocol)) {
      observation.state = NormalizedEventDedupState::COLLAPSED;
      if (collapsedInBurst < UINT16_MAX) ++collapsedInBurst;
      observation.collapsedInBurst = collapsedInBurst;
      lastActionSeenAtMs = event.capturedAtMs;
      return observation;
    }
  }

  haveLastAction = true;
  lastActionProtocol = event.protocol;
  lastActionCode = event.code;
  lastActionRadioId = event.radioId;
  lastActionSeenAtMs = event.capturedAtMs;
  collapsedInBurst = 0;

  observation.state = NormalizedEventDedupState::EMIT;
  observation.logicalEventAvailable = true;
  return observation;
}

}  // namespace

bool v2AuthoritativeActionEnabled() {
  return true;
}

void v2AuthoritativeActionSetEnabled(bool) {
  // Step 39.2 permanently retires legacy-only rollback. Kept as a no-op for
  // source compatibility with older development code/tests.
}

V2AuthoritativeRouteDecision v2AuthoritativeActionRoute(
    const NormalizedRfEvent& event) {
  V2AuthoritativeRouteDecision decision;

  portENTER_CRITICAL(&routeMux);

  if (!event.available) {
    decision = v2AuthoritativeDecide(
        true, event, NormalizedEventDedupObservation{});
  } else if (!v2AuthoritativeProtocolApproved(event.protocol)) {
    decision = v2AuthoritativeDecide(
        true, event, NormalizedEventDedupObservation{});
  } else {
    // FIX3: a complete structural classification emits on its first capture.
    // Both mechanical shapes share code 1 and a 400ms inactivity window.
    const NormalizedEventDedupObservation dedup =
        authoritativeDedupLocked(event);
    decision = v2AuthoritativeDecide(true, event, dedup);
  }

  status.enabled = true;
  status.lastState = decision.state;
  status.lastProtocol =
      event.available ? event.protocol : ProtocolId::UNKNOWN;
  status.lastCode = event.available ? event.code : 0;

  switch (decision.state) {
    case V2AuthoritativeRouteState::V2_EMIT:
      status.v2EmitCount++;
      break;
    case V2AuthoritativeRouteState::V2_SUPPRESSED_DUPLICATE:
      status.duplicateSuppressedCount++;
      break;
    case V2AuthoritativeRouteState::V2_UNKNOWN_NO_ACTION:
      status.unknownNoActionCount++;
      break;
    case V2AuthoritativeRouteState::V2_UNSUPPORTED_NO_ACTION:
      status.unsupportedNoActionCount++;
      break;
    case V2AuthoritativeRouteState::V2_WAITING_CONFIRMATION:
      status.nvkpConfirmationPendingCount++;
      break;
    default:
      break;
  }

  portEXIT_CRITICAL(&routeMux);
  return decision;
}

V2AuthoritativeStatus v2AuthoritativeActionGetStatus() {
  V2AuthoritativeStatus copy;
  portENTER_CRITICAL(&routeMux);
  copy = status;
  copy.enabled = true;
  portEXIT_CRITICAL(&routeMux);
  return copy;
}

const char* v2AuthoritativeRouteStateName(
    V2AuthoritativeRouteState state) {
  switch (state) {
    case V2AuthoritativeRouteState::V2_EMIT:
      return "V2_EMIT";
    case V2AuthoritativeRouteState::V2_SUPPRESSED_DUPLICATE:
      return "V2_SUPPRESSED_DUPLICATE";
    case V2AuthoritativeRouteState::V2_UNKNOWN_NO_ACTION:
      return "V2_UNKNOWN_NO_ACTION";
    case V2AuthoritativeRouteState::V2_UNSUPPORTED_NO_ACTION:
      return "V2_UNSUPPORTED_NO_ACTION";
    case V2AuthoritativeRouteState::V2_WAITING_CONFIRMATION:
      return "V2_WAITING_CONFIRMATION";
    default:
      return "V2_FIRST_READY";
  }
}
