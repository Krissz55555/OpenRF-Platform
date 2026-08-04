#pragma once

#include <Arduino.h>
#include "../protocol.h"

// High-level event types exposed by Kinetic decoders. The list is deliberately
// protocol-independent so MQTT, RX Slots and Home Assistant can consume the
// same model regardless of the RF encoding used on air.
enum class KineticEventType : uint8_t {
  UNKNOWN = 0,
  PRESS,
  RELEASE,
  LONG_PRESS,
  DOUBLE_PRESS,
  HOLD,
  OPEN,
  CLOSE,
  MOTION,
  LOW_BATTERY,
};

struct KineticEvent {
  bool valid = false;
  KineticEventType type = KineticEventType::UNKNOWN;

  // Stable, decoder-generated identifiers. Fixed-size buffers avoid dynamic
  // allocation in the RF receive path on ESP8266.
  char deviceId[25] = {0};
  char controlId[17] = {0};

  uint8_t channel = 0;
  bool repeat = false;
  OpenRfDecodeMetadata metadata;
};

inline const char* kineticEventTypeName(const KineticEventType type) {
  switch (type) {
    case KineticEventType::PRESS: return "PRESS";
    case KineticEventType::RELEASE: return "RELEASE";
    case KineticEventType::LONG_PRESS: return "LONG_PRESS";
    case KineticEventType::DOUBLE_PRESS: return "DOUBLE_PRESS";
    case KineticEventType::HOLD: return "HOLD";
    case KineticEventType::OPEN: return "OPEN";
    case KineticEventType::CLOSE: return "CLOSE";
    case KineticEventType::MOTION: return "MOTION";
    case KineticEventType::LOW_BATTERY: return "LOW_BATTERY";
    default: return "UNKNOWN";
  }
}
