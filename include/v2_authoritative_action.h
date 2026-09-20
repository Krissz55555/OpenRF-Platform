#pragma once

#include <stdint.h>

#include "normalized_event_dedup.h"
#include "normalized_rf_event.h"
#include "protocol_decoder_v2.h"

// Step 39.2: V2-only authoritative routing. The general legacy RX fallback is
// retired. UNKNOWN/AMBIGUOUS captures remain non-actionable and are not passed
// to the legacy protocol decoder. FIX3 NVKP01 uses structural classification
// and 400ms dedup; the waiting enum/counter remain for API compatibility.
enum class V2AuthoritativeRouteState : uint8_t {
  V2_FIRST_READY = 0,
  V2_EMIT = 1,
  V2_SUPPRESSED_DUPLICATE = 2,
  V2_UNKNOWN_NO_ACTION = 3,
  V2_UNSUPPORTED_NO_ACTION = 4,
  V2_WAITING_CONFIRMATION = 5,
};

struct V2AuthoritativeRouteDecision final {
  V2AuthoritativeRouteState state =
      V2AuthoritativeRouteState::V2_FIRST_READY;
  // Retained as compatibility fields for the already-frozen Step 31/39.1 host
  // helpers. In Step 39.2 they are constant policy: legacy is always
  // suppressed and fallback is never permitted.
  bool suppressLegacyAction = true;
  bool emitV2Action = false;
  bool fallbackToLegacy = false;
  NormalizedRfEvent event;
};

struct V2AuthoritativeStatus final {
  bool enabled = true;  // V2-only is permanent from Step 39.2 onward.
  V2AuthoritativeRouteState lastState =
      V2AuthoritativeRouteState::V2_FIRST_READY;
  ProtocolId lastProtocol = ProtocolId::UNKNOWN;
  uint64_t lastCode = 0;
  uint32_t v2EmitCount = 0;
  uint32_t duplicateSuppressedCount = 0;
  uint32_t unknownNoActionCount = 0;
  uint32_t unsupportedNoActionCount = 0;
  uint32_t nvkpConfirmationPendingCount = 0;
};

constexpr bool v2AuthoritativeProtocolApproved(ProtocolId protocol) {
  return protocol == ProtocolId::EV1527_PRINCETON ||
         protocol == ProtocolId::PT2262_TRI_STATE ||
         protocol == ProtocolId::NVKP01_KINETIC ||
         protocol == ProtocolId::HT12E;
}

// Compatibility helper retained for older host tests. Step 39.2 never asks the
// legacy decoder to evaluate a capture, regardless of V2 result or requested
// rollback state.
constexpr bool v2FirstNeedsLegacyEvaluation(
    bool, const NormalizedRfEvent&) {
  return false;
}

inline V2AuthoritativeRouteDecision v2AuthoritativeDecide(
    bool,
    const NormalizedRfEvent& event,
    const NormalizedEventDedupObservation& dedup) {
  V2AuthoritativeRouteDecision decision;

  if (!event.available) {
    decision.state = V2AuthoritativeRouteState::V2_UNKNOWN_NO_ACTION;
    return decision;
  }

  decision.event = event;

  if (!v2AuthoritativeProtocolApproved(event.protocol)) {
    decision.state = V2AuthoritativeRouteState::V2_UNSUPPORTED_NO_ACTION;
    return decision;
  }

  if (dedup.state == NormalizedEventDedupState::COLLAPSED) {
    decision.state =
        V2AuthoritativeRouteState::V2_SUPPRESSED_DUPLICATE;
    return decision;
  }

  if (dedup.state == NormalizedEventDedupState::EMIT &&
      dedup.logicalEventAvailable) {
    decision.state = V2AuthoritativeRouteState::V2_EMIT;
    decision.emitV2Action = true;
    return decision;
  }

  decision.state = V2AuthoritativeRouteState::V2_UNKNOWN_NO_ACTION;
  return decision;
}

// Compatibility API retained for source stability. V2-only routing is always
// enabled in Step 39.2; attempts to disable it are intentionally ignored.
bool v2AuthoritativeActionEnabled();
void v2AuthoritativeActionSetEnabled(bool enabled);

V2AuthoritativeRouteDecision v2AuthoritativeActionRoute(
    const NormalizedRfEvent& event);

V2AuthoritativeStatus v2AuthoritativeActionGetStatus();

const char* v2AuthoritativeRouteStateName(V2AuthoritativeRouteState state);
