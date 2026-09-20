#include "raw_slot_matcher.h"

#include <esp_heap_caps.h>
#include <math.h>
#include <string.h>

#include "dualcore.h"
#include "raw_match.h"
#include "raw_slot_dedup.h"
#include "scratch.h"
#include "storage.h"

namespace {
constexpr float RAW_SLOT_FREQUENCY_TOLERANCE_MHZ = 0.500F;

struct CachedRawSlot {
  bool used = false;
  uint8_t radioId = 0;
  float frequencyMHz = 0.0F;
  uint32_t fingerprint = 0;
  uint16_t patternCount = 0;
  bool repeatReduced = false;
  RawSlotMatchStats stats;
  uint32_t lastEmitAtMs = 0;
};

CachedRawSlot cache[OPENRF_SLOT_COUNT + 1];
int16_t* patternStorage = nullptr;
RawSlotMatcherDiagnostics diagnostics;

int16_t* slotPattern(uint8_t slot) {
  if (!patternStorage || slot < 1 || slot > OPENRF_SLOT_COUNT) return nullptr;
  return patternStorage +
         static_cast<size_t>(slot - 1U) * OpenRfRawMatch::kMaxPatternPulses;
}

bool validSlot(uint8_t slot) {
  return slot >= 1U && slot <= OPENRF_SLOT_COUNT;
}

bool allocatePatternStorage() {
  if (patternStorage) return true;
  const size_t pulseCapacity =
      static_cast<size_t>(OPENRF_SLOT_COUNT) *
      OpenRfRawMatch::kMaxPatternPulses;
  const size_t bytes = pulseCapacity * sizeof(int16_t);

  void* memory = nullptr;
  if (ESP.getPsramSize() > 0U) {
    memory = heap_caps_calloc(pulseCapacity, sizeof(int16_t),
                              MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  }
  if (!memory) {
    memory = heap_caps_calloc(pulseCapacity, sizeof(int16_t),
                              MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  patternStorage = static_cast<int16_t*>(memory);

  if (patternStorage) {
    Serial.print(F("Step 40 RAW matcher cache: "));
    Serial.print(bytes);
    Serial.println(F(" bytes allocated"));
  }
  return patternStorage != nullptr;
}

void resetCacheEntry(uint8_t slot, bool resetStats) {
  if (!validSlot(slot)) return;
  const RawSlotMatchStats previousStats = cache[slot].stats;
  cache[slot] = CachedRawSlot{};
  if (!resetStats) cache[slot].stats = previousStats;
  int16_t* const destination = slotPattern(slot);
  if (destination) {
    memset(destination, 0,
           OpenRfRawMatch::kMaxPatternPulses * sizeof(int16_t));
  }
}

}  // namespace

void __attribute__((weak)) mqttPublishRawSlotEvent(
    uint8_t, const SlotInfo&, const RawSlotMatchStats&) {}

bool rawSlotMatcherBegin() {
  if (!allocatePatternStorage()) {
    Serial.println(F("WARNING: Step 40 RAW matcher cache allocation failed"));
    return false;
  }

  uint8_t loaded = 0U;
  for (uint8_t slot = 1U; slot <= OPENRF_SLOT_COUNT; ++slot) {
    if (rawSlotMatcherReload(slot)) ++loaded;
    yield();
  }

  Serial.print(F("Step 40 bidirectional RAW matcher loaded slots: "));
  Serial.println(loaded);
  return true;
}

bool rawSlotMatcherReload(uint8_t slot) {
  if (!validSlot(slot) || !patternStorage || !openrfScratch) return false;

  const RawSlotMatchStats previousStats = cache[slot].stats;
  const uint32_t previousFingerprint = cache[slot].fingerprint;
  resetCacheEntry(slot, true);

  SlotInfo info;
  if (!storageLoadSlot(slot, openrfScratch, OPENRF_MAX_RAW_PULSES, info)) {
    return false;
  }

  int16_t* const destination = slotPattern(slot);
  if (!destination) return false;

  const OpenRfRawMatch::PreparedPattern prepared = OpenRfRawMatch::prepare(
      openrfScratch, info.pulseCount, destination,
      OpenRfRawMatch::kMaxPatternPulses);
  if (prepared.pulseCount < OpenRfRawMatch::kMinimumUsefulPulses) {
    resetCacheEntry(slot, true);
    return false;
  }

  CachedRawSlot& entry = cache[slot];
  entry.used = true;
  entry.radioId = info.radioId;
  entry.frequencyMHz = info.frequencyMHz;
  entry.fingerprint = info.fingerprint;
  entry.patternCount = prepared.pulseCount;
  entry.repeatReduced = prepared.repeatReduced;
  // A re-save/relearn creates a new receive identity. Preserve counters only
  // when the stored signal fingerprint did not change.
  if (previousFingerprint != 0U && previousFingerprint == info.fingerprint) {
    entry.stats = previousStats;
  }
  entry.stats.available = true;
  return true;
}

void rawSlotMatcherClear(uint8_t slot) {
  if (!validSlot(slot)) return;
  resetCacheEntry(slot, true);
}

void rawSlotMatcherHandleRFEvent(const RFEventMessage& event) {
  const bool supportedEvent =
      event.type == RFEventType::RX_FRAME ||
      event.type == RFEventType::RAW_MATCH_CANDIDATE;
  if (!supportedEvent || !event.rawMatchEligible ||
      !patternStorage || event.pulseCount == 0U) {
    return;
  }

  int16_t incoming[OpenRfRawMatch::kMaxPatternPulses];
  const OpenRfRawMatch::PreparedPattern prepared = OpenRfRawMatch::prepare(
      event.pulses, event.pulseCount, incoming,
      OpenRfRawMatch::kMaxPatternPulses);
  if (prepared.pulseCount < OpenRfRawMatch::kMinimumUsefulPulses) return;

  uint8_t bestSlot = 0U;
  OpenRfRawMatch::MatchResult bestResult;
  uint8_t bestCandidateSlot = 0U;
  OpenRfRawMatch::MatchResult bestCandidateResult;

  for (uint8_t slot = 1U; slot <= OPENRF_SLOT_COUNT; ++slot) {
    CachedRawSlot& entry = cache[slot];
    if (!entry.used || entry.patternCount == 0U) continue;

    if (entry.radioId != 0U && event.radioId != 0U &&
        entry.radioId != event.radioId) {
      continue;
    }
    if (entry.frequencyMHz > 0.0F && event.frequencyMhz > 0.0F &&
        fabsf(entry.frequencyMHz - event.frequencyMhz) >
            RAW_SLOT_FREQUENCY_TOLERANCE_MHZ) {
      continue;
    }

    const OpenRfRawMatch::MatchResult result =
        OpenRfRawMatch::comparePrepared(
            slotPattern(slot), entry.patternCount,
            incoming, prepared.pulseCount);

    // FIX1 diagnostics: retain the strongest attempted comparison even when
    // it did not cross the conservative match gates. Without this, a NO_MATCH
    // collapses to slot 0 / similarity 0 and hides whether timing, pulse count
    // or polarity caused the rejection.
    if (bestCandidateSlot == 0U ||
        result.similarity > bestCandidateResult.similarity) {
      bestCandidateSlot = slot;
      bestCandidateResult = result;
    }

    if (!result.matched) continue;
    if (bestSlot == 0U || result.similarity > bestResult.similarity) {
      bestSlot = slot;
      bestResult = result;
    }
  }

  diagnostics.available = true;
  diagnostics.incomingPatternPulses = prepared.pulseCount;
  diagnostics.incomingRepeatReduced = prepared.repeatReduced;
  diagnostics.lastSlot = bestCandidateSlot;
  diagnostics.lastSimilarity = bestCandidateResult.similarity;
  diagnostics.lastTimingSimilarity = bestCandidateResult.timingSimilarity;
  diagnostics.lastCountSimilarity = bestCandidateResult.countSimilarity;
  diagnostics.lastSignAgreement = bestCandidateResult.signAgreement;
  diagnostics.lastComparedPulses = bestCandidateResult.comparedPulses;
  diagnostics.learnedPatternPulses =
      bestCandidateSlot != 0U ? cache[bestCandidateSlot].patternCount : 0U;
  diagnostics.learnedRepeatReduced =
      bestCandidateSlot != 0U ? cache[bestCandidateSlot].repeatReduced : false;

  if (bestSlot == 0U) {
    diagnostics.lastState = RawSlotMatchState::NO_MATCH;
    diagnostics.noMatchCount++;
    return;
  }

  CachedRawSlot& best = cache[bestSlot];
  diagnostics.lastSlot = bestSlot;
  diagnostics.lastSimilarity = bestResult.similarity;
  diagnostics.lastTimingSimilarity = bestResult.timingSimilarity;
  diagnostics.lastCountSimilarity = bestResult.countSimilarity;
  diagnostics.lastSignAgreement = bestResult.signAgreement;
  diagnostics.lastComparedPulses = bestResult.comparedPulses;
  diagnostics.learnedPatternPulses = best.patternCount;
  diagnostics.learnedRepeatReduced = best.repeatReduced;

  best.stats.lastSimilarity = bestResult.similarity;
  best.stats.lastRssi = event.rssiDbm;
  best.stats.lastMatchedAtMs = millis();

  const uint32_t now = millis();
  // FIX6: use a sliding inactivity gate. Previously lastEmitAtMs changed only
  // on emitted events, so a long train of valid repeats generated a new event
  // every 300 ms. Every same-slot match now refreshes the timestamp, while the
  // first match after at least 300 ms of silence still emits immediately.
  if (!rawSlotDedupShouldEmit(best.lastEmitAtMs, now)) {
    diagnostics.lastState = RawSlotMatchState::SUPPRESSED_DUPLICATE;
    diagnostics.duplicateSuppressedCount++;
    return;
  }

  best.stats.matchCount++;
  diagnostics.lastState = RawSlotMatchState::MATCH;
  diagnostics.matchCount++;

  const SlotInfo info = storageGetSlotInfo(bestSlot);
  Serial.print(F("RAW slot RX match: slot "));
  Serial.print(bestSlot);
  Serial.print(F(", similarity="));
  Serial.print(bestResult.similarity);
  Serial.print(F("%, R"));
  Serial.print(event.radioId);
  Serial.print(F(" @ "));
  Serial.print(event.frequencyMhz, 4);
  Serial.println(F(" MHz"));

  mqttPublishRawSlotEvent(bestSlot, info, best.stats);
}

RawSlotMatchStats rawSlotMatcherGetStats(uint8_t slot) {
  if (!validSlot(slot)) return RawSlotMatchStats{};
  RawSlotMatchStats stats = cache[slot].stats;
  stats.available = cache[slot].used;
  return stats;
}

RawSlotMatcherDiagnostics rawSlotMatcherGetDiagnostics() {
  return diagnostics;
}

const char* rawSlotMatchStateName(RawSlotMatchState state) {
  switch (state) {
    case RawSlotMatchState::NO_MATCH:
      return "NO_MATCH";
    case RawSlotMatchState::MATCH:
      return "MATCH";
    case RawSlotMatchState::SUPPRESSED_DUPLICATE:
      return "SUPPRESSED_DUPLICATE";
    default:
      return "IDLE";
  }
}
