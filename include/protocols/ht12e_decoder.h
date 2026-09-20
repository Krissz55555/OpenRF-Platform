#pragma once

#include <stddef.h>
#include <stdint.h>

#include "protocol_decoder_v2.h"

// Step 30: HT12E module for the modular Known Protocol Library.
// Recognition is based on the public HT12E waveform model: a long pilot LOW,
// a short sync HIGH, then twelve binary address/data symbols. Each data symbol
// is represented by complementary LOW/HIGH durations near 1T:2T or 2T:1T.
enum class Ht12eRejectReason : uint8_t {
  NONE = 0,
  CAPTURE_ENVELOPE,
  POLARITY_SEQUENCE,
  PILOT_SYNC,
  SYMBOL_TIMING,
  REPEAT_MISMATCH,
};

static constexpr size_t kHt12eRejectReasonCount = 6U;

struct Ht12eDecodeDiagnostics final {
  Ht12eRejectReason rejectReason = Ht12eRejectReason::NONE;
  uint16_t pulseCount = 0;
  uint32_t durationUs = 0;
  uint8_t candidateWords = 0;
  uint8_t validWords = 0;
  uint8_t matchingWords = 0;
  uint16_t estimatedTUs = 0;
  uint16_t pilotMinUs = 0;
  uint16_t pilotMaxUs = 0;
  uint16_t shortMinUs = 0;
  uint16_t shortMaxUs = 0;
  uint16_t longMinUs = 0;
  uint16_t longMaxUs = 0;
  bool alternating = false;
  bool codeAvailable = false;
  uint16_t decodedWord = 0;
  uint8_t address = 0;
  uint8_t data = 0;
  uint8_t repeatCount = 0;
};

struct Ht12eDecoderLimits final {
  uint8_t expectedBits = 12;
  uint8_t addressBits = 8;
  uint8_t dataBits = 4;
  uint8_t minMatchingWords = 2;
  uint16_t minTUs = 140;
  uint16_t maxTUs = 900;
  uint8_t pilotMinT = 24;
  uint8_t pilotMaxT = 48;
  uint16_t shortMinPct = 55;
  uint16_t shortMaxPct = 150;
  uint16_t longMinPct = 150;
  uint16_t longMaxPct = 275;
  uint16_t pairMinPct = 235;
  uint16_t pairMaxPct = 365;
};

class Ht12eDecoder final : public ProtocolDecoder {
 public:
  ProtocolId protocolId() const override { return ProtocolId::HT12E; }
  ProtocolMatchResult decode(const RawCapture& capture) const override;
  ProtocolMatchResult decodeDetailed(const RawCapture& capture,
                                     Ht12eDecodeDiagnostics& diagnostics) const;
};

const Ht12eDecoder& ht12eDecoder();
const Ht12eDecoderLimits& ht12eDecoderLimits();
const char* ht12eRejectReasonName(Ht12eRejectReason reason);
