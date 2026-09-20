#include "protocols/ev1527_tx.h"

namespace {
const Ev1527TxEncoder kEncoder;
const Ev1527TxProfile kProfile;

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

bool appendPair(int16_t* pulses,
                uint16_t capacity,
                uint16_t& count,
                uint32_t highUs,
                uint32_t lowUs) {
  return appendPulse(pulses, capacity, count, true, highUs) &&
         appendPulse(pulses, capacity, count, false, lowUs);
}
}  // namespace

bool Ev1527TxEncoder::encode(const ProtocolTxRequest& request,
                            int16_t* pulses,
                            uint16_t capacity,
                            ProtocolTxPlan& plan) const {
  plan = ProtocolTxPlan{};

  if (request.protocol != ProtocolId::EV1527_PRINCETON ||
      request.symbolCount != kProfile.symbolCount ||
      request.pulseLengthUs < kProfile.minimumBasePulseUs ||
      request.pulseLengthUs > kProfile.maximumBasePulseUs ||
      request.requestedRepeats == 0U ||
      request.code > 0xFFFFFFULL ||
      pulses == nullptr) {
    return false;
  }

  // 24 complementary HIGH/LOW data pairs plus the final sync pair.
  static constexpr uint16_t kPulseCount = 24U * 2U + 2U;
  if (capacity < kPulseCount) return false;

  const uint32_t t = request.pulseLengthUs;
  uint16_t count = 0U;

  for (uint8_t bit = 0U; bit < kProfile.symbolCount; ++bit) {
    const uint8_t shift = static_cast<uint8_t>(kProfile.symbolCount - 1U - bit);
    const bool one = ((request.code >> shift) & 1ULL) != 0ULL;

    // EV1527/Princeton pulse-distance family, MSB first:
    //   0 = HIGH 1T, LOW 3T
    //   1 = HIGH 3T, LOW 1T
    const uint32_t highUs = one ? t * kProfile.longT : t;
    const uint32_t lowUs = one ? t : t * kProfile.longT;
    if (!appendPair(pulses, capacity, count, highUs, lowUs)) return false;
  }

  // Proven OpenRF sync model used by the existing EV1527 TX path.
  if (!appendPair(pulses, capacity, count, t,
                  t * static_cast<uint32_t>(kProfile.syncLowT))) {
    return false;
  }

  plan.pulseCount = count;
  plan.transmitRepeats = request.requestedRepeats;
  plan.protocolRepeats = request.requestedRepeats;
  return true;
}

const Ev1527TxEncoder& ev1527TxEncoder() { return kEncoder; }
const Ev1527TxProfile& ev1527TxProfile() { return kProfile; }
