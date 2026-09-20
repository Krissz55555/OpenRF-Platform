#include "protocol_registry.h"

bool ProtocolRegistry::add(const ProtocolDecoder* decoder) {
  if (decoder == nullptr || count_ >= kCapacity) return false;

  decoders_[count_] = decoder;
  ++count_;
  return true;
}

size_t ProtocolRegistry::count() const { return count_; }

const ProtocolDecoder* ProtocolRegistry::at(size_t index) const {
  return index < count_ ? decoders_[index] : nullptr;
}
