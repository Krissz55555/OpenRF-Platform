#pragma once

#include <stdint.h>

#include "protocol_tx.h"

struct Ht12eTxProfile final {
  uint16_t basePulseUs = 400;
  uint8_t pilotT = 36;
  uint8_t wordRepeats = 5;
  uint8_t symbolCount = 12;
};

class Ht12eTxEncoder final : public ProtocolTxEncoder {
 public:
  ProtocolId protocolId() const override { return ProtocolId::HT12E; }
  bool encode(const ProtocolTxRequest& request,
              int16_t* pulses,
              uint16_t capacity,
              ProtocolTxPlan& plan) const override;
};

const Ht12eTxEncoder& ht12eTxEncoder();
const Ht12eTxProfile& ht12eTxProfile();
