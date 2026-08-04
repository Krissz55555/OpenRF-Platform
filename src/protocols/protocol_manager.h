#pragma once

#include <Arduino.h>
#include "../../include/protocol_decoder.h"
#include "kinetic/kinetic_event.h"

// Unified result of the OpenRF decoder chain.
// "recognized" means a decoder structurally matched the frame.
// "actionable" means the match was decoded far enough to emit an event.
struct ProtocolManagerResult {
  bool recognized = false;
  bool actionable = false;
  bool kinetic = false;

  const char* protocolName = "Unknown";
  const char* encodingName = "Unknown";

  ProtocolDecodeResult classic;
  KineticEvent kineticEvent;
};

// Runs the registered decoder chain without publishing MQTT messages or
// modifying RX Slots, storage, WebUI or Home Assistant state.
ProtocolManagerResult protocolManagerDecode(const int16_t* pulses,
                                            uint16_t pulseCount);
