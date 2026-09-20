#include "protocol_diagnostics.h"

#include <Arduino.h>

#include "protocol_engine.h"

namespace {

portMUX_TYPE diagnosticsMux = portMUX_INITIALIZER_UNLOCKED;
ProtocolDiagnosticsSnapshot snapshot;
uint32_t sequence = 0;

}  // namespace

void protocolDiagnosticsRecord(const RawCapture& capture,
                               const ProtocolEngineObservation& v2,
                               bool frameAccepted) {
  ProtocolDiagnosticsSnapshot next;
  next.available = true;
  next.capturedAtMs = capture.capturedAtMs();
  next.radioId = capture.radioId();
  next.pulseCount = capture.pulseCount();
  next.durationUs = capture.durationUs();
  next.frequencyMHz = capture.frequencyMHz();
  next.rssiDbm = capture.rssiDbm();
  next.frameAccepted = frameAccepted;

  next.v2Evaluated = v2.evaluatedDecoders > 0U;
  next.v2Decision = v2.decision;
  next.v2CandidateCount = v2.candidateCount;
  for (uint8_t index = 0; index < v2.candidateCount && index < kProtocolEngineMaxCandidates; ++index) {
    next.v2Candidates[index] = v2.candidates[index];
  }
  next.v2SelectedProtocol = v2.selectedProtocol;
  next.v2Protocol = v2.primaryProtocol;
  next.v2Status = v2.primaryStatus;
  next.registeredDecoders = v2.registeredDecoders;
  next.evaluatedDecoders = v2.evaluatedDecoders;
  next.v2NormalizedEventAvailable = v2.normalizedEvent.available;
  next.v2Dedup = v2.dedup;
  next.v2ActionableDryRun = v2.actionableDryRun;
  if (v2.normalizedEvent.available) {
    next.v2NormalizedEvent = v2.normalizedEvent;
  }
  next.ev1527DiagnosticsAvailable = v2.ev1527DiagnosticsAvailable;
  if (v2.ev1527DiagnosticsAvailable) {
    next.ev1527 = v2.ev1527Diagnostics;
  }
  next.pt2262DiagnosticsAvailable = v2.pt2262DiagnosticsAvailable;
  next.pt2262Status = v2.pt2262Status;
  if (v2.pt2262DiagnosticsAvailable) {
    next.pt2262 = v2.pt2262Diagnostics;
  }
  next.nvkp01DiagnosticsAvailable = v2.nvkp01DiagnosticsAvailable;
  next.nvkp01Status = v2.nvkp01Status;
  if (v2.nvkp01DiagnosticsAvailable) {
    next.nvkp01 = v2.nvkp01Diagnostics;
  }
  next.ht12eDiagnosticsAvailable = v2.ht12eDiagnosticsAvailable;
  next.ht12eStatus = v2.ht12eStatus;
  if (v2.ht12eDiagnosticsAvailable) {
    next.ht12e = v2.ht12eDiagnostics;
  }

  portENTER_CRITICAL(&diagnosticsMux);
  next.sequence = ++sequence;
  next.v2MatchCount = snapshot.v2MatchCount + v2.matchCount;
  next.v2NoMatchCount = snapshot.v2NoMatchCount + v2.noMatchCount;
  next.v2UnknownDecisionCount = snapshot.v2UnknownDecisionCount;
  next.v2KnownDecisionCount = snapshot.v2KnownDecisionCount;
  next.v2AmbiguousDecisionCount = snapshot.v2AmbiguousDecisionCount;
  next.v2NormalizedEventCount = snapshot.v2NormalizedEventCount;

  // Step 35.1: preserve the last confident KNOWN event across later UNKNOWN,
  // rejected or noisy captures. The live same-capture snapshot above remains
  // unchanged; this is a separate latched diagnostic view.
  if (v2.normalizedEvent.available) {
    next.v2LastKnownEventAvailable = true;
    next.v2LastKnownEvent = v2.normalizedEvent;
  } else {
    next.v2LastKnownEventAvailable = snapshot.v2LastKnownEventAvailable;
    if (snapshot.v2LastKnownEventAvailable) {
      next.v2LastKnownEvent = snapshot.v2LastKnownEvent;
    }
  }

  next.v2LogicalEventCount = snapshot.v2LogicalEventCount;
  next.v2CollapsedEventCount = snapshot.v2CollapsedEventCount;
  next.v2ActionableWouldEmitCount = snapshot.v2ActionableWouldEmitCount;
  next.v2ActionableSuppressedDuplicateCount =
      snapshot.v2ActionableSuppressedDuplicateCount;
  if (v2.normalizedEvent.available) next.v2NormalizedEventCount++;
  if (v2.dedup.state == NormalizedEventDedupState::EMIT) {
    next.v2LogicalEventCount++;
  } else if (v2.dedup.state == NormalizedEventDedupState::COLLAPSED) {
    next.v2CollapsedEventCount++;
  }
  if (v2.actionableDryRun.state == V2ActionableDryRunState::WOULD_EMIT) {
    next.v2ActionableWouldEmitCount++;
  } else if (v2.actionableDryRun.state ==
             V2ActionableDryRunState::SUPPRESSED_DUPLICATE) {
    next.v2ActionableSuppressedDuplicateCount++;
  }
  if (v2.evaluatedDecoders > 0U) {
    switch (v2.decision) {
      case ProtocolEngineDecisionState::KNOWN:
        next.v2KnownDecisionCount++;
        break;
      case ProtocolEngineDecisionState::AMBIGUOUS:
        next.v2AmbiguousDecisionCount++;
        break;
      default:
        next.v2UnknownDecisionCount++;
        break;
    }
  }
  for (size_t index = 0; index < kEv1527RejectReasonCount; ++index) {
    next.ev1527RejectCounts[index] = snapshot.ev1527RejectCounts[index];
  }
  for (size_t index = 0; index < kPt2262RejectReasonCount; ++index) {
    next.pt2262RejectCounts[index] = snapshot.pt2262RejectCounts[index];
  }
  for (size_t index = 0; index < kNvkp01RejectReasonCount; ++index) {
    next.nvkp01RejectCounts[index] = snapshot.nvkp01RejectCounts[index];
  }
  for (size_t index = 0; index < kHt12eRejectReasonCount; ++index) {
    next.ht12eRejectCounts[index] = snapshot.ht12eRejectCounts[index];
  }
  if (v2.ev1527DiagnosticsAvailable &&
      v2.ev1527Diagnostics.rejectReason != Ev1527RejectReason::NONE) {
    const size_t reasonIndex =
        static_cast<size_t>(v2.ev1527Diagnostics.rejectReason);
    if (reasonIndex > 0U && reasonIndex < kEv1527RejectReasonCount) {
      next.ev1527RejectCounts[reasonIndex]++;
    }
  }
  if (v2.pt2262DiagnosticsAvailable &&
      v2.pt2262Diagnostics.rejectReason != Pt2262RejectReason::NONE) {
    const size_t reasonIndex =
        static_cast<size_t>(v2.pt2262Diagnostics.rejectReason);
    if (reasonIndex > 0U && reasonIndex < kPt2262RejectReasonCount) {
      next.pt2262RejectCounts[reasonIndex]++;
    }
  }
  if (v2.nvkp01DiagnosticsAvailable &&
      v2.nvkp01Diagnostics.rejectReason != Nvkp01RejectReason::NONE) {
    const size_t reasonIndex =
        static_cast<size_t>(v2.nvkp01Diagnostics.rejectReason);
    if (reasonIndex > 0U && reasonIndex < kNvkp01RejectReasonCount) {
      next.nvkp01RejectCounts[reasonIndex]++;
    }
  }
  if (v2.ht12eDiagnosticsAvailable &&
      v2.ht12eDiagnostics.rejectReason != Ht12eRejectReason::NONE) {
    const size_t reasonIndex = static_cast<size_t>(v2.ht12eDiagnostics.rejectReason);
    if (reasonIndex > 0U && reasonIndex < kHt12eRejectReasonCount) {
      next.ht12eRejectCounts[reasonIndex]++;
    }
  }
  snapshot = next;
  portEXIT_CRITICAL(&diagnosticsMux);
}

ProtocolDiagnosticsSnapshot protocolDiagnosticsGetSnapshot() {
  ProtocolDiagnosticsSnapshot copy;
  portENTER_CRITICAL(&diagnosticsMux);
  copy = snapshot;
  portEXIT_CRITICAL(&diagnosticsMux);
  return copy;
}

const char* protocolDiagnosticsV2ProtocolName(ProtocolId protocol) {
  switch (protocol) {
    case ProtocolId::EV1527_PRINCETON:
      return "EV1527/Princeton";
    case ProtocolId::PT2262_TRI_STATE:
      return "PT2262/Tri-State";
    case ProtocolId::NVKP01_KINETIC:
      return "NVKP01 Kinetic";
    case ProtocolId::HT12E:
      return "HT12E";
    default:
      return "Unknown";
  }
}

const char* protocolDiagnosticsV2StatusName(ProtocolMatchStatus status) {
  return status == ProtocolMatchStatus::MATCH ? "MATCH" : "NO_MATCH";
}
