#pragma once

#include "../kinetic_decoder.h"

class Nvkp01Decoder final : public KineticProtocolDecoder {
 public:
  const char* name() const override;
  bool matches(const int16_t* pulses, uint16_t pulseCount) const override;
  bool decode(const int16_t* pulses,
              uint16_t pulseCount,
              KineticEvent& event) const override;
};

// Stateless shared instance for the future decoder registry.
const Nvkp01Decoder& nvkp01Decoder();
