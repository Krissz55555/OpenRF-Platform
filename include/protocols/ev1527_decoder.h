#pragma once

#include <stddef.h>
#include <stdint.h>

#include "protocol_decoder_v2.h"

enum class Ev1527RejectReason : uint8_t {
  NONE = 0,
  PULSE_COUNT,
  POLARITY_SEQUENCE,
  CENTER_ESTIMATION,
  BASE_T_OUT_OF_RANGE,
  SYNC_TIMING,
  SHORT_TIMING,
  LONG_TIMING,
  SHORT_LONG_RATIO,
  PAIR_TIMING,
  INVALID_SYMBOL,
  TRISTATE_LOOKALIKE,
  INSUFFICIENT_REPEATS,
  REPEAT_CODE_MISMATCH,
  REPEAT_TIMING_MISMATCH,
  ADDRESS_PATTERN,
  COMMAND_PATTERN,
  COUNT,
};

enum class Ev1527CheckState : uint8_t {
  NOT_AVAILABLE = 0,
  PASS,
  FAIL,
};

struct Ev1527DecoderLimits final {
  uint32_t minimumBasePulseUs = 180;
  uint32_t maximumBasePulseUs = 700;
  uint8_t shortTolerancePercent = 38;
  uint8_t longTolerancePercent = 38;
  uint8_t pairTolerancePercent = 0;
  uint8_t syncHighTolerancePercent = 38;
  uint16_t minimumLongShortRatioX100 = 200;
  uint16_t maximumLongShortRatioX100 = 450;
  uint16_t minimumSyncLowTX100 = 800;
  uint16_t maximumSyncLowTX100 = 6000;
  uint8_t maximumRepeatBaseDeviationPercent = 20;
  uint8_t expectedBitsPerFrame = 24;
};

struct Ev1527DecodeDiagnostics final {
  bool available = false;
  Ev1527RejectReason rejectReason = Ev1527RejectReason::NONE;

  uint16_t totalPulseCount = 0;
  uint8_t candidateFrameCount = 0;
  uint8_t validFrameCount = 0;
  int8_t firstValidFrame = -1;
  int8_t firstFailingFrame = -1;

  bool codeAvailable = false;
  uint32_t decodedCode = 0;
  uint32_t estimatedBasePulseUs = 0;

  bool pulseRangeAvailable = false;
  uint16_t observedShortMinUs = 0;
  uint16_t observedShortMaxUs = 0;
  uint16_t observedLongMinUs = 0;
  uint16_t observedLongMaxUs = 0;
  uint16_t observedRatioMinX100 = 0;
  uint16_t observedRatioMaxX100 = 0;

  bool syncLowAvailable = false;
  uint16_t observedSyncLowTX100 = 0;

  uint8_t repeatFrameCount = 0;
  uint8_t matchingRepeatCount = 0;
  uint16_t maximumRepeatBaseDeviationX10Percent = 0;

  Ev1527CheckState addressPattern = Ev1527CheckState::NOT_AVAILABLE;
  Ev1527CheckState commandPattern = Ev1527CheckState::NOT_AVAILABLE;
};

// Conservative EV1527/Princeton pulse-distance family classifier.
// Step 29.5/6 uses established RC-switch-family recognition principles
// (short/long pulse centers, repeat segmentation, complementary pairs, repeat
// agreement) but remains an independent OpenRF implementation.
//
// The decoder consumes the untouched full capture synchronously. It owns no
// capture data, allocates no memory, and exposes MATCH/NO_MATCH through the V2
// contract. decodeDetailed() adds Step 29.5/3 diagnostics without changing the
// generic ProtocolDecoder interface or actionable behavior.
class Ev1527Decoder final : public ProtocolDecoder {
 public:
  ProtocolId protocolId() const override;
  ProtocolMatchResult decode(const RawCapture& capture) const override;
  ProtocolMatchResult decodeDetailed(
      const RawCapture& capture, Ev1527DecodeDiagnostics& diagnostics) const;
};

const Ev1527Decoder& ev1527Decoder();
const Ev1527DecoderLimits& ev1527DecoderLimits();
const char* ev1527RejectReasonName(Ev1527RejectReason reason);
const char* ev1527CheckStateName(Ev1527CheckState state);
constexpr size_t kEv1527RejectReasonCount =
    static_cast<size_t>(Ev1527RejectReason::COUNT);
