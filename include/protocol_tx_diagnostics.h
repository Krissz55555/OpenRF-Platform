#pragma once

#include <stdint.h>

#include "protocol_decoder_v2.h"

enum class ProtocolTxFailureReason : uint8_t {
  NONE = 0,
  INVALID_SLOT,
  UNSUPPORTED_PROTOCOL,
  INVALID_CODE,
  ENCODE_FAILED,
  RF_TX_FAILED,
};

struct ProtocolTxDiagnostics final {
  bool available = false;
  ProtocolId protocol = ProtocolId::UNKNOWN;
  uint64_t code = 0;
  uint8_t radioId = 0;
  float frequencyMHz = 0.0F;
  bool success = false;
  uint8_t repeatCount = 0;
  uint32_t timestampMs = 0;
  ProtocolTxFailureReason failureReason = ProtocolTxFailureReason::NONE;
};

void protocolTxDiagnosticsRecord(ProtocolId protocol,
                                 uint64_t code,
                                 uint8_t radioId,
                                 float frequencyMHz,
                                 bool success,
                                 uint8_t repeatCount,
                                 ProtocolTxFailureReason failureReason,
                                 uint32_t timestampMs);
ProtocolTxDiagnostics protocolTxDiagnosticsSnapshot();
const char* protocolTxFailureReasonName(ProtocolTxFailureReason reason);
