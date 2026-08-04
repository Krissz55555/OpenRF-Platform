#include <Arduino.h>
#include <RadioLib.h>

#include "hardware.h"
#include "radio.h"
#include "analyzer.h"
#include "config.h"
#include "universal_decoder.h"

namespace {
CC1101 cc1101 = new Module(OPENRF_CC1101_CS_PIN, OPENRF_CC1101_GDO0_PIN,
                           RADIOLIB_NC, OPENRF_CC1101_GDO2_PIN);

constexpr uint32_t RSSI_REFRESH_INTERVAL_MS = 5;
constexpr uint32_t FRAME_GAP_US = 25000;
constexpr uint16_t MIN_CAPTURE_PULSES = 20;
constexpr uint16_t MIN_VALID_PULSES = 30;
constexpr uint16_t MIN_MONITOR_PULSES = 60;
constexpr uint16_t MAX_VALID_PULSES = 500;
constexpr uint32_t MIN_FRAME_DURATION_US = 10000;
constexpr uint32_t MIN_MONITOR_DURATION_US = 80000;
constexpr uint32_t MAX_FRAME_DURATION_US = 400000;
constexpr uint32_t NOISE_US = 150;
constexpr float LEARN_RSSI_DELTA_DB = 6.0F;
constexpr uint16_t MAX_CLUSTER_COUNT = 12;

volatile int16_t rxPulses[OPENRF_MAX_RAW_PULSES];
volatile uint16_t rxCount = 0;
volatile uint32_t lastEdgeUs = 0;
volatile int lastLevel = LOW;
volatile bool frameReady = false;
volatile bool captureEnabled = false;
volatile uint32_t ignoredGlitchEdges = 0;
volatile uint32_t gapFinalizedFrames = 0;
volatile uint32_t timeoutFinalizedFrames = 0;
volatile uint32_t bufferFullFrames = 0;
volatile uint32_t mergedSameSignPulses = 0;

int16_t lastRaw[OPENRF_MAX_RAW_PULSES];
RawFrameInfo lastFrame;
int16_t learnRaw[OPENRF_MAX_RAW_PULSES];
LearnCaptureInfo learnCapture;

void IRAM_ATTR gdo0ISR() {
  if (!captureEnabled) return;

  const uint32_t now = micros();
  const uint32_t duration = now - lastEdgeUs;
  const int level = digitalRead(OPENRF_CC1101_GDO0_PIN);

  // Ignore very short glitches without moving the accepted-edge timestamp or
  // changing the accepted signal level. Updating either value here would split
  // one real pulse into fragments and can create artificial same-sign pairs.
  if (duration < NOISE_US) {
    ignoredGlitchEdges++;
    return;
  }

  if (frameReady) return;

  // This is now an accepted edge, so it becomes the reference for the next
  // measured pulse.
  lastEdgeUs = now;

  if (duration > FRAME_GAP_US) {
    if (rxCount >= MIN_CAPTURE_PULSES) {
      frameReady = true;
      gapFinalizedFrames++;
    } else {
      // A long idle period synchronizes the receiver to the next complete
      // transmission and discards any short partial/noise fragment.
      rxCount = 0;
    }
    lastLevel = level;
    return;
  }

  if (rxCount < OPENRF_MAX_RAW_PULSES) {
    const int32_t signedDuration = (lastLevel == HIGH)
        ? static_cast<int32_t>(duration)
        : -static_cast<int32_t>(duration);

    // Defensive RAW cleanup: two consecutive entries with the same sign cannot
    // represent two valid OOK levels. Merge them before they reach Analyzer,
    // Learn, RX Slots or a future decoder. The normal case still appends one
    // pulse per accepted edge.
    if (rxCount > 0 && ((rxPulses[rxCount - 1] > 0) == (signedDuration > 0))) {
      int32_t merged = static_cast<int32_t>(rxPulses[rxCount - 1]) + signedDuration;
      if (merged > INT16_MAX) merged = INT16_MAX;
      if (merged < INT16_MIN) merged = INT16_MIN;
      rxPulses[rxCount - 1] = static_cast<int16_t>(merged);
      mergedSameSignPulses++;
    } else {
      rxPulses[rxCount++] = static_cast<int16_t>(signedDuration);
    }

    if (rxCount >= OPENRF_MAX_RAW_PULSES) {
      frameReady = true;
      bufferFullFrames++;
    }
  }
  lastLevel = level;
}
}  // namespace

RadioManager Radio;

bool RadioManager::begin() {
  frequencyMHz_ = config.radioFrequencyMhz == 868 ? 868.35F : 433.92F;
  initialized_ = false;
  mode_ = RadioMode::OFFLINE;
  lastRssi_ = -127.0F;
  framePeakRssi_ = -127.0F;
  lastFrame = RawFrameInfo{};
  learnCapture = LearnCaptureInfo{};
  diagnostics_ = RadioDiagnostics{};
  analyzerReset();

  Serial.println("CC1101 initialization started");
  lastError_ = cc1101.begin(frequencyMHz_,
      OPENRF_RADIO_BIT_RATE_KBPS, OPENRF_RADIO_FREQUENCY_DEVIATION_KHZ,
      OPENRF_RADIO_RX_BANDWIDTH_KHZ, OPENRF_RADIO_OUTPUT_POWER_DBM,
      OPENRF_RADIO_PREAMBLE_BITS);
  if (lastError_ != RADIOLIB_ERR_NONE) {
    mode_ = RadioMode::ERROR;
    Serial.print("CC1101 initialization failed, code: ");
    Serial.println(lastError_);
    return false;
  }
  lastError_ = cc1101.setOOK(true);
  if (lastError_ != RADIOLIB_ERR_NONE) {
    mode_ = RadioMode::ERROR;
    Serial.print("CC1101 OOK setup failed, code: ");
    Serial.println(lastError_);
    return false;
  }
  lastError_ = cc1101.disableSyncWordFiltering();
  if (lastError_ != RADIOLIB_ERR_NONE) {
    mode_ = RadioMode::ERROR;
    Serial.print("CC1101 sync filter disable failed, code: ");
    Serial.println(lastError_);
    return false;
  }

  initialized_ = true;
  mode_ = RadioMode::IDLE;
  if (!startReceive()) {
    Serial.println("CC1101 initialized, but RAW RX start failed");
    return false;
  }
  Serial.println("CC1101 initialized successfully");
  Serial.print("Frequency: "); Serial.print(frequencyMHz_, 3); Serial.println(" MHz");
  Serial.print("RX bandwidth: "); Serial.print(OPENRF_RADIO_RX_BANDWIDTH_KHZ, 1); Serial.println(" kHz");
  Serial.println("Mode: RAW RX / OOK");
  Serial.println("RAW receiver armed on GDO0");
  return true;
}

void RadioManager::loop() {
  if (!initialized_ || mode_ != RadioMode::RX) return;
  const uint32_t nowMs = millis();
  if (nowMs - lastRssiReadMs_ >= RSSI_REFRESH_INTERVAL_MS) {
    lastRssiReadMs_ = nowMs;
    lastRssi_ = cc1101.getRSSI();
    uint16_t activeCount;
    noInterrupts(); activeCount = rxCount; interrupts();
    if (activeCount > 0 && lastRssi_ > framePeakRssi_) framePeakRssi_ = lastRssi_;
  }

  bool readyCopy;
  uint16_t countCopy;
  uint32_t edgeCopy;
  noInterrupts();
  readyCopy = frameReady; countCopy = rxCount; edgeCopy = lastEdgeUs;
  interrupts();

  if (!readyCopy && countCopy >= MIN_CAPTURE_PULSES &&
      static_cast<uint32_t>(micros() - edgeCopy) > FRAME_GAP_US) {
    noInterrupts();
    frameReady = true;
    timeoutFinalizedFrames++;
    interrupts();
    readyCopy = true;
  }
  if (readyCopy) finalizeFrame();
}

bool RadioManager::validateFrame(const int16_t* data, uint16_t count,
                                 uint32_t durationUs, String& reason) const {
  if (count < MIN_VALID_PULSES) { reason = "too_few_pulses"; return false; }
  if (count > MAX_VALID_PULSES) { reason = "too_many_pulses"; return false; }
  if (durationUs < MIN_FRAME_DURATION_US) { reason = "frame_too_short"; return false; }
  if (durationUs > MAX_FRAME_DURATION_US) { reason = "frame_too_long"; return false; }

  uint16_t useful = 0;
  uint16_t alternating = 0;
  uint32_t centers[MAX_CLUSTER_COUNT] = {0};
  uint16_t hits[MAX_CLUSTER_COUNT] = {0};
  uint8_t clusters = 0;

  for (uint16_t i = 0; i < count; i++) {
    const uint32_t value = static_cast<uint32_t>(abs(static_cast<int32_t>(data[i])));
    if (i > 0 && ((data[i] > 0) != (data[i - 1] > 0))) alternating++;
    if (value >= NOISE_US && value <= 20000) useful++;
    bool assigned = false;
    for (uint8_t c = 0; c < clusters; c++) {
      const uint32_t tolerance = max(static_cast<uint32_t>(120), centers[c] / 4);
      const uint32_t diff = value > centers[c] ? value - centers[c] : centers[c] - value;
      if (diff <= tolerance) {
        centers[c] = (centers[c] * hits[c] + value) / (hits[c] + 1);
        hits[c]++;
        assigned = true;
        break;
      }
    }
    if (!assigned && clusters < MAX_CLUSTER_COUNT) {
      centers[clusters] = value;
      hits[clusters] = 1;
      clusters++;
    }
  }

  if (useful * 100UL < count * 80UL) { reason = "implausible_pulse_widths"; return false; }
  if (count > 1 && alternating * 100UL < (count - 1) * 88UL) {
    reason = "non_alternating_noise"; return false;
  }

  uint16_t rankedHits[MAX_CLUSTER_COUNT] = {0};
  for (uint8_t c = 0; c < clusters; c++) rankedHits[c] = hits[c];
  uint16_t topCoverage = 0;
  for (uint8_t pick = 0; pick < 6; pick++) {
    uint16_t best = 0; uint8_t bestIndex = 0xFF;
    for (uint8_t c = 0; c < clusters; c++) {
      if (rankedHits[c] > best) { best = rankedHits[c]; bestIndex = c; }
    }
    if (bestIndex == 0xFF) break;
    topCoverage += best;
    rankedHits[bestIndex] = 0;
  }
  if (topCoverage * 100UL < count * 78UL) { reason = "random_timing_pattern"; return false; }

  // Real OOK remotes normally have at least two dominant timing classes.
  // A broad, flat distribution is typical of receiver noise.
  uint32_t dominant[2] = {0, 0};
  uint16_t dominantHits[2] = {0, 0};
  for (uint8_t c = 0; c < clusters; c++) {
    if (hits[c] > dominantHits[0]) {
      dominantHits[1] = dominantHits[0]; dominant[1] = dominant[0];
      dominantHits[0] = hits[c]; dominant[0] = centers[c];
    } else if (hits[c] > dominantHits[1]) {
      dominantHits[1] = hits[c]; dominant[1] = centers[c];
    }
  }
  if (!dominantHits[1]) { reason = "single_timing_class"; return false; }
  if (dominant[0] > dominant[1]) { const uint32_t t=dominant[0]; dominant[0]=dominant[1]; dominant[1]=t; }
  const float timingRatio = dominant[0] ? static_cast<float>(dominant[1]) / dominant[0] : 0.0F;
  if (dominant[0] < 170 || dominant[0] > 1800 || timingRatio < 1.35F || timingRatio > 6.5F) {
    reason = "implausible_timing_classes"; return false;
  }
  reason = "accepted";
  return true;
}

bool RadioManager::validateLearnSignal(float frameRssi, String& reason) const {
  if (learnCapture.noiseFloorDbm <= -120.0F) return true;
  if (frameRssi < learnCapture.noiseFloorDbm + LEARN_RSSI_DELTA_DB) {
    reason = "rssi_not_above_noise_floor";
    return false;
  }
  return true;
}

void RadioManager::finalizeFrame() {
  uint16_t count = 0;
  noInterrupts();
  count = rxCount;
  if (count > OPENRF_MAX_RAW_PULSES) count = OPENRF_MAX_RAW_PULSES;
  for (uint16_t i = 0; i < count; i++) lastRaw[i] = rxPulses[i];
  rxCount = 0; frameReady = false; lastEdgeUs = micros();
  lastLevel = digitalRead(OPENRF_CC1101_GDO0_PIN);
  interrupts();

  uint32_t totalDuration = 0;
  for (uint16_t i = 0; i < count; i++)
    totalDuration += static_cast<uint32_t>(abs(static_cast<int32_t>(lastRaw[i])));

  const float sampledRssi = cc1101.getRSSI();
  if (sampledRssi > framePeakRssi_) framePeakRssi_ = sampledRssi;
  const float frameRssi = framePeakRssi_;
  framePeakRssi_ = -127.0F;
  diagnostics_.rawCandidates++;
  noInterrupts();
  diagnostics_.ignoredGlitchEdges = ignoredGlitchEdges;
  diagnostics_.gapFinalizedFrames = gapFinalizedFrames;
  diagnostics_.timeoutFinalizedFrames = timeoutFinalizedFrames;
  diagnostics_.bufferFullFrames = bufferFullFrames;
  diagnostics_.mergedSameSignPulses = mergedSameSignPulses;
  interrupts();

  String rejectReason;
  const bool learningNow =
      learnCapture.state == LearnState::WAITING_FOR_SIGNAL;

  bool frameValid =
      validateFrame(lastRaw, count, totalDuration, rejectReason);

  /*
   * Give registered protocol decoders a chance before the normal monitoring
   * filter rejects short frames as background traffic.
   *
   * This is important for short Kinetic protocols such as NVKP01. A recognized
   * but not yet actionable protocol may still be displayed by Analyzer, while
   * RX Slots, MQTT and Home Assistant continue to ignore it.
   */
  bool recognizedProtocol = false;

  if (frameValid) {
    const DecodedRFEvent preliminary =
        universalDecode(lastRaw, count);

    recognizedProtocol = preliminary.recognized;
  }

  // Normal monitoring is intentionally quieter than Learn mode. Short
  // background bursts remain filtered unless a registered decoder already
  // recognizes their protocol structure.
  if (frameValid &&
      !learningNow &&
      !recognizedProtocol &&
      count < MIN_MONITOR_PULSES &&
      totalDuration < MIN_MONITOR_DURATION_US) {
    frameValid = false;
    rejectReason = "background_short_frame";
    diagnostics_.backgroundFilteredFrames++;
  }

  // Standard mode intentionally preserves the v1.0.0 Analyzer behaviour.
  // Analyzer v2 candidate capture and additional filters are enabled only by
  // the explicit Developer Mode switch.
  if (config.analyzerDeveloperMode) {
    analyzerRecordCandidate(
        lastRaw,
        count,
        totalDuration,
        frequencyMHz_,
        frameRssi,
        frameValid ? String("accepted") : rejectReason
    );
  }

  if (!frameValid) {
    // In standard mode rejected bursts remain diagnostics only, exactly as in
    // v1.0.0. Learn mode may still expose the rejected capture. Developer Mode
    // can additionally classify repeatable rejected candidates.
    if (config.analyzerDeveloperMode) {
      if (analyzerRssiPasses(frameRssi)) {
        if (config.analyzerShowRejected) {
          analyzerConsiderRejected(lastRaw, count, totalDuration, frequencyMHz_,
                                   frameRssi, rejectReason);
        }
      } else {
        analyzerRecordWeakRssi(frameRssi);
      }
    }
    diagnostics_.rejectedFrames++;
    diagnostics_.lastRejectReason = rejectReason;
    if (learnCapture.state == LearnState::WAITING_FOR_SIGNAL) {
      learnCapture.rejectedDuringLearn++;
      learnCapture.lastRejectReason = rejectReason;
    }
    if ((diagnostics_.rejectedFrames % 10) == 1) {
      Serial.print("RAW rejected: "); Serial.print(rejectReason);
      Serial.print(", "); Serial.print(count); Serial.print(" pulses, ");
      Serial.print(totalDuration); Serial.println(" us");
    }
    return;
  }

  // Full Analyzer processing is intentionally disabled in normal gateway
  // mode. This keeps the ESP8266 stable for RX Slots, MQTT and Home Assistant.
  // Developer Mode enables the complete diagnostics pipeline exclusively.
  if (config.analyzerDeveloperMode) {
    if (analyzerRssiPasses(frameRssi)) {
      analyzerProcess(lastRaw, count, totalDuration, frequencyMHz_, frameRssi,
                      true, "accepted");
    } else {
      analyzerRecordWeakRssi(frameRssi);
    }
  }
  diagnostics_.acceptedFrames++;
  lastFrame.available = true;
  lastFrame.sequence++;
  lastFrame.pulseCount = count;
  lastFrame.durationUs = totalDuration;
  lastFrame.rssiDbm = frameRssi;
  lastFrame.receivedAtMs = millis();

  Serial.print("RAW frame #"); Serial.print(lastFrame.sequence);
  Serial.print(" accepted: "); Serial.print(count); Serial.print(" pulses, ");
  Serial.print(totalDuration); Serial.print(" us, RSSI ");
  Serial.print(frameRssi, 1); Serial.println(" dBm");

  if (learnCapture.state == LearnState::WAITING_FOR_SIGNAL) {
    if (!validateLearnSignal(frameRssi, rejectReason)) {
      learnCapture.rejectedDuringLearn++;
      learnCapture.lastRejectReason = rejectReason;
      Serial.print("LEARN rejected: "); Serial.println(rejectReason);
      return;
    }
    for (uint16_t i = 0; i < count; i++) learnRaw[i] = lastRaw[i];
    learnCapture.state = LearnState::PREVIEW_READY;
    learnCapture.available = true;
    learnCapture.sequence++;
    learnCapture.pulseCount = count;
    learnCapture.durationUs = totalDuration;
    learnCapture.rssiDbm = frameRssi;
    learnCapture.capturedAtMs = millis();
    learnCapture.lastRejectReason = "";
    Serial.print("LEARN preview ready: "); Serial.print(count); Serial.println(" pulses");
  }
}

bool RadioManager::startReceive() {
  if (!initialized_) return false;
  detachInterrupt(digitalPinToInterrupt(OPENRF_CC1101_GDO0_PIN));
  pinMode(OPENRF_CC1101_GDO0_PIN, INPUT);
  lastError_ = cc1101.receiveDirectAsync();
  if (lastError_ != RADIOLIB_ERR_NONE) { mode_ = RadioMode::ERROR; return false; }
  noInterrupts();
  rxCount = 0; frameReady = false; captureEnabled = true;
  lastLevel = digitalRead(OPENRF_CC1101_GDO0_PIN); lastEdgeUs = micros();
  interrupts();
  framePeakRssi_ = -127.0F;
  attachInterrupt(digitalPinToInterrupt(OPENRF_CC1101_GDO0_PIN), gdo0ISR, CHANGE);
  mode_ = RadioMode::RX;
  return true;
}

bool RadioManager::stopReceive() {
  if (!initialized_) return false;
  detachInterrupt(digitalPinToInterrupt(OPENRF_CC1101_GDO0_PIN));
  noInterrupts(); captureEnabled = false; rxCount = 0; frameReady = false; interrupts();
  lastError_ = cc1101.standby();
  if (lastError_ != RADIOLIB_ERR_NONE) { mode_ = RadioMode::ERROR; return false; }
  mode_ = RadioMode::IDLE;
  return true;
}

void RadioManager::waitUs(uint32_t durationUs) {
  const uint32_t started = micros();
  while (static_cast<uint32_t>(micros() - started) < durationUs) yield();
}

bool RadioManager::sendRaw(const int16_t* pulses, uint16_t pulseCount, uint8_t repeats) {
  if (!initialized_ || !pulses || pulseCount == 0 || pulseCount > OPENRF_MAX_RAW_PULSES) return false;
  if (repeats < 1) repeats = 1;
  if (repeats > 10) repeats = 10;
  if (!stopReceive()) return false;
  mode_ = RadioMode::TX;
  lastError_ = cc1101.transmitDirectAsync();
  if (lastError_ != RADIOLIB_ERR_NONE) {
    diagnostics_.txErrors++;
    Serial.print("TX start failed, code: "); Serial.println(lastError_);
    startReceive();
    return false;
  }
  pinMode(OPENRF_CC1101_GDO0_PIN, OUTPUT);
  digitalWrite(OPENRF_CC1101_GDO0_PIN, LOW);
  delay(10);
  Serial.print("TX started: "); Serial.print(pulseCount);
  Serial.print(" pulses, repeats "); Serial.println(repeats);
  for (uint8_t rep = 0; rep < repeats; rep++) {
    for (uint16_t i = 0; i < pulseCount; i++) {
      const int16_t pulse = pulses[i];
      digitalWrite(OPENRF_CC1101_GDO0_PIN, pulse > 0 ? HIGH : LOW);
      waitUs(static_cast<uint32_t>(abs(static_cast<int32_t>(pulse))));
    }
    digitalWrite(OPENRF_CC1101_GDO0_PIN, LOW);
    waitUs(15000);
  }
  digitalWrite(OPENRF_CC1101_GDO0_PIN, LOW);
  diagnostics_.txCount++;
  Serial.println("TX completed");
  const bool rxRestored = startReceive();
  Serial.println(rxRestored ? "RAW receiver restored after TX" : "RX restore failed after TX");
  return rxRestored;
}

bool RadioManager::testSendLearnCapture(uint8_t repeats) {
  if (!learnCapture.available ||
      (learnCapture.state != LearnState::PREVIEW_READY &&
       learnCapture.state != LearnState::ACCEPTED_RAM)) return false;
  return sendRaw(learnRaw, learnCapture.pulseCount, repeats);
}

bool RadioManager::isInitialized() const { return initialized_; }
bool RadioManager::isReceiving() const { return initialized_ && mode_ == RadioMode::RX; }
float RadioManager::getRSSI() { return (!initialized_ || mode_ != RadioMode::RX) ? -127.0F : lastRssi_; }
float RadioManager::getFrequency() const { return frequencyMHz_; }
const char* RadioManager::getChipName() const { return "CC1101"; }
const char* RadioManager::getModeName() const {
  switch (mode_) {
    case RadioMode::IDLE: return "IDLE";
    case RadioMode::RX: return "RX";
    case RadioMode::TX: return "TX";
    case RadioMode::ERROR: return "ERROR";
    default: return "OFFLINE";
  }
}
int16_t RadioManager::getLastError() const { return lastError_; }
RawFrameInfo RadioManager::getLastFrameInfo() const { return lastFrame; }
uint16_t RadioManager::copyLastRaw(int16_t* destination, uint16_t capacity) const {
  if (!destination || capacity == 0 || !lastFrame.available) return 0;
  const uint16_t count = min(lastFrame.pulseCount, capacity);
  noInterrupts(); for (uint16_t i = 0; i < count; i++) destination[i] = lastRaw[i]; interrupts();
  return count;
}
bool RadioManager::hasRawFrame() const { return lastFrame.available; }

bool RadioManager::startLearning() {
  if (!initialized_ || mode_ != RadioMode::RX) return false;
  learnCapture = LearnCaptureInfo{};
  learnCapture.state = LearnState::WAITING_FOR_SIGNAL;
  learnCapture.noiseFloorDbm = cc1101.getRSSI();
  diagnostics_.lastNoiseFloorDbm = learnCapture.noiseFloorDbm;
  noInterrupts();
  rxCount = 0; frameReady = false; lastEdgeUs = micros();
  lastLevel = digitalRead(OPENRF_CC1101_GDO0_PIN);
  interrupts();
  Serial.print("LEARN started, noise floor: ");
  Serial.print(learnCapture.noiseFloorDbm, 1); Serial.println(" dBm");
  return true;
}

bool RadioManager::acceptLearnCapture() {
  if (learnCapture.state != LearnState::PREVIEW_READY || !learnCapture.available) return false;
  learnCapture.state = LearnState::ACCEPTED_RAM;
  Serial.println("LEARN capture accepted in RAM");
  return true;
}
bool RadioManager::discardLearnCapture() {
  if (learnCapture.state == LearnState::IDLE) return false;
  learnCapture = LearnCaptureInfo{};
  Serial.println("LEARN capture discarded");
  return true;
}
LearnCaptureInfo RadioManager::getLearnCaptureInfo() const { return learnCapture; }
uint16_t RadioManager::copyLearnRaw(int16_t* destination, uint16_t capacity) const {
  if (!destination || capacity == 0 || !learnCapture.available) return 0;
  const uint16_t count = min(learnCapture.pulseCount, capacity);
  noInterrupts(); for (uint16_t i = 0; i < count; i++) destination[i] = learnRaw[i]; interrupts();
  return count;
}
const char* RadioManager::getLearnStateName() const {
  switch (learnCapture.state) {
    case LearnState::WAITING_FOR_SIGNAL: return "WAITING_FOR_SIGNAL";
    case LearnState::PREVIEW_READY: return "PREVIEW_READY";
    case LearnState::ACCEPTED_RAM: return "ACCEPTED_RAM";
    default: return "IDLE";
  }
}
RadioDiagnostics RadioManager::getDiagnostics() const { return diagnostics_; }
