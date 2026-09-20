#include "protocols/pt2262_tx.h"

namespace {
const Pt2262TxEncoder kEncoder;
const Pt2262TxProfile kProfile;

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

bool Pt2262TxEncoder::encode(const ProtocolTxRequest& request,
                            int16_t* pulses,
                            uint16_t capacity,
                            ProtocolTxPlan& plan) const {
  plan = ProtocolTxPlan{};

  if (request.protocol != ProtocolId::PT2262_TRI_STATE ||
      request.symbolCount != kProfile.symbolCount ||
      request.pulseLengthUs < kProfile.minimumBasePulseUs ||
      request.pulseLengthUs > kProfile.maximumBasePulseUs ||
      request.requestedRepeats == 0U || request.code > kProfile.maximumCode ||
      pulses == nullptr) {
    return false;
  }

  // 12 tri-state symbols * 4 pulses plus the final sync pair.
  static constexpr uint16_t kPulseCount = 12U * 4U + 2U;
  if (capacity < kPulseCount) return false;

  uint8_t trits[12] = {};
  uint64_t value = request.code;
  for (int8_t index = static_cast<int8_t>(kProfile.symbolCount) - 1;
       index >= 0; --index) {
    trits[index] = static_cast<uint8_t>(value % 3ULL);
    value /= 3ULL;
  }
  if (value != 0ULL) return false;

  const uint32_t t = request.pulseLengthUs;
  const uint32_t longUs = t * static_cast<uint32_t>(kProfile.longT);
  uint16_t count = 0U;

  for (uint8_t index = 0U; index < kProfile.symbolCount; ++index) {
    switch (trits[index]) {
      case 0U:
        // 0 = S,L,S,L
        if (!appendPair(pulses, capacity, count, t, longUs) ||
            !appendPair(pulses, capacity, count, t, longUs)) {
          return false;
        }
        break;
      case 1U:
        // 1 = L,S,L,S
        if (!appendPair(pulses, capacity, count, longUs, t) ||
            !appendPair(pulses, capacity, count, longUs, t)) {
          return false;
        }
        break;
      case 2U:
        // F = S,L,L,S
        if (!appendPair(pulses, capacity, count, t, longUs) ||
            !appendPair(pulses, capacity, count, longUs, t)) {
          return false;
        }
        break;
      default:
        return false;
    }
  }

  if (!appendPair(pulses, capacity, count, t,
                  t * static_cast<uint32_t>(kProfile.syncLowT))) {
    return false;
  }

  plan.pulseCount = count;
  plan.transmitRepeats = request.requestedRepeats;
  plan.protocolRepeats = request.requestedRepeats;
  return true;
}

const Pt2262TxEncoder& pt2262TxEncoder() { return kEncoder; }
const Pt2262TxProfile& pt2262TxProfile() { return kProfile; }
