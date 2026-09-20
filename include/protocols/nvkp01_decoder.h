#pragma once

#include <stdint.h>
#include <stddef.h>

#include "protocol_decoder_v2.h"

// Step 40 FIX2: reject-first NVKP01 structure; sample-derived, not a full spec.
// NVKP01 is intentionally structurally different from the 24-bit/tri-state
// classic decoders and is used to prove registry/module extensibility.
enum class Nvkp01RejectReason : uint8_t {
  NONE = 0,
  CAPTURE_ENVELOPE,
  POLARITY_SEQUENCE,
  MARKER_STRUCTURE,
};

static constexpr size_t kNvkp01RejectReasonCount = 4U;

struct Nvkp01DecodeDiagnostics final {
  Nvkp01RejectReason rejectReason = Nvkp01RejectReason::NONE;
  uint16_t pulseCount = 0;
  uint32_t durationUs = 0;
  uint8_t markerPairs = 0;  // Qualified M pairs, including their ratio.
  uint8_t syncPulses = 0;  // Qualified LOW/HIGH S pairs, not isolated LOWs.
  bool fullLeader = false;
  bool alternating = false;  // Original FULL RAW, before local merge.
  uint16_t canonicalPulseCount = 0;
  uint16_t mergedPulses = 0;
  bool syncStructure = false;
  bool compactStructure = false;
  bool codeAvailable = false;
  uint64_t normalizedCode = 0;
  uint8_t repeatCount = 0;
};

struct Nvkp01DecoderLimits final {
  uint16_t minCapturePulses = 18;
  uint16_t maxCapturePulses = 70;
  uint32_t minDurationUs = 18000;
  uint32_t maxDurationUs = 60000;
  uint16_t markerLowMinUs = 1400;
  uint16_t markerLowMaxUs = 1750;
  uint16_t markerHighMinUs = 430;
  uint16_t markerHighMaxUs = 730;
  uint16_t syncLowMinUs = 5400;
  uint16_t syncLowMaxUs = 6000;
  uint16_t syncHighMinUs = 600;
  uint16_t syncHighMaxUs = 750;
  uint16_t shortMinUs = 140;
  uint16_t shortMaxUs = 280;
  uint16_t longMinUs = 580;
  uint16_t longMaxUs = 720;
  // Provisional locality windows in pulse indices, not proven frame lengths.
  uint8_t markerMaxOffset = 18;
  uint8_t shortLongMaxOffset = 32;
};

class Nvkp01V2Decoder final : public ProtocolDecoder {
 public:
  ProtocolId protocolId() const override {
    return ProtocolId::NVKP01_KINETIC;
  }

  ProtocolMatchResult decode(const RawCapture& capture) const override;
  ProtocolMatchResult decodeDetailed(const RawCapture& capture,
                                     Nvkp01DecodeDiagnostics& diagnostics) const;
};

const Nvkp01V2Decoder& nvkp01V2Decoder();
const Nvkp01DecoderLimits& nvkp01DecoderLimits();
const char* nvkp01RejectReasonName(Nvkp01RejectReason reason);
