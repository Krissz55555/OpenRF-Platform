#include <Arduino.h>
#include "universal_decoder.h"

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
  const ProtocolDecodeResult decoded = protocolDecode(pulses, count);
  DecodedRFEvent event;
  if (!decoded.valid) return event;

  event.valid = true;
  event.protocolId = decoded.protocol;
  event.protocol = protocolName(decoded.protocol);
  event.symbolCount = decoded.symbolCount;
  event.pulseLengthUs = decoded.pulseLengthUs;
  event.repeats = decoded.repeats;
  event.quality = decoded.quality;
  event.numericCode = decoded.code;
  event.code = hex64(decoded.code);
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
