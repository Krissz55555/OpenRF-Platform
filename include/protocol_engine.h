#pragma once

#include <stdint.h>

#include "protocol_decoder_v2.h"
#include "normalized_rf_event.h"
#include "normalized_event_dedup.h"
#include "v2_actionable_dry_run.h"
#include "protocols/ev1527_decoder.h"
#include "protocols/pt2262_decoder.h"
#include "protocols/nvkp01_decoder.h"
#include "protocols/ht12e_decoder.h"
#include "raw_capture.h"

class ProtocolRegistry;

enum class ProtocolEngineDecisionState : uint8_t {
  UNKNOWN = 0,
  KNOWN = 1,
  AMBIGUOUS = 2,
};

static constexpr uint8_t kProtocolEngineMaxCandidates = 16;

constexpr ProtocolEngineDecisionState protocolEngineDecisionFromMatchCount(uint8_t matchCount) {
  return matchCount == 0U ? ProtocolEngineDecisionState::UNKNOWN
       : matchCount == 1U ? ProtocolEngineDecisionState::KNOWN
                          : ProtocolEngineDecisionState::AMBIGUOUS;
}

struct ProtocolEngineObservation final {
  uint8_t registeredDecoders = 0;
  uint8_t evaluatedDecoders = 0;
  uint8_t matchCount = 0;
  uint8_t noMatchCount = 0;
  ProtocolEngineDecisionState decision = ProtocolEngineDecisionState::UNKNOWN;
  uint8_t candidateCount = 0;
  ProtocolId candidates[kProtocolEngineMaxCandidates] = {};
  ProtocolId selectedProtocol = ProtocolId::UNKNOWN;
  ProtocolId primaryProtocol = ProtocolId::UNKNOWN;
  ProtocolMatchStatus primaryStatus = ProtocolMatchStatus::NO_MATCH;
  bool ev1527DiagnosticsAvailable = false;
  Ev1527DecodeDiagnostics ev1527Diagnostics;
  bool pt2262DiagnosticsAvailable = false;
  ProtocolMatchStatus pt2262Status = ProtocolMatchStatus::NO_MATCH;
  Pt2262DecodeDiagnostics pt2262Diagnostics;
  bool nvkp01DiagnosticsAvailable = false;
  ProtocolMatchStatus nvkp01Status = ProtocolMatchStatus::NO_MATCH;
  Nvkp01DecodeDiagnostics nvkp01Diagnostics;
  bool ht12eDiagnosticsAvailable = false;
  ProtocolMatchStatus ht12eStatus = ProtocolMatchStatus::NO_MATCH;
  Ht12eDecodeDiagnostics ht12eDiagnostics;

  // Step 29.8 shadow-only normalized event. Available only for a single KNOWN
  // decoder match with a complete decoded payload.
  NormalizedRfEvent normalizedEvent;

  // Step 29.10 shadow-only 300 ms burst-collapse result for the normalized
  // event above. This still does not publish or drive the legacy action path.
  NormalizedEventDedupObservation dedup;

  // Step 29.11 shadow-only dry-run of the post-dedup action boundary.
  // This never calls RX Slots, MQTT or Home Assistant.
  V2ActionableDryRunObservation actionableDryRun;
};

// Registers the Step 30 modular Known Protocol Library. Call once during setup, before
// Core 1 RF processing starts. The function is idempotent and allocates no heap.
bool protocolEngineBegin();

// V2 Protocol Engine boundary. Step 32 makes this the primary decoder in
// V2-first mode; the engine itself remains side-effect free and only returns
// a compact observation to the routing layer.
//
// The call is synchronous on Core 1. Implementations must treat capture as a
// borrowed read-only view and must not retain it after the call returns. The
// engine never publishes directly; Step 32's routing layer decides whether the
// returned KNOWN normalized event owns the action path or falls back to legacy.
ProtocolEngineObservation protocolEngineObserve(const RawCapture& capture);

// Non-owning access to the engine-owned, process-lifetime registry.
ProtocolRegistry& protocolEngineRegistry();

const char* protocolEngineDecisionStateName(ProtocolEngineDecisionState state);
