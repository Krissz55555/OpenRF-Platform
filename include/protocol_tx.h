#pragma once

#include <stdint.h>

#include "protocol_decoder_v2.h"

// Step 34: optional, protocol-specific TX contract.
// RX-only protocol modules do not need to implement this interface.
struct ProtocolTxRequest final {
  ProtocolId protocol = ProtocolId::UNKNOWN;
  uint64_t code = 0;
  uint8_t symbolCount = 0;
  // Optional protocol timing learned from RX. Fixed-timing encoders may ignore it.
  uint16_t pulseLengthUs = 0;
  uint8_t requestedRepeats = 1;
};

struct ProtocolTxPlan final {
  uint16_t pulseCount = 0;
  uint8_t transmitRepeats = 0;
  uint8_t protocolRepeats = 0;
};

class ProtocolTxEncoder {
 public:
  virtual ~ProtocolTxEncoder() = default;
  virtual ProtocolId protocolId() const = 0;

  // Writes one protocol frame into the caller-owned signed pulse buffer.
  // transmitRepeats tells the existing radio TX path how many times to replay
  // that frame. No allocation, radio access, or UI work is allowed here.
  virtual bool encode(const ProtocolTxRequest& request,
                      int16_t* pulses,
                      uint16_t capacity,
                      ProtocolTxPlan& plan) const = 0;
};
