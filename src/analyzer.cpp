#include <Arduino.h>
#include "analyzer.h"
#include "universal_decoder.h"
#include "config.h"

namespace {
AnalyzerSnapshot snapshot;
AnalyzerCandidateSnapshot lastCandidate;
uint32_t weakRssiFrames = 0;
float peakRssiDbm = -127.0F;

constexpr uint8_t REJECT_CLUSTER_COUNT = 4;
constexpr uint32_t STRUCTURED_WINDOW_MS = 2500;

struct RejectedCluster {
  bool used = false;
  uint16_t pulseCount = 0;
  uint32_t durationUs = 0;
  uint16_t pulseClasses[6] = {0};
  uint8_t pulseClassCount = 0;
  uint8_t occurrences = 0;
  uint32_t lastSeenMs = 0;
  String reason;
};

RejectedCluster rejectedClusters[REJECT_CLUSTER_COUNT];

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


uint8_t relativeSimilarity(uint32_t a, uint32_t b) {
  if (!a || !b) return 0;

  const uint32_t larger = max(a, b);
  const uint32_t difference = a > b ? a - b : b - a;

  const uint32_t score =
      difference >= larger
          ? 0U
          : 100U - ((difference * 100U) / larger);

  return static_cast<uint8_t>(score > 100U ? 100U : score);
}
uint8_t clusterSimilarity(const RejectedCluster& cluster, uint16_t count,
                          uint32_t durationUs, const uint16_t* classes,
                          uint8_t classCount, const String& reason) {
  if (!cluster.used || cluster.reason != reason) return 0;
  const uint8_t countScore = relativeSimilarity(cluster.pulseCount, count);
  const uint8_t durationScore = relativeSimilarity(cluster.durationUs, durationUs);
  const uint8_t shared = min(cluster.pulseClassCount, classCount);
  if (shared < 2) return 0;
  uint16_t classScoreSum = 0;
  for (uint8_t i = 0; i < shared; i++)
    classScoreSum += relativeSimilarity(cluster.pulseClasses[i], classes[i]);
  const uint8_t classScore = classScoreSum / shared;
  return static_cast<uint8_t>((countScore * 30UL + durationScore * 30UL + classScore * 40UL) / 100UL);
}

bool eligibleRejectedCandidate(uint16_t count, uint32_t durationUs,
                               uint8_t classCount, const String& reason, uint8_t alternationRatio) {
  if (count < config.analyzerMinPulseCount || durationUs < config.analyzerMinDurationUs || classCount < 2) return false;
  if (reason == "implausible_pulse_widths" || reason == "too_many_pulses" || reason == "frame_too_long") return false;
  if (reason == "non_alternating_noise" && alternationRatio < config.analyzerAlternationTolerance) return false;
  return true;
}


void analyzeAlternation(const int16_t* pulses, uint16_t count, AnalyzerCandidateSnapshot& out) {
  if (!count) return;
  uint16_t alternatingPairs = 0;
  uint16_t samePairs = 0;
  uint8_t currentRun = 1;
  uint8_t longestRun = 1;
  int32_t merged = pulses[0];
  uint16_t normalizedCount = 0;
  for (uint16_t i = 1; i < count; i++) {
    const bool sameSign = (pulses[i] < 0) == (pulses[i - 1] < 0);
    if (sameSign) {
      samePairs++;
      if (currentRun < 255) currentRun++;
      if (currentRun > longestRun) longestRun = currentRun;
      merged += pulses[i];
    } else {
      alternatingPairs++;
      if (normalizedCount < OPENRF_ANALYZER_RAW_PREVIEW) {
        if (merged > INT16_MAX) merged = INT16_MAX;
        if (merged < INT16_MIN) merged = INT16_MIN;
        out.normalizedPulses[normalizedCount++] = static_cast<int16_t>(merged);
      }
      merged = pulses[i];
      currentRun = 1;
    }
  }
  if (normalizedCount < OPENRF_ANALYZER_RAW_PREVIEW) {
    if (merged > INT16_MAX) merged = INT16_MAX;
    if (merged < INT16_MIN) merged = INT16_MIN;
    out.normalizedPulses[normalizedCount++] = static_cast<int16_t>(merged);
  }
  out.sameSignPairs = samePairs;
  out.longestSameSignRun = longestRun;
  out.normalizedPulseCount = normalizedCount;
  const uint16_t pairs = count > 1 ? count - 1 : 0;
  out.alternationRatio = pairs ? static_cast<uint8_t>((alternatingPairs * 100UL) / pairs) : 100;
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

void analyzerReset() {
  snapshot = AnalyzerSnapshot{};
  lastCandidate = AnalyzerCandidateSnapshot{};
  weakRssiFrames = 0;
  peakRssiDbm = -127.0F;
  for (auto& cluster : rejectedClusters) cluster = RejectedCluster{};
}

void analyzerRecordCandidate(const int16_t* pulses, uint16_t count, uint32_t durationUs,
                             float frequencyMHz, float rssiDbm,
                             const String& rejectReason) {
  if (config.analyzerFreezeCandidate && lastCandidate.available) return;
  const uint32_t nextSequence = lastCandidate.sequence + 1;
  lastCandidate = AnalyzerCandidateSnapshot{};
  lastCandidate.available = true;
  lastCandidate.sequence = nextSequence;
  lastCandidate.capturedAtMs = millis();
  lastCandidate.frequencyMHz = frequencyMHz;
  lastCandidate.rssiDbm = rssiDbm;
  lastCandidate.pulseCount = count;
  lastCandidate.durationUs = durationUs;
  lastCandidate.rejectReason = rejectReason;
  lastCandidate.rawPulseCount = min(count, static_cast<uint16_t>(OPENRF_ANALYZER_RAW_PREVIEW));
  uint32_t minPulse = UINT32_MAX;
  uint32_t maxPulse = 0;
  for (uint16_t i = 0; i < count; i++) {
    const uint32_t width = pulseWidth(pulses[i]);
    if (width < minPulse) minPulse = width;
    if (width > maxPulse) maxPulse = width;
    if (i < lastCandidate.rawPulseCount) lastCandidate.rawPulses[i] = pulses[i];
  }
  analyzeAlternation(pulses, count, lastCandidate);
  if (count) {
    lastCandidate.minPulseUs = static_cast<uint16_t>(minPulse > UINT16_MAX ? UINT16_MAX : minPulse);
    lastCandidate.maxPulseUs = static_cast<uint16_t>(maxPulse > UINT16_MAX ? UINT16_MAX : maxPulse);
  }
}

AnalyzerCandidateSnapshot analyzerGetLastCandidate() { return lastCandidate; }

bool analyzerRssiPasses(float rssiDbm) {
  if (rssiDbm > peakRssiDbm) peakRssiDbm = rssiDbm;
  return rssiDbm >= static_cast<float>(config.analyzerMinRssi);
}

void analyzerRecordWeakRssi(float rssiDbm) {
  if (weakRssiFrames < UINT32_MAX) weakRssiFrames++;
  if (rssiDbm > peakRssiDbm) peakRssiDbm = rssiDbm;
}

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
  snapshot.weakRssiFrames = weakRssiFrames;
  snapshot.peakRssiDbm = peakRssiDbm;
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


bool analyzerConsiderRejected(const int16_t* pulses, uint16_t count, uint32_t durationUs,
                              float frequencyMHz, float rssiDbm,
                              const String& rejectReason) {
  uint16_t classes[6] = {0};
  uint8_t classCount = 0;
  AnalyzerCandidateSnapshot metrics;
  analyzeAlternation(pulses, count, metrics);
  const int16_t* analysisPulses = metrics.normalizedPulseCount ? metrics.normalizedPulses : pulses;
  const uint16_t analysisCount = metrics.normalizedPulseCount ? metrics.normalizedPulseCount : count;
  estimatePulseClasses(analysisPulses, analysisCount, classes, classCount);
  if (!eligibleRejectedCandidate(analysisCount, durationUs, classCount, rejectReason, metrics.alternationRatio)) return false;

  const uint32_t now = millis();
  int8_t bestIndex = -1;
  uint8_t bestSimilarity = 0;
  int8_t replacementIndex = 0;
  uint32_t oldestSeen = UINT32_MAX;

  for (uint8_t i = 0; i < REJECT_CLUSTER_COUNT; i++) {
    RejectedCluster& cluster = rejectedClusters[i];
    if (!cluster.used || static_cast<uint32_t>(now - cluster.lastSeenMs) > STRUCTURED_WINDOW_MS) {
      if (!cluster.used) { replacementIndex = i; oldestSeen = 0; }
      else if (cluster.lastSeenMs < oldestSeen) { replacementIndex = i; oldestSeen = cluster.lastSeenMs; }
      continue;
    }
    const uint8_t similarity = clusterSimilarity(cluster, analysisCount, durationUs, classes, classCount, rejectReason);
    if (similarity > bestSimilarity) { bestSimilarity = similarity; bestIndex = i; }
    if (cluster.lastSeenMs < oldestSeen) { replacementIndex = i; oldestSeen = cluster.lastSeenMs; }
  }

  RejectedCluster* cluster = nullptr;
  if (bestIndex >= 0 && bestSimilarity >= config.analyzerSimilarity) {
    cluster = &rejectedClusters[bestIndex];
    if (cluster->occurrences < 255) cluster->occurrences++;
    cluster->pulseCount = static_cast<uint16_t>((cluster->pulseCount * 2UL + analysisCount) / 3UL);
    cluster->durationUs = (cluster->durationUs * 2UL + durationUs) / 3UL;
    for (uint8_t i = 0; i < min(cluster->pulseClassCount, classCount); i++)
      cluster->pulseClasses[i] = static_cast<uint16_t>((cluster->pulseClasses[i] * 2UL + classes[i]) / 3UL);
  } else {
    cluster = &rejectedClusters[replacementIndex];
    *cluster = RejectedCluster{};
    cluster->used = true;
    cluster->pulseCount = analysisCount;
    cluster->durationUs = durationUs;
    cluster->pulseClassCount = classCount;
    for (uint8_t i = 0; i < classCount; i++) cluster->pulseClasses[i] = classes[i];
    cluster->occurrences = 1;
    cluster->reason = rejectReason;
    bestSimilarity = 100;
  }
  cluster->lastSeenMs = now;

  if (cluster->occurrences < config.analyzerOccurrences) return false;

  analyzerProcess(pulses, count, durationUs, frequencyMHz, rssiDbm, false, rejectReason);
  snapshot.status = "Structured unknown";
  snapshot.protocol = "Structured unknown";
  snapshot.encoding = snapshot.pulseClassCount >= 2 ? "OOK / PWM candidate" : "Unknown";
  snapshot.structuredSignal = true;
  snapshot.occurrences = cluster->occurrences;
  snapshot.similarity = bestSimilarity;
  return true;
}

AnalyzerSnapshot analyzerGetSnapshot() {
  AnalyzerSnapshot current = snapshot;
  current.weakRssiFrames = weakRssiFrames;
  current.peakRssiDbm = peakRssiDbm;
  return current;
}
