#include "decoder.h"

namespace {

const Nvkp01Decoder INSTANCE;

constexpr uint16_t MIN_CAPTURE_PULSES = 18;
constexpr uint16_t MARKER_LOW_MIN_US = 1250;
constexpr uint16_t MARKER_LOW_MAX_US = 2100;
constexpr uint16_t MARKER_HIGH_MIN_US = 280;
constexpr uint16_t MARKER_HIGH_MAX_US = 950;
constexpr uint16_t SYNC_LOW_MIN_US = 5000;
constexpr uint16_t SYNC_LOW_MAX_US = 6300;
constexpr uint8_t MIN_MARKERS_WITHOUT_SYNC = 3;
constexpr uint8_t MIN_MARKERS_WITH_SYNC = 2;

uint32_t pulseWidth(const int16_t pulse) {
  return static_cast<uint32_t>(abs(static_cast<int32_t>(pulse)));
}

bool inRange(const int16_t pulse,
             const bool expectedPositive,
             const uint16_t minimumUs,
             const uint16_t maximumUs) {
  if (expectedPositive && pulse <= 0) return false;
  if (!expectedPositive && pulse >= 0) return false;

  const uint32_t value = pulseWidth(pulse);
  return value >= minimumUs && value <= maximumUs;
}

bool isMarkerPair(const int16_t lowPulse, const int16_t highPulse) {
  return inRange(lowPulse, false, MARKER_LOW_MIN_US, MARKER_LOW_MAX_US) &&
         inRange(highPulse, true, MARKER_HIGH_MIN_US, MARKER_HIGH_MAX_US);
}

bool isSyncLow(const int16_t pulse) {
  return inRange(pulse, false, SYNC_LOW_MIN_US, SYNC_LOW_MAX_US);
}

bool isFullyAlternating(const int16_t* pulses, const uint16_t pulseCount) {
  for (uint16_t i = 0; i + 1 < pulseCount; i++) {
    if ((pulses[i] > 0) == (pulses[i + 1] > 0)) return false;
  }
  return true;
}

uint8_t countMarkerPairs(const int16_t* pulses, const uint16_t pulseCount) {
  uint8_t markerPairs = 0;

  // Scan every adjacent position instead of assuming that capture began on a
  // frame boundary. This makes matching independent of where RX capture began.
  for (uint16_t i = 0; i + 1 < pulseCount; i++) {
    if (!isMarkerPair(pulses[i], pulses[i + 1])) continue;
    if (markerPairs < UINT8_MAX) markerPairs++;
  }

  return markerPairs;
}

uint8_t countSyncPulses(const int16_t* pulses, const uint16_t pulseCount) {
  uint8_t syncPulses = 0;
  for (uint16_t i = 0; i < pulseCount; i++) {
    if (!isSyncLow(pulses[i])) continue;
    if (syncPulses < UINT8_MAX) syncPulses++;
  }
  return syncPulses;
}

bool containsFullLeader(const int16_t* pulses, const uint16_t pulseCount) {
  // Full leader: marker pair followed immediately by the long LOW sync and a
  // normal HIGH recovery pulse. Search the complete capture, not only its head.
  for (uint16_t i = 0; i + 3 < pulseCount; i++) {
    if (isMarkerPair(pulses[i], pulses[i + 1]) &&
        isSyncLow(pulses[i + 2]) &&
        inRange(pulses[i + 3], true, 450, 950)) {
      return true;
    }
  }
  return false;
}

}  // namespace

const char* Nvkp01Decoder::name() const {
  return "NVKP01 Kinetic";
}

bool Nvkp01Decoder::matches(const int16_t* pulses,
                            const uint16_t pulseCount) const {
  if (pulses == nullptr || pulseCount < MIN_CAPTURE_PULSES) return false;

  // RX cleanup should already provide a strictly alternating OOK sequence.
  // Keeping this requirement prevents marker-like pulse widths in noisy or
  // damaged captures from producing false NVKP01 recognition.
  if (!isFullyAlternating(pulses, pulseCount)) return false;

  // A complete measured leader is sufficient by itself.
  if (containsFullLeader(pulses, pulseCount)) return true;

  const uint8_t markerPairs = countMarkerPairs(pulses, pulseCount);
  const uint8_t syncPulses = countSyncPulses(pulses, pulseCount);

  /*
   * Capture-position-independent recognition:
   *
   * - Three marker pairs are accepted even when the ~5.7 ms sync gap was
   *   consumed as a capture boundary.
   * - Two marker pairs are enough when a measured ~5.7 ms sync LOW is also
   *   present somewhere in the capture.
   *
   * This is still structural recognition only. It does not emit PRESS,
   * RELEASE or any other Kinetic event until the payload is understood.
   */
  if (markerPairs >= MIN_MARKERS_WITHOUT_SYNC) return true;
  return markerPairs >= MIN_MARKERS_WITH_SYNC && syncPulses >= 1;
}

bool Nvkp01Decoder::decode(const int16_t* pulses,
                           const uint16_t pulseCount,
                           KineticEvent& event) const {
  event = KineticEvent{};
  if (!matches(pulses, pulseCount)) return false;

  /*
   * NVKP01 currently exposes one physical control. Both mechanical phases
   * are intentionally normalized to one PRESS event. RX Slot lockout handles
   * the closely spaced duplicate telegram produced by a normal click cycle.
   * PRESS/RELEASE separation can be added later without changing the shared
   * Kinetic event model or the saved RX Slot format.
   */
  event.valid = true;
  event.type = KineticEventType::PRESS;
  snprintf(event.deviceId, sizeof(event.deviceId), "%s", "nvkp01");
  snprintf(event.controlId, sizeof(event.controlId), "%s", "button");
  event.channel = 1;
  event.repeat = false;

  const uint8_t markerPairs = countMarkerPairs(pulses, pulseCount);
  const bool fullLeader = containsFullLeader(pulses, pulseCount);
  const uint8_t scoredMarkers = markerPairs > 6U ? 6U : markerPairs;
  uint16_t quality = 82U + static_cast<uint16_t>(scoredMarkers) * 2U;
  if (fullLeader) quality += 6U;
  if (quality > 100U) quality = 100U;

  event.metadata.protocolName = name();
  event.metadata.quality = static_cast<uint8_t>(quality);
  event.metadata.repeats = markerPairs > 0 ? markerPairs : 1;
  event.metadata.pulseCount = pulseCount;
  return true;
}

const Nvkp01Decoder& nvkp01Decoder() {
  return INSTANCE;
}
