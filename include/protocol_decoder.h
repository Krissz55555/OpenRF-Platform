#pragma once

#include <Arduino.h>

enum class OpenRfProtocol : uint8_t {
  UNKNOWN = 0,
  EV1527_PRINCETON = 1,
  PT2262 = 2,
};

struct ProtocolDecodeResult {
  bool valid = false;
  OpenRfProtocol protocol = OpenRfProtocol::UNKNOWN;
  uint8_t symbolCount = 0;       // bits for binary protocols, trits for PT2262
  uint64_t code = 0;
  uint16_t pulseLengthUs = 0;
  uint8_t repeats = 0;
  uint8_t quality = 0;
};

ProtocolDecodeResult protocolDecode(const int16_t* pulses, uint16_t count);
const char* protocolName(OpenRfProtocol protocol);
