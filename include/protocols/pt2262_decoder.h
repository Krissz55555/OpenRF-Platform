#pragma once

#include <stddef.h>
#include <stdint.h>

#include "protocol_decoder_v2.h"

enum class Pt2262RejectReason : uint8_t {
  NONE = 0,
  PULSE_COUNT,
  POLARITY_SEQUENCE,
  CENTER_ESTIMATION,
  SYNC_TIMING,
  INVALID_TRISTATE_SYMBOL,
  INSUFFICIENT_REPEATS,
  REPEAT_CODE_MISMATCH,
  REPEAT_TIMING_MISMATCH,
  COUNT,
};

struct Pt2262DecoderLimits final {
  uint32_t minimumBasePulseUs = 180;
  uint32_t maximumBasePulseUs = 700;
  uint8_t classificationTolerancePercent = 38;
  uint16_t minimumLongShortRatioX100 = 200;
  uint16_t maximumLongShortRatioX100 = 450;
  uint16_t minimumSyncLowTX100 = 800;
  uint16_t maximumSyncLowTX100 = 6000;
  uint8_t maximumRepeatBaseDeviationPercent = 20;
  uint8_t expectedTritsPerFrame = 12;
};

struct Pt2262DecodeDiagnostics final {
  bool available = false;
  Pt2262RejectReason rejectReason = Pt2262RejectReason::NONE;
  uint16_t totalPulseCount = 0;
  uint8_t candidateFrameCount = 0;
  uint8_t validFrameCount = 0;
  int8_t firstValidFrame = -1;
  int8_t firstFailingFrame = -1;

  bool codeAvailable = false;
  uint64_t decodedCode = 0;
  uint32_t estimatedBasePulseUs = 0;
  uint8_t decodedTritCount = 0;
  uint8_t zeroTritCount = 0;
  uint8_t oneTritCount = 0;
  uint8_t floatingTritCount = 0;

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
};

// OpenRF-owned conservative PT2262/SC2262-style tri-state classifier.
//
// The implementation follows established protocol-recognition principles only:
// estimate short/long pulse families, segment repeated frames at a local long
// gap, decode each trit from four alternating pulses (0 / 1 / floating), and
// require repeated agreement. It does not depend on or call the legacy decoder.
class Pt2262Decoder final : public ProtocolDecoder {
 public:
  ProtocolId protocolId() const override;
  ProtocolMatchResult decode(const RawCapture& capture) const override;
  ProtocolMatchResult decodeDetailed(
      const RawCapture& capture, Pt2262DecodeDiagnostics& diagnostics) const;
};

const Pt2262Decoder& pt2262Decoder();
const Pt2262DecoderLimits& pt2262DecoderLimits();
const char* pt2262RejectReasonName(Pt2262RejectReason reason);
constexpr size_t kPt2262RejectReasonCount =
    static_cast<size_t>(Pt2262RejectReason::COUNT);
