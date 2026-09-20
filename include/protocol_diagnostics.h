#pragma once

#include <stdint.h>

#include "protocol_decoder_v2.h"
#include "protocol_engine.h"
#include "normalized_rf_event.h"
#include "v2_actionable_dry_run.h"
#include "protocols/ev1527_decoder.h"
#include "protocols/pt2262_decoder.h"
#include "protocols/nvkp01_decoder.h"
#include "protocols/ht12e_decoder.h"
#include "raw_capture.h"


struct ProtocolDiagnosticsSnapshot final {
  bool available = false;
  uint32_t sequence = 0;
  uint32_t capturedAtMs = 0;
  uint8_t radioId = 0;
  uint16_t pulseCount = 0;
  uint32_t durationUs = 0;
  float frequencyMHz = 0.0F;
  float rssiDbm = -127.0F;
  bool frameAccepted = false;

  bool v2Evaluated = false;
  ProtocolEngineDecisionState v2Decision = ProtocolEngineDecisionState::UNKNOWN;
  uint8_t v2CandidateCount = 0;
  ProtocolId v2Candidates[kProtocolEngineMaxCandidates] = {};
  ProtocolId v2SelectedProtocol = ProtocolId::UNKNOWN;
  ProtocolId v2Protocol = ProtocolId::UNKNOWN;
  ProtocolMatchStatus v2Status = ProtocolMatchStatus::NO_MATCH;
  uint8_t registeredDecoders = 0;
  uint8_t evaluatedDecoders = 0;

  uint32_t v2MatchCount = 0;
  uint32_t v2NoMatchCount = 0;
  uint32_t v2UnknownDecisionCount = 0;
  uint32_t v2KnownDecisionCount = 0;
  uint32_t v2AmbiguousDecisionCount = 0;

  bool v2NormalizedEventAvailable = false;
  NormalizedRfEvent v2NormalizedEvent;
  uint32_t v2NormalizedEventCount = 0;

  // Step 35.1: latch the most recent confidently KNOWN V2 event so transient
  // UNKNOWN/noise captures cannot erase the useful protocol readout. This is
  // diagnostics-only RAM state and resets on reboot.
  bool v2LastKnownEventAvailable = false;
  NormalizedRfEvent v2LastKnownEvent;

  NormalizedEventDedupObservation v2Dedup;
  uint32_t v2LogicalEventCount = 0;
  uint32_t v2CollapsedEventCount = 0;

  V2ActionableDryRunObservation v2ActionableDryRun;
  uint32_t v2ActionableWouldEmitCount = 0;
  uint32_t v2ActionableSuppressedDuplicateCount = 0;

  bool ev1527DiagnosticsAvailable = false;
  Ev1527DecodeDiagnostics ev1527;
  uint32_t ev1527RejectCounts[kEv1527RejectReasonCount] = {0};

  bool pt2262DiagnosticsAvailable = false;
  ProtocolMatchStatus pt2262Status = ProtocolMatchStatus::NO_MATCH;
  Pt2262DecodeDiagnostics pt2262;
  uint32_t pt2262RejectCounts[kPt2262RejectReasonCount] = {0};

  bool nvkp01DiagnosticsAvailable = false;
  ProtocolMatchStatus nvkp01Status = ProtocolMatchStatus::NO_MATCH;
  Nvkp01DecodeDiagnostics nvkp01;
  uint32_t nvkp01RejectCounts[kNvkp01RejectReasonCount] = {0};

  bool ht12eDiagnosticsAvailable = false;
  ProtocolMatchStatus ht12eStatus = ProtocolMatchStatus::NO_MATCH;
  Ht12eDecodeDiagnostics ht12e;
  uint32_t ht12eRejectCounts[kHt12eRejectReasonCount] = {0};

};

// Core 1 only: publishes one compact V2 diagnostics snapshot. Step 39.2 no
// longer carries legacy protocol comparison/action state.
void protocolDiagnosticsRecord(const RawCapture& capture,
                               const ProtocolEngineObservation& v2,
                               bool frameAccepted);

// Core 0/WebUI: atomically copies the latest compact diagnostics snapshot.
ProtocolDiagnosticsSnapshot protocolDiagnosticsGetSnapshot();

const char* protocolDiagnosticsV2ProtocolName(ProtocolId protocol);
const char* protocolDiagnosticsV2StatusName(ProtocolMatchStatus status);
