#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "hardware.h"
#include "dualcore.h"
#include "radio.h"
#include "raw_capture.h"
#include "protocol_engine.h"
#include "protocol_diagnostics.h"
#include "v2_authoritative_action.h"
#include "short_raw_candidate_route.h"
#include "analyzer.h"
#include "psram_buffers.h"
#include "config.h"
#include "universal_decoder.h"

namespace {

SemaphoreHandle_t radioMutex = nullptr;

class RadioGuard {
 public:
  RadioGuard() {
    if (radioMutex) xSemaphoreTakeRecursive(radioMutex, portMAX_DELAY);
  }
  ~RadioGuard() {
    if (radioMutex) xSemaphoreGiveRecursive(radioMutex);
  }
};

CC1101 cc1101Radio1 = new Module(
    OPENRF_CC1101_CS_PIN, OPENRF_CC1101_GDO0_PIN,
    RADIOLIB_NC, OPENRF_CC1101_GDO2_PIN);

CC1101 cc1101Radio2 = new Module(
    OPENRF_CC1101_2_CS_PIN, OPENRF_CC1101_2_GDO0_PIN,
    RADIOLIB_NC, OPENRF_CC1101_2_GDO2_PIN);

constexpr uint32_t RSSI_REFRESH_INTERVAL_MS = 5;

// Step 28: explicit per-radio capture profiles.
// These profiles intentionally preserve the already-tested behavior:
// Radio 1 / 433 MHz keeps conservative capture with NO short-frame recovery.
// Radio 2 / 868 MHz keeps the Step 14 recovery for genuine 6..19 pulse bursts.
// Future band-specific tuning now has one controlled place instead of scattered
// `if (radioId == ...)` branches.
struct RadioCaptureProfile {
  const char* name;
  uint32_t frameGapUs;
  uint16_t minCapturePulses;
  bool shortFrameRecovery;
  uint16_t minStalePartialPulses;
  uint32_t stalePartialTimeoutUs;
};

constexpr RadioCaptureProfile RADIO1_CAPTURE_PROFILE = {
    "433 conservative", 25000, 20, false, 20, 0};

constexpr RadioCaptureProfile RADIO2_CAPTURE_PROFILE = {
    "868 short-burst", 25000, 20, true, 6, 75000};

const RadioCaptureProfile& captureProfile(uint8_t radioId) {
  return radioId == 2 ? RADIO2_CAPTURE_PROFILE : RADIO1_CAPTURE_PROFILE;
}

V2LearnPayload v2LearnPayloadFromObservation(
    const ProtocolEngineObservation& observation) {
  V2LearnPayload payload;
  const NormalizedRfEvent& event = observation.normalizedEvent;
  if (!event.available || !v2AuthoritativeProtocolApproved(event.protocol)) {
    return payload;
  }

  payload.available = true;
  payload.protocolId = static_cast<uint16_t>(event.protocol);
  payload.code = event.code;
  payload.symbolCount = event.symbolCount;
  payload.repeats = event.repeats;

  uint32_t pulseLengthUs = 0;
  switch (event.protocol) {
    case ProtocolId::EV1527_PRINCETON:
      if (observation.ev1527DiagnosticsAvailable) {
        pulseLengthUs = observation.ev1527Diagnostics.estimatedBasePulseUs;
      }
      break;
    case ProtocolId::PT2262_TRI_STATE:
      if (observation.pt2262DiagnosticsAvailable) {
        pulseLengthUs = observation.pt2262Diagnostics.estimatedBasePulseUs;
      }
      break;
    case ProtocolId::NVKP01_KINETIC:
      // NVKP01 is RX-only and has no single TX base pulse requirement. Keep
      // timing metadata at 0; the V2 slot identity is protocol + normalized
      // code + learned radio/frequency.
      pulseLengthUs = 0;
      break;
    case ProtocolId::HT12E:
      if (observation.ht12eDiagnosticsAvailable) {
        pulseLengthUs = observation.ht12eDiagnostics.estimatedTUs;
      }
      break;
    default:
      break;
  }
  if (pulseLengthUs > UINT16_MAX) pulseLengthUs = UINT16_MAX;
  payload.pulseLengthUs = static_cast<uint16_t>(pulseLengthUs);
  return payload;
}

constexpr uint16_t MIN_VALID_PULSES = 30;
constexpr uint16_t MIN_MONITOR_PULSES = 60;
constexpr uint16_t MAX_VALID_PULSES = 500;
constexpr uint32_t MIN_FRAME_DURATION_US = 10000;
constexpr uint32_t MIN_MONITOR_DURATION_US = 80000;
constexpr uint32_t MAX_FRAME_DURATION_US = 400000;
constexpr uint32_t NOISE_US = 150;
constexpr float LEARN_RSSI_DELTA_DB = 6.0F;
constexpr uint16_t MAX_CLUSTER_COUNT = 12;
constexpr uint32_t TUNE_SESSION_DURATION_MS = 15UL * 60UL * 1000UL;

portMUX_TYPE rxMux1 = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE rxMux2 = portMUX_INITIALIZER_UNLOCKED;

struct RadioChannel {
  uint8_t id = 0;
  float defaultFrequencyMHz = 0.0F;
  // Current center frequency used for RX/analyzer operation. This may differ
  // from defaultFrequencyMHz after System frequency tuning.
  float frequencyMHz = 0.0F;
  CC1101* radio = nullptr;
  uint8_t gdo0Pin = 0;
  portMUX_TYPE* mux = nullptr;

  bool enabled = false;
  bool initialized = false;
  RadioMode mode = RadioMode::OFFLINE;
  int16_t lastError = 0;

  float lastRssi = -127.0F;
  float framePeakRssi = -127.0F;
  float learnNoiseFloor = -127.0F;
  float learnFrequencyMHz = 0.0F;  // RAW Learn frequency snapshot
  uint32_t tuneSessionExpiresAtMs = 0; // 0 = not in a System tuning session
  uint32_t lastRssiReadMs = 0;

  volatile int16_t rxPulses[OPENRF_ISR_CAPTURE_PULSES] = {0};
  volatile uint16_t rxCount = 0;
  volatile uint32_t lastEdgeUs = 0;
  volatile int lastLevel = LOW;
  volatile bool frameReady = false;
  volatile bool captureEnabled = false;

  volatile uint32_t totalEdges = 0;
  uint32_t rawCandidates = 0;
  uint32_t acceptedFrames = 0;
  uint32_t rejectedFrames = 0;
  volatile uint32_t ignoredGlitchEdges = 0;
  volatile uint32_t ignoredWhileFrameReady = 0;
  volatile uint32_t shortGapResets = 0;
  volatile uint32_t gapFinalizedFrames = 0;
  volatile uint32_t timeoutFinalizedFrames = 0;
  volatile uint32_t stalePartialFinalizedFrames = 0;
  volatile uint32_t bufferFullFrames = 0;
  volatile uint32_t mergedSameSignPulses = 0;

  // Last completed frame diagnostics; updated outside the ISR.
  uint16_t lastFinalizedPulses = 0;
  uint32_t lastFinalizedDurationUs = 0;
  uint32_t lastFinalizedAtMs = 0;
  String lastFinalizeReason;
  String lastValidationResult;
};

RadioChannel channels[2];

int16_t* lastRaw = nullptr;
RawFrameInfo lastFrame;
int16_t* learnRaw = nullptr;
LearnCaptureInfo learnCapture;

RadioChannel* channelById(uint8_t radioId) {
  if (radioId == 1) return &channels[0];
  if (radioId == 2) return &channels[1];
  return nullptr;
}

void configureChannels() {
  channels[0].id = 1;
  channels[0].defaultFrequencyMHz = OPENRF_RADIO1_DEFAULT_FREQUENCY_MHZ;
  // Step 26.2.1: restore a persisted TUNED frequency after reboot.
  // A restored session receives a fresh 15-minute safety window.
  channels[0].frequencyMHz = config.radio1FrequencyMhz;
  if (channels[0].frequencyMHz < 430.0F || channels[0].frequencyMHz > 440.0F)
    channels[0].frequencyMHz = channels[0].defaultFrequencyMHz;
  channels[0].radio = &cc1101Radio1;
  channels[0].gdo0Pin = OPENRF_CC1101_GDO0_PIN;
  channels[0].mux = &rxMux1;
  channels[0].enabled = config.radio1Enabled;
  if (fabsf(channels[0].frequencyMHz - channels[0].defaultFrequencyMHz) >
      OPENRF_FREQUENCY_TUNED_EPSILON_MHZ)
    channels[0].tuneSessionExpiresAtMs = millis() + TUNE_SESSION_DURATION_MS;

  channels[1].id = 2;
  channels[1].defaultFrequencyMHz = OPENRF_RADIO2_DEFAULT_FREQUENCY_MHZ;
  channels[1].frequencyMHz = config.radio2FrequencyMhz;
  if (channels[1].frequencyMHz < 867.0F || channels[1].frequencyMHz > 870.0F)
    channels[1].frequencyMHz = channels[1].defaultFrequencyMHz;
  channels[1].radio = &cc1101Radio2;
  channels[1].gdo0Pin = OPENRF_CC1101_2_GDO0_PIN;
  channels[1].mux = &rxMux2;
  channels[1].enabled = config.radio2Enabled;
  if (fabsf(channels[1].frequencyMHz - channels[1].defaultFrequencyMHz) >
      OPENRF_FREQUENCY_TUNED_EPSILON_MHZ)
    channels[1].tuneSessionExpiresAtMs = millis() + TUNE_SESSION_DURATION_MS;
}

void IRAM_ATTR captureEdge(RadioChannel* ch) {
  if (!ch || !ch->mux) return;
  const RadioCaptureProfile& profile = captureProfile(ch->id);

  portENTER_CRITICAL_ISR(ch->mux);

  if (!ch->captureEnabled) {
    portEXIT_CRITICAL_ISR(ch->mux);
    return;
  }

  ch->totalEdges++;

  const uint32_t now = micros();
  const uint32_t duration = now - ch->lastEdgeUs;
  const int level = digitalRead(ch->gdo0Pin);

  if (duration < NOISE_US) {
    ch->ignoredGlitchEdges++;
    portEXIT_CRITICAL_ISR(ch->mux);
    return;
  }

  if (ch->frameReady) {
    ch->ignoredWhileFrameReady++;
    portEXIT_CRITICAL_ISR(ch->mux);
    return;
  }

  ch->lastEdgeUs = now;

  if (duration > profile.frameGapUs) {
    if (ch->rxCount >= profile.minCapturePulses) {
      ch->frameReady = true;
      ch->gapFinalizedFrames++;
    } else {
      if (ch->rxCount > 0) ch->shortGapResets++;
      ch->rxCount = 0;
    }
    ch->lastLevel = level;
    portEXIT_CRITICAL_ISR(ch->mux);
    return;
  }

  if (ch->rxCount < OPENRF_ISR_CAPTURE_PULSES) {
    const int32_t signedDuration =
        ch->lastLevel == HIGH ? static_cast<int32_t>(duration)
                              : -static_cast<int32_t>(duration);

    if (ch->rxCount > 0 &&
        ((ch->rxPulses[ch->rxCount - 1] > 0) == (signedDuration > 0))) {
      int32_t merged =
          static_cast<int32_t>(ch->rxPulses[ch->rxCount - 1]) + signedDuration;
      if (merged > INT16_MAX) merged = INT16_MAX;
      if (merged < INT16_MIN) merged = INT16_MIN;
      ch->rxPulses[ch->rxCount - 1] = static_cast<int16_t>(merged);
      ch->mergedSameSignPulses++;
    } else {
      ch->rxPulses[ch->rxCount++] = static_cast<int16_t>(signedDuration);
    }

    if (ch->rxCount >= OPENRF_ISR_CAPTURE_PULSES) {
      ch->frameReady = true;
      ch->bufferFullFrames++;
    }
  }

  ch->lastLevel = level;
  portEXIT_CRITICAL_ISR(ch->mux);
}

void IRAM_ATTR gdo0ISR1() { captureEdge(&channels[0]); }
void IRAM_ATTR gdo0ISR2() { captureEdge(&channels[1]); }

bool startChannelReceive(RadioChannel& ch) {
  if (!ch.initialized || !ch.radio) return false;

  detachInterrupt(digitalPinToInterrupt(ch.gdo0Pin));
  pinMode(ch.gdo0Pin, INPUT);

  ch.lastError = ch.radio->receiveDirectAsync();
  if (ch.lastError != RADIOLIB_ERR_NONE) {
    ch.mode = RadioMode::ERROR;
    return false;
  }

  portENTER_CRITICAL(ch.mux);
  ch.rxCount = 0;
  ch.frameReady = false;
  ch.captureEnabled = true;
  ch.lastLevel = digitalRead(ch.gdo0Pin);
  ch.lastEdgeUs = micros();
  portEXIT_CRITICAL(ch.mux);

  ch.framePeakRssi = -127.0F;

  attachInterrupt(
      digitalPinToInterrupt(ch.gdo0Pin),
      ch.id == 1 ? gdo0ISR1 : gdo0ISR2,
      CHANGE);

  ch.mode = RadioMode::RX;
  return true;
}

bool stopChannelReceive(RadioChannel& ch) {
  if (!ch.initialized || !ch.radio) return false;

  detachInterrupt(digitalPinToInterrupt(ch.gdo0Pin));

  portENTER_CRITICAL(ch.mux);
  ch.captureEnabled = false;
  ch.rxCount = 0;
  ch.frameReady = false;
  portEXIT_CRITICAL(ch.mux);

  ch.lastError = ch.radio->standby();
  if (ch.lastError != RADIOLIB_ERR_NONE) {
    ch.mode = RadioMode::ERROR;
    return false;
  }

  ch.mode = RadioMode::IDLE;
  return true;
}

bool initializeChannel(RadioChannel& ch) {
  if (!ch.enabled) {
    ch.initialized = false;
    ch.mode = RadioMode::OFFLINE;
    return true;
  }

  Serial.print(F("CC1101 Radio "));
  Serial.print(ch.id);
  Serial.print(F(" initialization started @ "));
  Serial.print(ch.frequencyMHz, 3);
  Serial.println(F(" MHz"));

  ch.lastError = ch.radio->begin(
      ch.frequencyMHz,
      OPENRF_RADIO_BIT_RATE_KBPS,
      OPENRF_RADIO_FREQUENCY_DEVIATION_KHZ,
      OPENRF_RADIO_RX_BANDWIDTH_KHZ,
      OPENRF_RADIO_OUTPUT_POWER_DBM,
      OPENRF_RADIO_PREAMBLE_BITS);

  if (ch.lastError != RADIOLIB_ERR_NONE) {
    ch.mode = RadioMode::ERROR;
    ch.initialized = false;
    Serial.print(F("CC1101 Radio init failed, id="));
    Serial.print(ch.id);
    Serial.print(F(", code="));
    Serial.println(ch.lastError);
    return false;
  }

  ch.lastError = ch.radio->setOOK(true);
  if (ch.lastError != RADIOLIB_ERR_NONE) {
    ch.mode = RadioMode::ERROR;
    ch.initialized = false;
    return false;
  }

  ch.lastError = ch.radio->disableSyncWordFiltering();
  if (ch.lastError != RADIOLIB_ERR_NONE) {
    ch.mode = RadioMode::ERROR;
    ch.initialized = false;
    return false;
  }

  ch.initialized = true;
  ch.mode = RadioMode::IDLE;

  if (!startChannelReceive(ch)) {
    Serial.print(F("CC1101 Radio "));
    Serial.print(ch.id);
    Serial.println(F(" initialized, but RAW RX start failed"));
    return false;
  }

  Serial.print(F("CC1101 Radio "));
  Serial.print(ch.id);
  Serial.println(F(" initialized successfully"));
  return true;
}

// Step 34 FIX2: one common post-direct-TX recovery path for either CC1101.
// Radio 1 and Radio 2 are electrically identical CC1101 devices; only their
// pins/frequency/profile differ. Recovery therefore uses the same channel
// abstraction and the same known-good boot initialization path for both.
bool recoverChannelAfterDirectTx(RadioChannel& ch) {
  if (!ch.enabled || !ch.radio) return false;

  detachInterrupt(digitalPinToInterrupt(ch.gdo0Pin));
  pinMode(ch.gdo0Pin, INPUT);

  portENTER_CRITICAL(ch.mux);
  ch.captureEnabled = false;
  ch.rxCount = 0;
  ch.frameReady = false;
  portEXIT_CRITICAL(ch.mux);

  // A direct asynchronous TX changes the CC1101 direct-mode/GDO state. When a
  // simple RX restart does not recover on real hardware, reset the transceiver
  // and rebuild the exact same OOK/direct-RX configuration used at boot.
  ch.radio->reset();
  delay(2);

  ch.initialized = false;
  ch.mode = RadioMode::IDLE;
  ch.lastError = RADIOLIB_ERR_NONE;

  return initializeChannel(ch);
}

}  // namespace

RadioManager Radio;

bool RadioManager::begin() {
  lastRaw = psramRadioLastRaw();
  learnRaw = psramRadioLearnRaw();

  if (!lastRaw || !learnRaw) {
    Serial.println(F("Radio PSRAM working buffers unavailable"));
    return false;
  }

  if (!radioMutex) {
    radioMutex = xSemaphoreCreateRecursiveMutex();
    if (!radioMutex) {
      Serial.println(F("Radio mutex creation failed"));
      return false;
    }
  }

  RadioGuard guard;

  SPI.begin(
      OPENRF_CC1101_SCK_PIN,
      OPENRF_CC1101_MISO_PIN,
      OPENRF_CC1101_MOSI_PIN,
      OPENRF_CC1101_CS_PIN);

  digitalWrite(OPENRF_CC1101_CS_PIN, HIGH);
  digitalWrite(OPENRF_CC1101_2_CS_PIN, HIGH);

  configureChannels();

  lastFrame = RawFrameInfo{};
  learnCapture = LearnCaptureInfo{};
  diagnostics_ = RadioDiagnostics{};
  analyzerReset();
  lastError_ = 0;

  bool anyEnabled = false;
  bool allEnabledInitialized = true;

  for (auto& ch : channels) {
    if (!ch.enabled) continue;
    anyEnabled = true;

    if (!initializeChannel(ch)) {
      allEnabledInitialized = false;
      lastError_ = ch.lastError;
    }
  }

  if (!anyEnabled) {
    Serial.println(F("No CC1101 radio enabled in System"));
    return false;
  }

  Serial.print(F("RF engine active: Radio1="));
  Serial.print(channels[0].initialized ? F("RX") : F("OFF"));
  Serial.print(F(", Radio2="));
  Serial.println(channels[1].initialized ? F("RX") : F("OFF"));
  Serial.print(F("RF capture profiles: R1="));
  Serial.print(RADIO1_CAPTURE_PROFILE.name);
  Serial.print(F(", R2="));
  Serial.println(RADIO2_CAPTURE_PROFILE.name);

  return allEnabledInitialized;
}

void RadioManager::loop() {
  RadioGuard guard;

  const uint32_t nowMs = millis();

  for (auto& ch : channels) {
    if (!ch.initialized) continue;

    // Step 26.2: automatically end a forgotten System tuning session.
    if (ch.tuneSessionExpiresAtMs != 0 &&
        static_cast<int32_t>(nowMs - ch.tuneSessionExpiresAtMs) >= 0) {
      const bool wasReceiving = ch.mode == RadioMode::RX;
      if (!wasReceiving || stopChannelReceive(ch)) {
        const int16_t err = ch.radio->setFrequency(ch.defaultFrequencyMHz);
        if (err == RADIOLIB_ERR_NONE) {
          ch.frequencyMHz = ch.defaultFrequencyMHz;
          ch.tuneSessionExpiresAtMs = 0;
          if (ch.id == 1) config.radio1FrequencyMhz = ch.defaultFrequencyMHz;
          else if (ch.id == 2) config.radio2FrequencyMhz = ch.defaultFrequencyMHz;
          configSave();
          Serial.print(F("CC1101 Radio "));
          Serial.print(ch.id);
          Serial.println(F(" tuning session expired -> Default frequency restored"));
        } else {
          ch.lastError = err;
        }
        if (wasReceiving) startChannelReceive(ch);
      }
    }

    if (ch.mode != RadioMode::RX) continue;

    if (nowMs - ch.lastRssiReadMs >= RSSI_REFRESH_INTERVAL_MS) {
      ch.lastRssiReadMs = nowMs;
      ch.lastRssi = ch.radio->getRSSI();

      uint16_t activeCount;
      portENTER_CRITICAL(ch.mux);
      activeCount = ch.rxCount;
      portEXIT_CRITICAL(ch.mux);

      if (activeCount > 0 && ch.lastRssi > ch.framePeakRssi) {
        ch.framePeakRssi = ch.lastRssi;
      }
    }

    bool readyCopy;
    uint16_t countCopy;
    uint32_t edgeCopy;

    portENTER_CRITICAL(ch.mux);
    readyCopy = ch.frameReady;
    countCopy = ch.rxCount;
    edgeCopy = ch.lastEdgeUs;
    portEXIT_CRITICAL(ch.mux);

    if (!readyCopy &&
        countCopy >= captureProfile(ch.id).minCapturePulses &&
        static_cast<uint32_t>(micros() - edgeCopy) >
            captureProfile(ch.id).frameGapUs) {
      portENTER_CRITICAL(ch.mux);
      if (!ch.frameReady &&
          ch.rxCount >= captureProfile(ch.id).minCapturePulses &&
          static_cast<uint32_t>(micros() - ch.lastEdgeUs) >
              captureProfile(ch.id).frameGapUs) {
        ch.frameReady = true;
        ch.timeoutFinalizedFrames++;
      }
      readyCopy = ch.frameReady;
      portEXIT_CRITICAL(ch.mux);
    }

    // Step 28: band-specific short-frame recovery is profile-driven.
    // Radio 1 profile disables it; Radio 2 profile enables the tested
    // 6..19 pulse / 75 ms recovery path.
    const RadioCaptureProfile& profile = captureProfile(ch.id);
    if (profile.shortFrameRecovery &&
        !readyCopy &&
        countCopy >= profile.minStalePartialPulses &&
        countCopy < profile.minCapturePulses &&
        static_cast<uint32_t>(micros() - edgeCopy) >
            profile.stalePartialTimeoutUs) {
      portENTER_CRITICAL(ch.mux);
      if (!ch.frameReady &&
          ch.rxCount >= profile.minStalePartialPulses &&
          ch.rxCount < profile.minCapturePulses &&
          static_cast<uint32_t>(micros() - ch.lastEdgeUs) >
              profile.stalePartialTimeoutUs) {
        ch.frameReady = true;
        ch.stalePartialFinalizedFrames++;
      }
      readyCopy = ch.frameReady;
      portEXIT_CRITICAL(ch.mux);
    }

    if (readyCopy) finalizeFrame(ch.id);
  }
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


bool RadioManager::validateLearnSignal(float frameRssi, float noiseFloor,
                                       String& reason) const {
  if (noiseFloor <= -120.0F) return true;
  if (frameRssi < noiseFloor + LEARN_RSSI_DELTA_DB) {
    reason = "rssi_not_above_noise_floor";
    return false;
  }
  return true;
}

void RadioManager::finalizeFrame(uint8_t radioId) {
  RadioChannel* ch = channelById(radioId);
  if (!ch || !ch->initialized) return;

  // Snapshot the three finalization counters before clearing frameReady.
  // Whichever counter advanced most recently identifies how this frame ended.
  const uint32_t gapFinalizeCount = ch->gapFinalizedFrames;
  const uint32_t timeoutFinalizeCount = ch->timeoutFinalizedFrames;
  const uint32_t stalePartialFinalizeCount =
      ch->stalePartialFinalizedFrames;
  const uint32_t bufferFinalizeCount = ch->bufferFullFrames;

  uint16_t count = 0;

  portENTER_CRITICAL(ch->mux);
  count = ch->rxCount;
  if (count > OPENRF_MAX_RAW_PULSES) count = OPENRF_MAX_RAW_PULSES;

  for (uint16_t i = 0; i < count; i++) {
    lastRaw[i] = ch->rxPulses[i];
  }

  ch->rxCount = 0;
  ch->frameReady = false;
  ch->lastEdgeUs = micros();
  ch->lastLevel = digitalRead(ch->gdo0Pin);
  portEXIT_CRITICAL(ch->mux);

  uint32_t totalDuration = 0;
  for (uint16_t i = 0; i < count; i++) {
    totalDuration +=
        static_cast<uint32_t>(abs(static_cast<int32_t>(lastRaw[i])));
  }

  static uint32_t prevGapFinalize[2] = {0, 0};
  static uint32_t prevTimeoutFinalize[2] = {0, 0};
  static uint32_t prevStalePartialFinalize[2] = {0, 0};
  static uint32_t prevBufferFinalize[2] = {0, 0};
  const uint8_t diagIndex = ch->id == 2 ? 1 : 0;

  String finalizeReason = "unknown";
  if (bufferFinalizeCount != prevBufferFinalize[diagIndex]) {
    finalizeReason = "buffer_full";
  } else if (stalePartialFinalizeCount !=
             prevStalePartialFinalize[diagIndex]) {
    finalizeReason = "stale_partial";
  } else if (timeoutFinalizeCount != prevTimeoutFinalize[diagIndex]) {
    finalizeReason = "timeout_gap";
  } else if (gapFinalizeCount != prevGapFinalize[diagIndex]) {
    finalizeReason = "edge_gap";
  }

  prevGapFinalize[diagIndex] = gapFinalizeCount;
  prevTimeoutFinalize[diagIndex] = timeoutFinalizeCount;
  prevStalePartialFinalize[diagIndex] = stalePartialFinalizeCount;
  prevBufferFinalize[diagIndex] = bufferFinalizeCount;

  ch->lastFinalizedPulses = count;
  ch->lastFinalizedDurationUs = totalDuration;
  ch->lastFinalizedAtMs = millis();
  ch->lastFinalizeReason = finalizeReason;

  const float sampledRssi = ch->radio->getRSSI();
  if (sampledRssi > ch->framePeakRssi) ch->framePeakRssi = sampledRssi;

  const float frameRssi = ch->framePeakRssi;
  ch->framePeakRssi = -127.0F;

  // Step 29.1: expose the complete finalized capture through one immutable,
  // zero-copy view. This is the future Protocol Engine hand-off boundary. The
  // stable Step 28 processing below remains authoritative and consumes exactly
  // the same pulse buffer and metadata as before.
  const RawCapture rawCapture(
      lastRaw, count, totalDuration, frameRssi, ch->frequencyMHz, ch->id,
      ch->lastFinalizedAtMs);

  // Step 39.2: the V2 Known Protocol Engine is now the only protocol RX
  // decision path. UNKNOWN/AMBIGUOUS captures remain non-actionable and are
  // never passed to the retired legacy fallback decoder.
  const ProtocolEngineObservation v2Observation =
      protocolEngineObserve(rawCapture);

  diagnostics_.rawCandidates++;
  ch->rawCandidates++;

  diagnostics_.ignoredGlitchEdges =
      channels[0].ignoredGlitchEdges + channels[1].ignoredGlitchEdges;
  diagnostics_.gapFinalizedFrames =
      channels[0].gapFinalizedFrames + channels[1].gapFinalizedFrames;
  diagnostics_.timeoutFinalizedFrames =
      channels[0].timeoutFinalizedFrames + channels[1].timeoutFinalizedFrames;
  diagnostics_.bufferFullFrames =
      channels[0].bufferFullFrames + channels[1].bufferFullFrames;
  diagnostics_.mergedSameSignPulses =
      channels[0].mergedSameSignPulses + channels[1].mergedSameSignPulses;

  String rejectReason;
  const bool learningNow =
      learnCapture.state == LearnState::WAITING_FOR_SIGNAL;

  bool frameValid =
      validateFrame(lastRaw, count, totalDuration, rejectReason);

  // FIX5: RAW Learn accepts a base-valid short signal, but normal monitoring
  // deliberately classifies the same signal as background.  Remember the
  // narrow V2-UNKNOWN case before applying that monitor-only filter so it can
  // later be offered exclusively to already learned RAW slots.
  const bool shortRawMatchCandidate = shortRawMatchCandidateEligible(
      frameValid,
      learningNow,
      v2Observation.decision == ProtocolEngineDecisionState::UNKNOWN,
      count,
      totalDuration);

  ch->lastValidationResult =
      frameValid ? String("accepted") : rejectReason;

  const bool recognizedProtocol = v2Observation.normalizedEvent.available;

  if (frameValid &&
      !learningNow &&
      !recognizedProtocol &&
      count < MIN_MONITOR_PULSES &&
      totalDuration < MIN_MONITOR_DURATION_US) {
    frameValid = false;
    rejectReason = "background_short_frame";
    ch->lastValidationResult = rejectReason;
    diagnostics_.backgroundFilteredFrames++;
  }

  protocolDiagnosticsRecord(rawCapture, v2Observation, frameValid);

  if (config.analyzerDeveloperMode) {
    analyzerRecordCandidate(
        lastRaw,
        count,
        totalDuration,
        ch->frequencyMHz,
        ch->id,
        frameRssi,
        frameValid ? String("accepted") : rejectReason
    );
  }

  if (!frameValid) {
    if (config.analyzerDeveloperMode) {
      if (analyzerRssiPasses(ch->id, frameRssi)) {
        if (config.analyzerShowRejected) {
          analyzerConsiderRejected(
              lastRaw, count, totalDuration, ch->frequencyMHz,
              ch->id, frameRssi, rejectReason);
        }
      } else {
        analyzerRecordWeakRssi(ch->id, frameRssi);
      }
    }

    diagnostics_.rejectedFrames++;
    ch->rejectedFrames++;
    diagnostics_.lastRejectReason = rejectReason;

    if (learningNow) {
      learnCapture.rejectedDuringLearn++;
      learnCapture.lastRejectReason = rejectReason;
    }

    if ((diagnostics_.rejectedFrames % 10) == 1) {
      Serial.print(F("RAW rejected R"));
      Serial.print(ch->id);
      Serial.print(F(": "));
      Serial.print(rejectReason);
      Serial.print(F(", "));
      Serial.print(count);
      Serial.print(F(" pulses, "));
      Serial.print(totalDuration);
      Serial.println(F(" us"));
    }

    // Do not restore the rejected frame to normal RX, MQTT telemetry, RX Slot
    // handling or protocol actions.  A dedicated event type lets Core 0 offer
    // only a base-valid, V2-UNKNOWN short frame to the conservative Learned
    // RAW matcher.  AMBIGUOUS and KNOWN captures remain ineligible.
    if (shortRawMatchCandidate && rejectReason == "background_short_frame") {
      rfEventPublishFrame(
          RFEventType::RAW_MATCH_CANDIDATE,
          diagnostics_.rawCandidates,
          lastRaw,
          count,
          totalDuration,
          frameRssi,
          ch->frequencyMHz,
          ch->id,
          ch->lastFinalizedAtMs,
          false,
          nullptr,
          nullptr,
          true);
    }
    return;
  }

  if (config.analyzerDeveloperMode) {
    if (analyzerRssiPasses(ch->id, frameRssi)) {
      analyzerProcess(
          lastRaw, count, totalDuration, ch->frequencyMHz,
          ch->id, frameRssi, true, "accepted", &v2Observation);
    } else {
      analyzerRecordWeakRssi(ch->id, frameRssi);
    }
  }

  diagnostics_.acceptedFrames++;
  ch->acceptedFrames++;

  lastFrame.available = true;
  lastFrame.sequence++;
  lastFrame.pulseCount = count;
  lastFrame.durationUs = totalDuration;
  lastFrame.rssiDbm = frameRssi;
  lastFrame.frequencyMHz = ch->frequencyMHz;
  lastFrame.radioId = ch->id;
  lastFrame.receivedAtMs = millis();

  // Step 39.2: V2-only routing is decided only after frame validation accepts
  // the capture. KNOWN captures may emit; UNKNOWN/AMBIGUOUS are explicitly
  // non-actionable and never invoke the legacy protocol decoder.
  V2AuthoritativeRouteDecision v2Route;
  if (!learningNow) {
    v2Route =
        v2AuthoritativeActionRoute(v2Observation.normalizedEvent);
  }

  V2ActionPayload v2Action;
  const V2ActionPayload* v2ActionPtr = nullptr;
  if (v2Route.emitV2Action && v2Route.event.available) {
    v2Action.available = true;
    v2Action.protocolId =
        static_cast<uint16_t>(v2Route.event.protocol);
    v2Action.code = v2Route.event.code;
    v2Action.symbolCount = v2Route.event.symbolCount;
    v2Action.repeats = v2Route.event.repeats;
    v2ActionPtr = &v2Action;
  }

  // Step 39.2: the Core 0 legacy protocol action matcher is retired. Every
  // accepted RX frame carries raw telemetry, while protocol actions arrive
  // only through the optional V2ActionPayload below.
  const bool legacyActionAllowedForFrame = false;

  // Step 40: Learned RAW is the explicit fallback only for a true V2 UNKNOWN.
  // KNOWN captures remain owned by the protocol path even when burst dedup
  // suppresses their action, and AMBIGUOUS captures stay non-actionable.
  const bool rawMatchEligibleForFrame =
      !learningNow &&
      v2Observation.decision == ProtocolEngineDecisionState::UNKNOWN;

  rfEventPublishFrame(
      RFEventType::RX_FRAME,
      lastFrame.sequence,
      lastRaw,
      count,
      totalDuration,
      frameRssi,
      ch->frequencyMHz,
      ch->id,
      lastFrame.receivedAtMs,
      legacyActionAllowedForFrame,
      v2ActionPtr,
      nullptr,
      rawMatchEligibleForFrame);

  Serial.print(F("RAW frame R"));
  Serial.print(ch->id);
  Serial.print(F(" #"));
  Serial.print(lastFrame.sequence);
  Serial.print(F(" accepted: "));
  Serial.print(count);
  Serial.print(F(" pulses, "));
  Serial.print(totalDuration);
  Serial.print(F(" us, RSSI "));
  Serial.print(frameRssi, 1);
  Serial.println(F(" dBm"));

  if (learningNow) {
    if (!validateLearnSignal(
            frameRssi, ch->learnNoiseFloor, rejectReason)) {
      learnCapture.rejectedDuringLearn++;
      learnCapture.lastRejectReason = rejectReason;
      Serial.print(F("LEARN rejected R"));
      Serial.print(ch->id);
      Serial.print(F(": "));
      Serial.println(rejectReason);
      return;
    }

    for (uint16_t i = 0; i < count; i++) {
      learnRaw[i] = lastRaw[i];
    }

    learnCapture.state = LearnState::PREVIEW_READY;
    learnCapture.available = true;
    learnCapture.sequence++;
    learnCapture.pulseCount = count;
    learnCapture.durationUs = totalDuration;
    learnCapture.rssiDbm = frameRssi;
    learnCapture.noiseFloorDbm = ch->learnNoiseFloor;
    // Step 26.1: bind RAW metadata to the frequency present when this Learn
    // session started. A later retune must not leak into the pending capture.
    learnCapture.frequencyMHz = ch->learnFrequencyMHz > 0.0F
                                    ? ch->learnFrequencyMHz
                                    : ch->frequencyMHz;
    learnCapture.radioId = ch->id;
    learnCapture.capturedAtMs = millis();
    learnCapture.lastRejectReason = "";

    // Step 39.2: RX Slot Learn is V2-only. KNOWN captures carry the normalized
    // learn payload; UNKNOWN/AMBIGUOUS captures carry no payload and are
    // reported as unsupported until Step 40 adds Learned RAW matching.
    const V2LearnPayload v2Learn =
        v2LearnPayloadFromObservation(v2Observation);
    const V2LearnPayload* const v2LearnPtr =
        v2Learn.available ? &v2Learn : nullptr;

    rfEventPublishFrame(
        RFEventType::LEARN_PREVIEW,
        learnCapture.sequence,
        learnRaw,
        count,
        totalDuration,
        frameRssi,
        ch->frequencyMHz,
        ch->id,
        learnCapture.capturedAtMs,
        false,
        nullptr,
        v2LearnPtr);

    Serial.print(F("LEARN preview ready from Radio "));
    Serial.println(ch->id);
  }
}

bool RadioManager::startReceive() {
  RadioGuard guard;

  bool any = false;
  bool ok = true;

  for (auto& ch : channels) {
    if (!ch.initialized) continue;
    any = true;

    if (ch.mode != RadioMode::RX && !startChannelReceive(ch)) {
      ok = false;
      lastError_ = ch.lastError;
    }
  }

  return any && ok;
}

bool RadioManager::stopReceive() {
  RadioGuard guard;

  bool any = false;
  bool ok = true;

  for (auto& ch : channels) {
    if (!ch.initialized) continue;
    any = true;

    if (ch.mode == RadioMode::RX && !stopChannelReceive(ch)) {
      ok = false;
      lastError_ = ch.lastError;
    }
  }

  return any && ok;
}

void RadioManager::waitUs(uint32_t durationUs) {
  const uint32_t started = micros();
  while (static_cast<uint32_t>(micros() - started) < durationUs) {
    yield();
  }
}

bool RadioManager::sendRaw(const int16_t* pulses, uint16_t pulseCount,
                           uint8_t repeats, float frequencyMHz) {
  RadioGuard guard;

  if (!pulses || pulseCount == 0 ||
      pulseCount > OPENRF_MAX_RAW_PULSES) {
    return false;
  }

  const uint8_t targetId = frequencyMHz >= 700.0F ? 2 : 1;
  RadioChannel* target = channelById(targetId);

  if (!target || !target->initialized || !target->radio) {
    return false;
  }

  if (repeats < 1) repeats = 1;
  if (repeats > 10) repeats = 10;

  // Pause both receivers during TX to prevent the other CC1101 from capturing
  // OpenRF's own transmission.
  bool wasReceiving[2] = {
      channels[0].initialized && channels[0].mode == RadioMode::RX,
      channels[1].initialized && channels[1].mode == RadioMode::RX
  };

  for (auto& ch : channels) {
    if (ch.initialized && ch.mode == RadioMode::RX) {
      stopChannelReceive(ch);
    }
  }

  target->mode = RadioMode::TX;
  target->lastError = target->radio->transmitDirectAsync();

  if (target->lastError != RADIOLIB_ERR_NONE) {
    diagnostics_.txErrors++;
    lastError_ = target->lastError;

    for (uint8_t i = 0; i < 2; i++) {
      if (wasReceiving[i]) startChannelReceive(channels[i]);
    }
    return false;
  }

  pinMode(target->gdo0Pin, OUTPUT);
  digitalWrite(target->gdo0Pin, LOW);
  delay(10);

  for (uint8_t rep = 0; rep < repeats; rep++) {
    for (uint16_t i = 0; i < pulseCount; i++) {
      const int16_t pulse = pulses[i];
      digitalWrite(target->gdo0Pin, pulse > 0 ? HIGH : LOW);
      waitUs(static_cast<uint32_t>(
          abs(static_cast<int32_t>(pulse))));
    }

    digitalWrite(target->gdo0Pin, LOW);
    waitUs(15000);
  }

  digitalWrite(target->gdo0Pin, LOW);
  diagnostics_.txCount++;

  bool restored = true;
  for (uint8_t i = 0; i < 2; i++) {
    if (wasReceiving[i] && !startChannelReceive(channels[i])) {
      restored = false;
      lastError_ = channels[i].lastError;
    }
  }

  return restored;
}

bool RadioManager::sendRawTuned(const int16_t* pulses, uint16_t pulseCount,
                                uint8_t repeats, uint8_t radioId,
                                float frequencyMHz) {
  RadioGuard guard;

  if (!pulses || pulseCount == 0 || pulseCount > OPENRF_MAX_RAW_PULSES ||
      (radioId != 1 && radioId != 2)) return false;

  RadioChannel* target = channelById(radioId);
  if (!target || !target->initialized || !target->radio) return false;

  const float minFrequency = radioId == 1 ? 430.0F : 867.0F;
  const float maxFrequency = radioId == 1 ? 440.0F : 870.0F;
  if (frequencyMHz < minFrequency || frequencyMHz > maxFrequency) return false;

  repeats = constrain(repeats, static_cast<uint8_t>(1), static_cast<uint8_t>(10));

  const float restoreFrequencyMHz = target->frequencyMHz;
  const bool wasReceiving[2] = {
      channels[0].initialized && channels[0].mode == RadioMode::RX,
      channels[1].initialized && channels[1].mode == RadioMode::RX
  };

  // Pause every active receiver so the other CC1101 cannot capture our own TX.
  // This is identical for 433 and 868 MHz; only channel pin/frequency metadata
  // differs.
  bool pauseOk = true;
  for (auto& ch : channels) {
    if (ch.initialized && ch.mode == RadioMode::RX && !stopChannelReceive(ch)) {
      pauseOk = false;
      lastError_ = ch.lastError;
    }
  }

  bool txOk = pauseOk;
  if (txOk) {
    target->lastError = target->radio->setFrequency(frequencyMHz);
    if (target->lastError != RADIOLIB_ERR_NONE) {
      txOk = false;
      lastError_ = target->lastError;
    }
  }

  if (txOk) {
    target->mode = RadioMode::TX;
    target->lastError = target->radio->transmitDirectAsync();
    if (target->lastError != RADIOLIB_ERR_NONE) {
      txOk = false;
      lastError_ = target->lastError;
      diagnostics_.txErrors++;
    }
  }

  if (txOk) {
    pinMode(target->gdo0Pin, OUTPUT);
    digitalWrite(target->gdo0Pin, LOW);
    delay(10);

    for (uint8_t rep = 0; rep < repeats; rep++) {
      for (uint16_t i = 0; i < pulseCount; i++) {
        const int16_t pulse = pulses[i];
        digitalWrite(target->gdo0Pin, pulse > 0 ? HIGH : LOW);
        waitUs(static_cast<uint32_t>(abs(static_cast<int32_t>(pulse))));
      }
      // Protocol encoders already carry their own sync/gap timing.
      digitalWrite(target->gdo0Pin, LOW);
    }

    digitalWrite(target->gdo0Pin, LOW);
    diagnostics_.txCount++;
  }

  // Always release the direct-TX data pin before any CC1101 recovery.
  pinMode(target->gdo0Pin, INPUT);

  // FIX2 deliberately avoids maintaining a second, subtly different post-TX
  // state machine. The TX target is rebuilt through exactly the same
  // initializeChannel() path that is known to work at boot. The non-target
  // receiver only needs its normal receiveDirectAsync() restart because its
  // CC1101 never entered direct TX.
  bool restoreOk = true;
  for (uint8_t i = 0; i < 2; i++) {
    if (!wasReceiving[i]) continue;

    bool channelOk = false;
    if (i == static_cast<uint8_t>(radioId - 1)) {
      Serial.print(F("Protocol TX restore: hard recovery Radio "));
      Serial.println(i + 1);
      channelOk = recoverChannelAfterDirectTx(channels[i]);
    } else {
      channelOk = startChannelReceive(channels[i]);
      if (!channelOk) {
        // Keep recovery symmetrical: if even the paused companion does not
        // restart cleanly, use the same full boot initialization path.
        Serial.print(F("Protocol TX restore: companion recovery Radio "));
        Serial.println(i + 1);
        channelOk = recoverChannelAfterDirectTx(channels[i]);
      }
    }

    if (!channelOk) {
      restoreOk = false;
      lastError_ = channels[i].lastError;
      diagnostics_.txErrors++;
      Serial.print(F("Protocol TX restore FAILED on Radio "));
      Serial.print(i + 1);
      Serial.print(F(", code="));
      Serial.println(channels[i].lastError);
    } else {
      Serial.print(F("Protocol TX restore OK on Radio "));
      Serial.println(i + 1);
    }
  }

  // If the target was not receiving before TX, restore its configured
  // operating frequency but do not invent a new RX session.
  if (!wasReceiving[radioId - 1] && target->initialized) {
    const int16_t standbyError = target->radio->standby();
    const int16_t frequencyError =
        standbyError == RADIOLIB_ERR_NONE
            ? target->radio->setFrequency(restoreFrequencyMHz)
            : standbyError;
    if (frequencyError != RADIOLIB_ERR_NONE) {
      restoreOk = false;
      lastError_ = frequencyError;
    } else {
      target->mode = RadioMode::IDLE;
    }
  }

  // Final software-state verification. A successful user transmission must not
  // be reported until every receiver that was active beforehand is back in RX.
  for (uint8_t i = 0; i < 2; i++) {
    if (wasReceiving[i] &&
        (!channels[i].initialized || channels[i].mode != RadioMode::RX)) {
      restoreOk = false;
    }
  }

  Serial.print(F("Protocol TX R"));
  Serial.print(radioId);
  Serial.print(F(" @ "));
  Serial.print(frequencyMHz, 4);
  Serial.print(F(" MHz -> restore "));
  Serial.print(restoreFrequencyMHz, 4);
  Serial.print(F(" MHz, TX="));
  Serial.print(txOk ? F("OK") : F("FAILED"));
  Serial.print(F(", RX restore="));
  Serial.println(restoreOk ? F("OK") : F("FAILED"));

  if (txOk && restoreOk) lastError_ = RADIOLIB_ERR_NONE;
  return txOk && restoreOk;
}

bool RadioManager::testSendLearnCapture(uint8_t repeats) {
  RadioGuard guard;

  if (!learnCapture.available ||
      (learnCapture.state != LearnState::PREVIEW_READY &&
       learnCapture.state != LearnState::ACCEPTED_RAM)) {
    return false;
  }

  return sendRaw(
      learnRaw,
      learnCapture.pulseCount,
      repeats,
      learnCapture.frequencyMHz);
}

bool RadioManager::isInitialized() const {
  RadioGuard guard;
  return channels[0].initialized || channels[1].initialized;
}

bool RadioManager::isReceiving() const {
  RadioGuard guard;
  return (channels[0].initialized && channels[0].mode == RadioMode::RX) ||
         (channels[1].initialized && channels[1].mode == RadioMode::RX);
}

bool RadioManager::isRadioActive(uint8_t radioId) const {
  RadioGuard guard;
  RadioChannel* ch = channelById(radioId);
  return ch && ch->initialized && ch->mode == RadioMode::RX;
}

float RadioManager::getRSSI() {
  RadioGuard guard;

  float best = -127.0F;
  bool found = false;

  for (const auto& ch : channels) {
    if (!ch.initialized || ch.mode != RadioMode::RX) continue;
    if (!found || ch.lastRssi > best) best = ch.lastRssi;
    found = true;
  }

  return found ? best : -127.0F;
}

float RadioManager::getRadioRSSI(uint8_t radioId) {
  RadioGuard guard;
  RadioChannel* ch = channelById(radioId);

  if (!ch || !ch->initialized || ch->mode != RadioMode::RX) {
    return -127.0F;
  }

  return ch->lastRssi;
}

float RadioManager::getFrequency() const {
  RadioGuard guard;

  if (lastFrame.available) return lastFrame.frequencyMHz;

  const bool r1 = channels[0].initialized && channels[0].mode == RadioMode::RX;
  const bool r2 = channels[1].initialized && channels[1].mode == RadioMode::RX;

  if (r1 && !r2) return channels[0].frequencyMHz;
  if (r2 && !r1) return channels[1].frequencyMHz;
  return 0.0F;
}

uint8_t RadioManager::getActiveRadioId() const {
  RadioGuard guard;

  const bool r1 = channels[0].initialized && channels[0].mode == RadioMode::RX;
  const bool r2 = channels[1].initialized && channels[1].mode == RadioMode::RX;

  if (r1 && !r2) return 1;
  if (r2 && !r1) return 2;
  return 0;
}

const char* RadioManager::getChipName() const {
  return "CC1101";
}

const char* RadioManager::getModeName() const {
  RadioGuard guard;

  const bool r1 = channels[0].initialized && channels[0].mode == RadioMode::RX;
  const bool r2 = channels[1].initialized && channels[1].mode == RadioMode::RX;

  if (r1 && r2) return "DUAL RX";
  if (r1 || r2) return "RX";

  if ((channels[0].initialized && channels[0].mode == RadioMode::ERROR) ||
      (channels[1].initialized && channels[1].mode == RadioMode::ERROR)) {
    return "ERROR";
  }

  if (channels[0].initialized || channels[1].initialized) return "IDLE";
  return "OFFLINE";
}

int16_t RadioManager::getLastError() const {
  RadioGuard guard;
  return lastError_;
}

RawFrameInfo RadioManager::getLastFrameInfo() const {
  RadioGuard guard;
  return lastFrame;
}

uint16_t RadioManager::copyLastRaw(
    int16_t* destination, uint16_t capacity) const {
  RadioGuard guard;

  if (!destination || capacity == 0 || !lastFrame.available) return 0;

  const uint16_t count = min(lastFrame.pulseCount, capacity);
  for (uint16_t i = 0; i < count; i++) {
    destination[i] = lastRaw[i];
  }
  return count;
}

bool RadioManager::hasRawFrame() const {
  RadioGuard guard;
  return lastFrame.available;
}

bool RadioManager::startLearning() {
  RadioGuard guard;

  if (!isReceiving()) return false;

  learnCapture = LearnCaptureInfo{};
  learnCapture.state = LearnState::WAITING_FOR_SIGNAL;

  float bestNoise = -127.0F;

  for (auto& ch : channels) {
    if (!ch.initialized) continue;

    if (ch.mode != RadioMode::RX) continue;

    // Step 26.1: freeze the operating frequency for this Learn session.
    // This prevents a previous/parallel tuning state from changing slot metadata
    // after Learn has already started.
    ch.learnFrequencyMHz = ch.frequencyMHz;
    ch.learnNoiseFloor = ch.radio->getRSSI();
    if (ch.learnNoiseFloor > bestNoise) bestNoise = ch.learnNoiseFloor;

    portENTER_CRITICAL(ch.mux);
    ch.rxCount = 0;
    ch.frameReady = false;
    ch.lastEdgeUs = micros();
    ch.lastLevel = digitalRead(ch.gdo0Pin);
    portEXIT_CRITICAL(ch.mux);
  }

  learnCapture.noiseFloorDbm = bestNoise;
  diagnostics_.lastNoiseFloorDbm = bestNoise;

  Serial.print(F("LEARN started, active radios="));
  Serial.print(isRadioActive(1) ? F("433 ") : F(""));
  Serial.println(isRadioActive(2) ? F("868") : F(""));

  return true;
}

bool RadioManager::acceptLearnCapture() {
  RadioGuard guard;

  if (learnCapture.state != LearnState::PREVIEW_READY ||
      !learnCapture.available) {
    return false;
  }

  learnCapture.state = LearnState::ACCEPTED_RAM;
  return true;
}

bool RadioManager::discardLearnCapture() {
  RadioGuard guard;

  if (learnCapture.state == LearnState::IDLE) return false;

  learnCapture = LearnCaptureInfo{};
  return true;
}

LearnCaptureInfo RadioManager::getLearnCaptureInfo() const {
  RadioGuard guard;
  return learnCapture;
}

uint16_t RadioManager::copyLearnRaw(
    int16_t* destination, uint16_t capacity) const {
  RadioGuard guard;

  if (!destination || capacity == 0 || !learnCapture.available) return 0;

  const uint16_t count = min(learnCapture.pulseCount, capacity);
  for (uint16_t i = 0; i < count; i++) {
    destination[i] = learnRaw[i];
  }
  return count;
}

const char* RadioManager::getLearnStateName() const {
  RadioGuard guard;

  switch (learnCapture.state) {
    case LearnState::WAITING_FOR_SIGNAL: return "WAITING_FOR_SIGNAL";
    case LearnState::PREVIEW_READY: return "PREVIEW_READY";
    case LearnState::ACCEPTED_RAM: return "ACCEPTED_RAM";
    default: return "IDLE";
  }
}

RadioDiagnostics RadioManager::getDiagnostics() const {
  RadioGuard guard;
  return diagnostics_;
}

RadioChannelDiagnostics RadioManager::getChannelDiagnostics(
    uint8_t radioId) const {
  RadioGuard guard;

  RadioChannelDiagnostics result;
  RadioChannel* ch = channelById(radioId);
  if (!ch) return result;

  result.initialized = ch->initialized;
  result.receiving = ch->initialized && ch->mode == RadioMode::RX;
  result.rawCandidates = ch->rawCandidates;
  result.acceptedFrames = ch->acceptedFrames;
  result.rejectedFrames = ch->rejectedFrames;
  result.lastFinalizedPulses = ch->lastFinalizedPulses;
  result.lastFinalizedDurationUs = ch->lastFinalizedDurationUs;
  result.lastFinalizedAtMs = ch->lastFinalizedAtMs;
  result.lastFinalizeReason = ch->lastFinalizeReason;
  result.lastValidationResult = ch->lastValidationResult;

  uint32_t lastEdgeUsCopy = 0;
  portENTER_CRITICAL(ch->mux);
  result.edges = ch->totalEdges;
  result.currentPulses = ch->rxCount;
  result.ignoredGlitchEdges = ch->ignoredGlitchEdges;
  result.ignoredWhileFrameReady = ch->ignoredWhileFrameReady;
  result.shortGapResets = ch->shortGapResets;
  result.gapFinalizedFrames = ch->gapFinalizedFrames;
  result.timeoutFinalizedFrames = ch->timeoutFinalizedFrames;
  result.stalePartialFinalizedFrames =
      ch->stalePartialFinalizedFrames;
  result.bufferFullFrames = ch->bufferFullFrames;
  result.mergedSameSignPulses = ch->mergedSameSignPulses;
  result.frameReady = ch->frameReady;
  lastEdgeUsCopy = ch->lastEdgeUs;
  portEXIT_CRITICAL(ch->mux);

  if (result.currentPulses > 0 && lastEdgeUsCopy > 0) {
    result.pendingAgeUs = static_cast<uint32_t>(micros() - lastEdgeUsCopy);
  }

  return result;
}

bool RadioManager::setOperatingFrequency(uint8_t radioId,
                                               float frequencyMHz) {
  RadioGuard guard;

  RadioChannel* ch = channelById(radioId);
  if (!ch || !ch->initialized || !ch->radio) return false;

  const float minFrequency = radioId == 1 ? 430.0F : 867.0F;
  const float maxFrequency = radioId == 1 ? 440.0F : 870.0F;

  if (frequencyMHz < minFrequency || frequencyMHz > maxFrequency) {
    return false;
  }

  const bool wasReceiving = ch->mode == RadioMode::RX;
  if (wasReceiving && !stopChannelReceive(*ch)) {
    lastError_ = ch->lastError;
    return false;
  }

  ch->lastError = ch->radio->setFrequency(frequencyMHz);
  if (ch->lastError != RADIOLIB_ERR_NONE) {
    lastError_ = ch->lastError;
    if (wasReceiving) startChannelReceive(*ch);
    return false;
  }

  ch->frequencyMHz = frequencyMHz;

  // Only explicit System tuning uses setOperatingFrequency(). A non-default
  // frequency therefore starts/restarts a 15-minute tuning session.
  if (fabsf(frequencyMHz - ch->defaultFrequencyMHz) >
      OPENRF_FREQUENCY_TUNED_EPSILON_MHZ) {
    ch->tuneSessionExpiresAtMs = millis() + TUNE_SESSION_DURATION_MS;
  } else {
    ch->tuneSessionExpiresAtMs = 0;
  }

  if (wasReceiving && !startChannelReceive(*ch)) {
    lastError_ = ch->lastError;
    return false;
  }

  Serial.print(F("CC1101 Radio "));
  Serial.print(radioId);
  Serial.print(F(" Operating frequency -> "));
  Serial.print(frequencyMHz, 4);
  if (ch->tuneSessionExpiresAtMs != 0) Serial.print(F(" MHz (15 min TUNED session)"));
  else Serial.print(F(" MHz (Default)"));
  Serial.println();

  lastError_ = RADIOLIB_ERR_NONE;
  return true;
}

bool RadioManager::restoreDefaultFrequency(uint8_t radioId) {
  const float defaultMHz = getDefaultFrequency(radioId);
  if (defaultMHz <= 0.0F) return false;
  return setOperatingFrequency(radioId, defaultMHz);
}

float RadioManager::getOperatingFrequency(uint8_t radioId) const {
  RadioGuard guard;
  RadioChannel* ch = channelById(radioId);
  return ch ? ch->frequencyMHz : 0.0F;
}

float RadioManager::getDefaultFrequency(uint8_t radioId) const {
  RadioGuard guard;
  RadioChannel* ch = channelById(radioId);
  return ch ? ch->defaultFrequencyMHz : 0.0F;
}

bool RadioManager::isFrequencyTuned(uint8_t radioId) const {
  RadioGuard guard;
  RadioChannel* ch = channelById(radioId);
  if (!ch) return false;
  return fabsf(ch->frequencyMHz - ch->defaultFrequencyMHz) >
         OPENRF_FREQUENCY_TUNED_EPSILON_MHZ;
}

uint32_t RadioManager::getTuneSessionRemainingMs(uint8_t radioId) const {
  RadioGuard guard;
  RadioChannel* ch = channelById(radioId);
  if (!ch || ch->tuneSessionExpiresAtMs == 0) return 0;
  const uint32_t nowMs = millis();
  if (static_cast<int32_t>(nowMs - ch->tuneSessionExpiresAtMs) >= 0) return 0;
  return ch->tuneSessionExpiresAtMs - nowMs;
}

bool RadioManager::scanRssi(uint8_t radioId, float frequencyMHz,
                            float& rssiDbm, uint16_t dwellMs) {
  RadioGuard guard;

  RadioChannel* ch = channelById(radioId);

  if (!ch || !ch->initialized || !ch->radio ||
      ch->mode != RadioMode::RX) {
    return false;
  }

  const float minFrequency = radioId == 1 ? 430.0F : 867.0F;
  const float maxFrequency = radioId == 1 ? 440.0F : 870.0F;

  if (frequencyMHz < minFrequency || frequencyMHz > maxFrequency) {
    return false;
  }

  dwellMs = constrain(
      dwellMs,
      static_cast<uint16_t>(20),
      static_cast<uint16_t>(250));

  if (!stopChannelReceive(*ch)) {
    lastError_ = ch->lastError;
    return false;
  }

  ch->lastError = ch->radio->setFrequency(frequencyMHz);
  if (ch->lastError != RADIOLIB_ERR_NONE) {
    ch->radio->setFrequency(ch->frequencyMHz);
    startChannelReceive(*ch);
    lastError_ = ch->lastError;
    return false;
  }

  ch->lastError = ch->radio->receiveDirectAsync();
  if (ch->lastError != RADIOLIB_ERR_NONE) {
    ch->radio->setFrequency(ch->frequencyMHz);
    startChannelReceive(*ch);
    lastError_ = ch->lastError;
    return false;
  }

  delay(5);

  float peakRssi = -127.0F;
  const uint32_t startedMs = millis();

  while (static_cast<uint32_t>(millis() - startedMs) < dwellMs) {
    const float sample = ch->radio->getRSSI();
    if (sample > peakRssi) peakRssi = sample;
    delay(2);
    yield();
  }

  rssiDbm = peakRssi;

  ch->radio->standby();
  ch->radio->setFrequency(ch->frequencyMHz);

  if (!startChannelReceive(*ch)) {
    lastError_ = ch->lastError;
    return false;
  }

  lastError_ = RADIOLIB_ERR_NONE;
  return true;
}
