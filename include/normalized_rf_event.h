#pragma once

#include <stdint.h>

#include "protocol_decoder_v2.h"

// Compact protocol-independent representation of one confidently KNOWN V2
// decode. The object itself has no side effects. Since Step 31/32 the routing
// layer may promote approved events to the real RX Slot/MQTT/HA action path.
struct NormalizedRfEvent final {
  bool available = false;
  ProtocolId protocol = ProtocolId::UNKNOWN;
  uint64_t code = 0;
  uint8_t symbolCount = 0;  // bits for binary protocols, trits for tri-state.
  uint8_t repeats = 0;

  uint8_t radioId = 0;
  uint16_t rawPulseCount = 0;
  uint32_t rawDurationUs = 0;
  uint32_t capturedAtMs = 0;
  float frequencyMHz = 0.0F;
  float rssiDbm = -127.0F;
};
