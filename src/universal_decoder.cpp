#include <Arduino.h>
#include "universal_decoder.h"
#include "protocols/protocol_manager.h"

namespace {
String hex64(uint64_t value, uint8_t minDigits = 1) {
  char buffer[19];
  const uint32_t high = static_cast<uint32_t>(value >> 32);
  const uint32_t low = static_cast<uint32_t>(value);
  if (high) snprintf(buffer, sizeof(buffer), "%08X%08X", high, low);
  else snprintf(buffer, sizeof(buffer), "%08X", low);
  String result(buffer);
  while (result.length() > minDigits && result[0] == '0') result.remove(0, 1);
  while (result.length() < minDigits) result = "0" + result;
  return result;
}
}

DecodedRFEvent universalDecode(const int16_t* pulses, uint16_t count) {
  const ProtocolManagerResult decoded = protocolManagerDecode(pulses, count);
  DecodedRFEvent event;
  if (!decoded.recognized) return event;

  event.recognized = true;
  event.kinetic = decoded.kinetic;
  event.protocol = decoded.protocolName;
  event.encoding = decoded.encodingName;

  if (!decoded.actionable) {
    // Recognition-only results are intentionally not valid. Analyzer may show
    // them, while RX learning, RX Slots, MQTT and Home Assistant ignore them.
    return event;
  }

  event.valid = true;

  if (decoded.kinetic) {
    const KineticEvent& kinetic = decoded.kineticEvent;
    event.deviceId = kinetic.deviceId;
    event.command = kinetic.controlId;
    event.code = kineticEventTypeName(kinetic.type);
    event.repeats = kinetic.metadata.repeats;
    event.quality = kinetic.metadata.quality;
    return event;
  }

  const ProtocolDecodeResult& classic = decoded.classic;
  event.protocolId = classic.protocol;
  event.symbolCount = classic.symbolCount;
  event.pulseLengthUs = classic.pulseLengthUs;
  event.repeats = classic.repeats;
  event.quality = classic.quality;
  event.numericCode = classic.code;
  event.code = hex64(classic.code);
  event.deviceId = event.code;
  return event;
}

bool decodedEventMatches(const DecodedRFEvent& event, const String& protocol,
                         const String& deviceId, const String& command,
                         const String& code, bool matchCode) {
  if (!event.valid || !event.protocol.equalsIgnoreCase(protocol)) return false;
  if (deviceId.length() && !event.deviceId.equalsIgnoreCase(deviceId)) return false;
  if (command.length() && !event.command.equalsIgnoreCase(command)) return false;
  if (matchCode && code.length() && !event.code.equalsIgnoreCase(code)) return false;
  return true;
}
