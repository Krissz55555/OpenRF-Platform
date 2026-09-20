#pragma once

#include <Arduino.h>

enum class OpenRfProtocol : uint8_t {
  UNKNOWN = 0,
  EV1527_PRINCETON = 1,
  PT2262 = 2,
};


constexpr uint8_t kLegacyEv1527MaxSegments = 8;

enum class LegacyBinaryRejectReason : uint8_t {
  NONE = 0,
  PULSE_COUNT,
  POLARITY,
  CLASSIFICATION,
  SAME_LENGTH_PAIR,
  QUALITY,
};

struct LegacyEv1527SegmentDiagnostics {
  uint16_t rawPulseCount = 0;
  uint16_t trimmedPulseCount = 0;
  uint8_t decodedBits = 0;
  uint8_t quality = 0;
  bool accepted = false;
  LegacyBinaryRejectReason rejectReason = LegacyBinaryRejectReason::NONE;
};

struct LegacyEv1527Diagnostics {
  bool available = false;
  bool centersValid = false;
  uint16_t shortCenterUs = 0;
  uint16_t longCenterUs = 0;
  uint16_t centerRatioX100 = 0;
  uint32_t gapThresholdUs = 0;
  uint8_t segmentCount = 0;
  uint8_t binaryCandidateCount = 0;
  LegacyEv1527SegmentDiagnostics segments[kLegacyEv1527MaxSegments];
  bool wholeCaptureFallbackTried = false;
  LegacyEv1527SegmentDiagnostics wholeCapture;
  bool finalRecognized = false;
  uint8_t finalBits = 0;
  uint64_t finalCode = 0;
  uint8_t finalRepeats = 0;
  uint8_t finalQuality = 0;
};

struct ProtocolDecodeResult {
  bool valid = false;
  OpenRfProtocol protocol = OpenRfProtocol::UNKNOWN;
  uint8_t symbolCount = 0;       // bits for binary protocols, trits for PT2262
  uint64_t code = 0;
  uint16_t pulseLengthUs = 0;
  uint8_t repeats = 0;
  uint8_t quality = 0;
};

ProtocolDecodeResult protocolDecode(const int16_t* pulses, uint16_t count);
ProtocolDecodeResult protocolDecodeDetailed(const int16_t* pulses, uint16_t count, LegacyEv1527Diagnostics& diagnostics);
const char* legacyBinaryRejectReasonName(LegacyBinaryRejectReason reason);
const char* protocolName(OpenRfProtocol protocol);
