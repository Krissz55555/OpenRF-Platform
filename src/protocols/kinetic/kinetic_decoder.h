#pragma once

#include "../protocol.h"
#include "kinetic_event.h"

// Shared interface for every event-oriented RF decoder.
class KineticProtocolDecoder : public OpenRfProtocolDecoder {
 public:
  virtual bool decode(const int16_t* pulses,
                      uint16_t pulseCount,
                      KineticEvent& event) const = 0;
};
