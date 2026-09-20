#include "protocols/ht12e_tx.h"

namespace {
const Ht12eTxEncoder kEncoder;
const Ht12eTxProfile kProfile;

bool appendPulse(int16_t* pulses,
                 uint16_t capacity,
                 uint16_t& count,
                 bool high,
                 uint32_t widthUs) {
  if (pulses == nullptr || count >= capacity || widthUs == 0U ||
      widthUs > 32767U) {
    return false;
  }
  pulses[count++] = high ? static_cast<int16_t>(widthUs)
                         : -static_cast<int16_t>(widthUs);
  return true;
}
}  // namespace

bool Ht12eTxEncoder::encode(const ProtocolTxRequest& request,
                            int16_t* pulses,
                            uint16_t capacity,
                            ProtocolTxPlan& plan) const {
  plan = ProtocolTxPlan{};
  if (request.protocol != ProtocolId::HT12E ||
      request.symbolCount != kProfile.symbolCount ||
      request.code > 0x0FFFULL || pulses == nullptr) {
    return false;
  }

  // One physical HT12E word is encoded here. The existing tuned RAW TX path
  // replays the frame five times, matching the physically verified reference
  // transmitter while keeping RX-measured jitter out of TX timing.
  static constexpr uint16_t kPulsesPerWord = 26U;
  if (capacity < kPulsesPerWord) return false;

  const uint32_t t = kProfile.basePulseUs;
  uint16_t count = 0U;

  // Demodulated HT12E word model used by the Step 30 decoder:
  // pilot LOW (~36T), sync HIGH (~1T), then 12 LOW/HIGH symbol pairs.
  if (!appendPulse(pulses, capacity, count, false,
                   t * static_cast<uint32_t>(kProfile.pilotT)) ||
      !appendPulse(pulses, capacity, count, true, t)) {
    return false;
  }

  for (uint8_t bit = 0; bit < kProfile.symbolCount; ++bit) {
    const uint8_t shift = static_cast<uint8_t>(kProfile.symbolCount - 1U - bit);
    const bool one = ((request.code >> shift) & 1ULL) != 0ULL;
    const uint32_t lowUs = one ? t * 2U : t;
    const uint32_t highUs = one ? t : t * 2U;
    if (!appendPulse(pulses, capacity, count, false, lowUs) ||
        !appendPulse(pulses, capacity, count, true, highUs)) {
      return false;
    }
  }

  plan.pulseCount = count;
  plan.transmitRepeats = kProfile.wordRepeats;
  plan.protocolRepeats = kProfile.wordRepeats;
  return true;
}

const Ht12eTxEncoder& ht12eTxEncoder() { return kEncoder; }
const Ht12eTxProfile& ht12eTxProfile() { return kProfile; }
