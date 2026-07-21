#include <Arduino.h>
#include "analyzer.h"
#include "universal_decoder.h"

namespace {
AnalyzerSnapshot snapshot;

uint32_t pulseWidth(int16_t pulse) {
  return static_cast<uint32_t>(abs(static_cast<int32_t>(pulse)));
}

void estimatePulseClasses(const int16_t* pulses, uint16_t count,
                          uint16_t* output, uint8_t& outputCount) {
  constexpr uint8_t MAX_CLASSES = 12;
  uint32_t centers[MAX_CLASSES] = {0};
  uint16_t hits[MAX_CLASSES] = {0};
  uint8_t classes = 0;

  for (uint16_t i = 0; i < count; i++) {
    const uint32_t value = pulseWidth(pulses[i]);
    if (value < 140 || value > 20000) continue;
    bool assigned = false;
    for (uint8_t c = 0; c < classes; c++) {
      const uint32_t tolerance = max(static_cast<uint32_t>(90), centers[c] / 5);
      const uint32_t difference = value > centers[c] ? value - centers[c] : centers[c] - value;
      if (difference <= tolerance) {
        centers[c] = (centers[c] * hits[c] + value) / (hits[c] + 1);
        hits[c]++;
        assigned = true;
        break;
      }
    }
    if (!assigned && classes < MAX_CLASSES) {
      centers[classes] = value;
      hits[classes] = 1;
      classes++;
    }
  }

  outputCount = 0;
  for (uint8_t pick = 0; pick < 6; pick++) {
    uint16_t bestHits = 0;
    uint8_t bestIndex = 0xFF;
    for (uint8_t c = 0; c < classes; c++) {
      if (hits[c] > bestHits) {
        bestHits = hits[c];
        bestIndex = c;
      }
    }
    if (bestIndex == 0xFF || bestHits == 0) break;
    const uint32_t limitedCenter = centers[bestIndex] > UINT16_MAX
        ? static_cast<uint32_t>(UINT16_MAX)
        : centers[bestIndex];
    output[outputCount++] = static_cast<uint16_t>(limitedCenter);
    hits[bestIndex] = 0;
  }

  for (uint8_t i = 0; i < outputCount; i++) {
    for (uint8_t j = i + 1; j < outputCount; j++) {
      if (output[j] < output[i]) {
        const uint16_t temp = output[i]; output[i] = output[j]; output[j] = temp;
      }
    }
  }
}

String binaryString(uint64_t code, uint8_t bits) {
  if (!bits || bits > 64) return String();
  String result;
  result.reserve(bits);
  for (int8_t i = static_cast<int8_t>(bits) - 1; i >= 0; i--)
    result += ((code >> i) & 1ULL) ? '1' : '0';
  return result;
}
}  // namespace

void analyzerReset() { snapshot = AnalyzerSnapshot{}; }

void analyzerProcess(const int16_t* pulses, uint16_t count, uint32_t durationUs,
                     float frequencyMHz, float rssiDbm, bool accepted,
                     const String& rejectReason) {
  const uint32_t decodedBefore = snapshot.decodedFrames;
  const uint32_t unknownBefore = snapshot.unknownFrames;
  const uint32_t nextSequence = snapshot.sequence + 1;

  snapshot = AnalyzerSnapshot{};
  snapshot.available = true;
  snapshot.sequence = nextSequence;
  snapshot.capturedAtMs = millis();
  snapshot.frequencyMHz = frequencyMHz;
  snapshot.rssiDbm = rssiDbm;
  snapshot.pulseCount = count;
  snapshot.durationUs = durationUs;
  snapshot.accepted = accepted;
  snapshot.status = accepted ? "Accepted" : "Rejected";
  snapshot.rejectReason = rejectReason;
  snapshot.decodedFrames = decodedBefore;
  snapshot.unknownFrames = unknownBefore;
  estimatePulseClasses(pulses, count, snapshot.pulseClasses, snapshot.pulseClassCount);

  const uint16_t previewCount = min(count, static_cast<uint16_t>(OPENRF_ANALYZER_RAW_PREVIEW));
  snapshot.rawPulseCount = previewCount;
  uint64_t pulseSum = 0;
  uint32_t minPulse = UINT32_MAX;
  uint32_t maxPulse = 0;
  for (uint16_t i = 0; i < count; i++) {
    const uint32_t width = pulseWidth(pulses[i]);
    pulseSum += width;
    if (width < minPulse) minPulse = width;
    if (width > maxPulse) maxPulse = width;
    if (i < previewCount) snapshot.rawPulses[i] = pulses[i];
  }
  if (count) {
    snapshot.minPulseUs = static_cast<uint16_t>(minPulse > UINT16_MAX ? UINT16_MAX : minPulse);
    snapshot.maxPulseUs = static_cast<uint16_t>(maxPulse > UINT16_MAX ? UINT16_MAX : maxPulse);
    const uint32_t average = static_cast<uint32_t>(pulseSum / count);
    snapshot.averagePulseUs = static_cast<uint16_t>(average > UINT16_MAX ? UINT16_MAX : average);
  }
  if (snapshot.pulseClassCount) snapshot.shortestClassUs = snapshot.pulseClasses[0];
  if (snapshot.pulseClassCount >= 2 && snapshot.pulseClasses[0])
    snapshot.classRatio = static_cast<float>(snapshot.pulseClasses[1]) / snapshot.pulseClasses[0];

  const DecodedRFEvent decoded = universalDecode(pulses, count);
  if (decoded.valid) {
    snapshot.protocol = decoded.protocol;
    snapshot.encoding = decoded.protocol == "PT2262" ? "Tri-state PWM" : "OOK PWM";
    snapshot.deviceId = decoded.deviceId;
    snapshot.command = decoded.command;
    snapshot.symbolCount = decoded.symbolCount;
    snapshot.code = decoded.numericCode;
    snapshot.basePulseUs = decoded.pulseLengthUs;
    snapshot.frameCount = decoded.repeats;
    snapshot.quality = decoded.quality;
    if (decoded.symbolCount <= 64 && decoded.protocol != "PT2262")
      snapshot.bitstream = binaryString(decoded.numericCode, decoded.symbolCount);
    snapshot.decodedFrames++;
  } else {
    snapshot.protocol = "Unknown";
    snapshot.encoding = snapshot.pulseClassCount >= 2 ? "OOK / PWM candidate" : "Unknown";
    snapshot.unknownFrames++;
  }
}

AnalyzerSnapshot analyzerGetSnapshot() { return snapshot; }
