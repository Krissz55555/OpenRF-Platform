#include "protocol_tx_diagnostics.h"

namespace {
ProtocolTxDiagnostics gDiagnostics;
}

void protocolTxDiagnosticsRecord(ProtocolId protocol,
                                 uint64_t code,
                                 uint8_t radioId,
                                 float frequencyMHz,
                                 bool success,
                                 uint8_t repeatCount,
                                 ProtocolTxFailureReason failureReason,
                                 uint32_t timestampMs) {
  ProtocolTxDiagnostics next;
  next.available = true;
  next.protocol = protocol;
  next.code = code;
  next.radioId = radioId;
  next.frequencyMHz = frequencyMHz;
  next.success = success;
  next.repeatCount = repeatCount;
  next.timestampMs = timestampMs;
  next.failureReason = failureReason;
  gDiagnostics = next;
}

ProtocolTxDiagnostics protocolTxDiagnosticsSnapshot() { return gDiagnostics; }

const char* protocolTxFailureReasonName(ProtocolTxFailureReason reason) {
  switch (reason) {
    case ProtocolTxFailureReason::INVALID_SLOT: return "INVALID_SLOT";
    case ProtocolTxFailureReason::UNSUPPORTED_PROTOCOL: return "UNSUPPORTED_PROTOCOL";
    case ProtocolTxFailureReason::INVALID_CODE: return "INVALID_CODE";
    case ProtocolTxFailureReason::ENCODE_FAILED: return "ENCODE_FAILED";
    case ProtocolTxFailureReason::RF_TX_FAILED: return "RF_TX_FAILED";
    default: return "NONE";
  }
}
