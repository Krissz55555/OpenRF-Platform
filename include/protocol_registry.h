#pragma once

#include <stddef.h>

#include "protocol_decoder_v2.h"

// Fixed-capacity registry owned by the V2 Protocol Engine.
//
// The registry never owns decoder objects and performs no allocation. Future
// decoder instances must outlive their registration, which is expected to
// happen explicitly during startup before RF capture processing begins.
class ProtocolRegistry final {
 public:
  static constexpr size_t kCapacity = 16;

  constexpr ProtocolRegistry() : decoders_{}, count_(0) {}

  bool add(const ProtocolDecoder* decoder);
  size_t count() const;
  const ProtocolDecoder* at(size_t index) const;

 private:
  const ProtocolDecoder* decoders_[kCapacity];
  size_t count_;
};
