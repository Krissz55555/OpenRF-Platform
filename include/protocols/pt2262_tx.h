#pragma once

#include <stdint.h>

#include "protocol_tx.h"

// Step 37 V2-native PT2262/Tri-State TX profile.
// The code value is the same base-3 packed representation produced by the
// V2 decoder: twelve trits, most-significant trit first.
struct Pt2262TxProfile final {
  uint8_t symbolCount = 12;
  uint8_t longT = 3;
  uint8_t syncLowT = 31;
  uint16_t minimumBasePulseUs = 180;
  uint16_t maximumBasePulseUs = 700;
  uint64_t maximumCode = 531440ULL;  // 3^12 - 1
};

class Pt2262TxEncoder final : public ProtocolTxEncoder {
 public:
  ProtocolId protocolId() const override {
    return ProtocolId::PT2262_TRI_STATE;
  }

  bool encode(const ProtocolTxRequest& request,
              int16_t* pulses,
              uint16_t capacity,
              ProtocolTxPlan& plan) const override;
};

const Pt2262TxEncoder& pt2262TxEncoder();
const Pt2262TxProfile& pt2262TxProfile();
