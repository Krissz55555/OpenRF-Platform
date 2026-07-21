#include <Arduino.h>
#include "protocol_decoder.h"

namespace {
constexpr uint16_t MIN_PULSE_US = 140;
constexpr uint16_t MAX_DATA_PULSE_US = 5000;
constexpr uint8_t MAX_CANDIDATES = 16;
constexpr uint8_t MIN_BINARY_BITS = 8;
constexpr uint8_t MAX_BINARY_BITS = 64;
constexpr uint8_t MIN_TRITS = 4;
constexpr uint8_t MAX_TRITS = 32;

struct Centers { float shortUs = 0; float longUs = 0; bool valid = false; };
struct Candidate {
  OpenRfProtocol protocol = OpenRfProtocol::UNKNOWN;
  uint8_t symbols = 0;
  uint64_t code = 0;
  uint8_t quality = 0;
};

uint32_t width(const int16_t p) {
  return static_cast<uint32_t>(abs(static_cast<int32_t>(p)));
}

Centers estimateCenters(const int16_t* p, uint16_t count) {
  uint32_t minV = UINT32_MAX, maxV = 0;
  uint16_t usable = 0;
  for (uint16_t i = 0; i < count; i++) {
    const uint32_t v = width(p[i]);
    if (v < MIN_PULSE_US || v > MAX_DATA_PULSE_US) continue;
    minV = min(minV, v); maxV = max(maxV, v); usable++;
  }
  if (usable < 12 || minV == UINT32_MAX || maxV < minV * 18UL / 10UL) return {};
  float a = static_cast<float>(minV), b = static_cast<float>(maxV);
  for (uint8_t pass = 0; pass < 8; pass++) {
    float sumA = 0, sumB = 0; uint16_t nA = 0, nB = 0;
    for (uint16_t i = 0; i < count; i++) {
      const uint32_t v = width(p[i]);
      if (v < MIN_PULSE_US || v > MAX_DATA_PULSE_US) continue;
      if (fabsf(v - a) <= fabsf(v - b)) { sumA += v; nA++; }
      else { sumB += v; nB++; }
    }
    if (!nA || !nB) return {};
    a = sumA / nA; b = sumB / nB;
  }
  if (a > b) { const float t = a; a = b; b = t; }
  const float ratio = b / a;
  if (ratio < 1.7F || ratio > 5.5F) return {};
  Centers c; c.shortUs = a; c.longUs = b; c.valid = true; return c;
}


bool classifyPulse(uint32_t value, const Centers& c, bool& isLong, float& error) {
  const float es = fabsf(value - c.shortUs) / c.shortUs;
  const float el = fabsf(value - c.longUs) / c.longUs;
  if (es <= el && es <= 0.42F) { isLong = false; error = es; return true; }
  if (el < es && el <= 0.42F) { isLong = true; error = el; return true; }
  return false;
}

bool decodeBinarySegment(const int16_t* p, uint16_t start, uint16_t end,
                         const Centers& c, Candidate& out) {
  while (start < end && p[start] < 0) start++;
  const uint16_t n = end > start ? end - start : 0;
  if (n < MIN_BINARY_BITS * 2 || n > MAX_BINARY_BITS * 2 + 2) return false;
  const uint16_t pairCount = n / 2;
  if (pairCount < MIN_BINARY_BITS || pairCount > MAX_BINARY_BITS) return false;

  uint64_t code = 0; float totalError = 0; uint8_t bits = 0;
  for (uint16_t i = 0; i + 1 < n && bits < MAX_BINARY_BITS; i += 2) {
    if (!(p[start + i] > 0 && p[start + i + 1] < 0)) return false;
    bool aLong = false, bLong = false; float ea = 0, eb = 0;
    if (!classifyPulse(width(p[start+i]), c, aLong, ea) ||
        !classifyPulse(width(p[start+i+1]), c, bLong, eb) || aLong == bLong) return false;
    code = (code << 1) | (aLong && !bLong ? 1ULL : 0ULL);
    totalError += ea + eb; bits++;
  }
  if (bits < MIN_BINARY_BITS) return false;
  const float meanError = totalError / (bits * 2.0F);
  const int q = static_cast<int>(100.0F - meanError * 100.0F);
  if (q < 72) return false;
  out.protocol = OpenRfProtocol::EV1527_PRINCETON;
  out.symbols = bits; out.code = code; out.quality = static_cast<uint8_t>(constrain(q, 0, 100));
  return true;
}

bool decodeTriStateSegment(const int16_t* p, uint16_t start, uint16_t end,
                           const Centers& c, Candidate& out) {
  while (start < end && p[start] < 0) start++;
  const uint16_t n = end > start ? end - start : 0;
  if (n < MIN_TRITS * 4 || n > MAX_TRITS * 4 + 3) return false;
  const uint8_t trits = n / 4;
  if (trits < MIN_TRITS || trits > MAX_TRITS) return false;

  uint64_t code = 0; float totalError = 0;
  for (uint8_t t = 0; t < trits; t++) {
    const uint16_t i = start + t * 4;
    if (!(p[i] > 0 && p[i+1] < 0 && p[i+2] > 0 && p[i+3] < 0)) return false;
    bool l[4]; float e[4];
    for (uint8_t k = 0; k < 4; k++) if (!classifyPulse(width(p[i+k]), c, l[k], e[k])) return false;
    uint8_t symbol;
    if (!l[0] && l[1] && !l[2] && l[3]) symbol = 0;       // 0
    else if (l[0] && !l[1] && l[2] && !l[3]) symbol = 1;  // 1
    else if (!l[0] && l[1] && l[2] && !l[3]) symbol = 2;  // floating
    else return false;
    code = code * 3ULL + symbol;
    totalError += e[0] + e[1] + e[2] + e[3];
  }
  const float meanError = totalError / (trits * 4.0F);
  const int q = static_cast<int>(100.0F - meanError * 100.0F);
  if (q < 72) return false;
  out.protocol = OpenRfProtocol::PT2262;
  out.symbols = trits; out.code = code; out.quality = static_cast<uint8_t>(constrain(q, 0, 100));
  return true;
}

void addCandidate(Candidate* list, uint8_t& count, const Candidate& c) {
  if (count < MAX_CANDIDATES) list[count++] = c;
}
} // namespace

const char* protocolName(OpenRfProtocol protocol) {
  switch (protocol) {
    case OpenRfProtocol::EV1527_PRINCETON: return "EV1527/Princeton";
    case OpenRfProtocol::PT2262: return "PT2262";
    default: return "Unknown";
  }
}

ProtocolDecodeResult protocolDecode(const int16_t* pulses, uint16_t count) {
  ProtocolDecodeResult result;
  if (!pulses || count < 16) return result;
  const Centers centers = estimateCenters(pulses, count);
  if (!centers.valid) return result;

  const uint32_t gapThreshold = max(static_cast<uint32_t>(4500),
                                    static_cast<uint32_t>(centers.longUs * 4.5F));
  Candidate candidates[MAX_CANDIDATES]; uint8_t candidateCount = 0;
  uint16_t segmentStart = 0;
  for (uint16_t i = 0; i <= count; i++) {
    const bool boundary = i == count || width(pulses[i]) >= gapThreshold;
    if (!boundary) continue;
    if (i > segmentStart + 7) {
      Candidate c;
      if (decodeTriStateSegment(pulses, segmentStart, i, centers, c)) addCandidate(candidates, candidateCount, c);
      c = Candidate{};
      if (decodeBinarySegment(pulses, segmentStart, i, centers, c)) addCandidate(candidates, candidateCount, c);
    }
    segmentStart = i + 1;
  }
  // Some remotes are captured without an internal sync gap. Try the whole frame too.
  if (!candidateCount) {
    Candidate c;
    if (decodeTriStateSegment(pulses, 0, count, centers, c)) addCandidate(candidates, candidateCount, c);
    c = Candidate{};
    if (decodeBinarySegment(pulses, 0, count, centers, c)) addCandidate(candidates, candidateCount, c);
  }
  if (!candidateCount) return result;

  uint8_t bestIndex = 0, bestRepeats = 1;
  for (uint8_t i = 0; i < candidateCount; i++) {
    uint8_t repeats = 1;
    for (uint8_t j = i + 1; j < candidateCount; j++) {
      if (candidates[i].protocol == candidates[j].protocol &&
          candidates[i].symbols == candidates[j].symbols &&
          candidates[i].code == candidates[j].code) repeats++;
    }
    if (repeats > bestRepeats || (repeats == bestRepeats && candidates[i].quality > candidates[bestIndex].quality)) {
      bestRepeats = repeats; bestIndex = i;
    }
  }
  const Candidate& best = candidates[bestIndex];
  // A repeated exact code is preferred. A single pristine frame is accepted as fallback.
  if (bestRepeats < 2 && best.quality < 92) return result;
  result.valid = true;
  result.protocol = best.protocol;
  result.symbolCount = best.symbols;
  result.code = best.code;
  result.pulseLengthUs = static_cast<uint16_t>((centers.shortUs + 0.5F));
  result.repeats = bestRepeats;
  result.quality = best.quality;
  return result;
}
