#pragma once

#include <stdint.h>

#include "protocol_tx.h"

// Step 36 V2-native EV1527/Princeton TX profile.
// The timing ratios preserve the proven OpenRF legacy waveform while moving
// ownership into the modular Known Protocol Library.
struct Ev1527TxProfile final {
  uint8_t symbolCount = 24;
  uint8_t longT = 3;
  uint8_t syncLowT = 31;
  uint16_t minimumBasePulseUs = 180;
  uint16_t maximumBasePulseUs = 700;
};

class Ev1527TxEncoder final : public ProtocolTxEncoder {
 public:
  ProtocolId protocolId() const override {
    return ProtocolId::EV1527_PRINCETON;
  }

  bool encode(const ProtocolTxRequest& request,
              int16_t* pulses,
              uint16_t capacity,
              ProtocolTxPlan& plan) const override;
};

const Ev1527TxEncoder& ev1527TxEncoder();
const Ev1527TxProfile& ev1527TxProfile();
