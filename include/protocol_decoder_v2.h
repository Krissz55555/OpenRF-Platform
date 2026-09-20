#pragma once

#include <stdint.h>

#include "raw_capture.h"

// Stable V2 protocol identities. Values are append-only once published.
enum class ProtocolId : uint16_t {
  UNKNOWN = 0,
  EV1527_PRINCETON = 1,
  PT2262_TRI_STATE = 2,
  NVKP01_KINETIC = 3,
  HT12E = 4,
};

// Conservative classifier outcome. A decoder must be free to reject any
// capture; UNKNOWN resolution remains an engine-level concern for a later step.
enum class ProtocolMatchStatus : uint8_t {
  NO_MATCH = 0,
  MATCH = 1,
};

struct ProtocolMatchResult final {
  ProtocolMatchStatus status = ProtocolMatchStatus::NO_MATCH;

  constexpr bool matched() const {
    return status == ProtocolMatchStatus::MATCH;
  }
};

// Common non-owning V2 decoder contract.
//
// Implementations are deterministic classifier/parsers. They receive the same
// untouched FULL RawCapture, must not modify or retain it, and must not perform
// allocation, blocking, publishing, radio control, Analyzer/RX Slot access,
// Core 0 work, or TX. Decoder instances are expected to be statically allocated
// and outlive their registry entry.
class ProtocolDecoder {
 public:
  virtual ~ProtocolDecoder() = default;

  virtual ProtocolId protocolId() const = 0;
  virtual ProtocolMatchResult decode(const RawCapture& capture) const = 0;
};
