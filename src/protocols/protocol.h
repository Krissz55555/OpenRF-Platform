#pragma once

#include <Arduino.h>

// Common metadata returned by every future OpenRF protocol decoder.
// Protocol-specific decoders must not publish MQTT messages or access
// Home Assistant directly. Their only responsibility is translating a RAW
// pulse frame into a typed result.
struct OpenRfDecodeMetadata {
  const char* protocolName = "Unknown";
  uint8_t quality = 0;       // 0..100
  uint8_t repeats = 0;
  uint16_t pulseCount = 0;
};

// Minimal common decoder interface. A concrete decoder owns the type of its
// output object, while this base interface provides discovery and validation.
class OpenRfProtocolDecoder {
 public:
  virtual ~OpenRfProtocolDecoder() = default;

  virtual const char* name() const = 0;

  // Returns true only when the supplied frame structurally matches the
  // protocol. It must not modify global application state.
  virtual bool matches(const int16_t* pulses, uint16_t pulseCount) const = 0;
};
