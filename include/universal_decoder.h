#pragma once
#include <Arduino.h>
#include "protocol_decoder.h"

struct DecodedRFEvent {
  // valid=true means the frame is fully decoded and safe for RX Slots/MQTT.
  bool valid = false;

  // recognized=true may also describe a structural protocol match whose
  // payload/event decoding is still under development.
  bool recognized = false;
  bool kinetic = false;

  OpenRfProtocol protocolId = OpenRfProtocol::UNKNOWN;
  String protocol;
  String encoding;
  String deviceId;
  String command;
  String code;
  uint8_t symbolCount = 0;
  uint16_t pulseLengthUs = 0;
  uint8_t repeats = 0;
  uint8_t quality = 0;
  uint64_t numericCode = 0;
};

DecodedRFEvent universalDecode(const int16_t* pulses, uint16_t count);
bool decodedEventMatches(const DecodedRFEvent& event, const String& protocol,
                         const String& deviceId, const String& command,
                         const String& code, bool matchCode);
