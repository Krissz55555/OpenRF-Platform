#include "protocol_manager.h"

#include "kinetic/nvkp01/decoder.h"

namespace {

ProtocolManagerResult decodeClassic(const int16_t* pulses,
                                    const uint16_t pulseCount) {
  ProtocolManagerResult result;
  result.classic = protocolDecode(pulses, pulseCount);
  if (!result.classic.valid) return result;

  result.recognized = true;
  result.actionable = true;
  result.kinetic = false;
  result.protocolName = protocolName(result.classic.protocol);
  result.encodingName = result.classic.protocol == OpenRfProtocol::PT2262
                            ? "Tri-state PWM"
                            : "OOK PWM";
  return result;
}

ProtocolManagerResult decodeKinetic(const int16_t* pulses,
                                    const uint16_t pulseCount) {
  ProtocolManagerResult result;

  // Registry order matters: more specific decoders should be listed before
  // broad/fallback decoders. NVKP01 currently normalizes its one physical
  // control to a Kinetic PRESS event.
  const KineticProtocolDecoder* const decoders[] = {
      &nvkp01Decoder(),
  };

  for (const KineticProtocolDecoder* decoder : decoders) {
    if (!decoder || !decoder->matches(pulses, pulseCount)) continue;

    result.recognized = true;
    result.kinetic = true;
    result.protocolName = decoder->name();
    result.encodingName = "Kinetic marker train";

    KineticEvent event;
    if (decoder->decode(pulses, pulseCount, event) && event.valid) {
      result.actionable = true;
      result.kineticEvent = event;
    }
    return result;
  }

  return result;
}

}  // namespace

ProtocolManagerResult protocolManagerDecode(const int16_t* pulses,
                                            const uint16_t pulseCount) {
  ProtocolManagerResult result;
  if (!pulses || !pulseCount) return result;

  // Keep the existing stable classic decoders authoritative. Kinetic
  // recognition is attempted only when no classic decoder produced a valid
  // result, so EV1527/PT2262 behaviour remains unchanged.
  result = decodeClassic(pulses, pulseCount);
  if (result.recognized) return result;

  return decodeKinetic(pulses, pulseCount);
}
