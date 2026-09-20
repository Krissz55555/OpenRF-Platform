#pragma once

#include <Arduino.h>

constexpr uint16_t OPENRF_ISR_CAPTURE_PULSES = 600;
constexpr uint16_t OPENRF_MAX_RAW_PULSES = 2048;

// Central RF frequency model. The defaults describe the normal band/profile
// center, while each RadioChannel keeps a separate operating frequency that
// may be changed by System frequency tuning.
constexpr float OPENRF_RADIO1_DEFAULT_FREQUENCY_MHZ = 433.920F;
constexpr float OPENRF_RADIO2_DEFAULT_FREQUENCY_MHZ = 868.350F;
constexpr float OPENRF_FREQUENCY_TUNED_EPSILON_MHZ = 0.0005F;

enum class RadioMode : uint8_t { OFFLINE, IDLE, RX, TX, ERROR };
enum class LearnState : uint8_t { IDLE, WAITING_FOR_SIGNAL, PREVIEW_READY, ACCEPTED_RAM };

struct RawFrameInfo {
  bool available = false;
  uint32_t sequence = 0;
  uint16_t pulseCount = 0;
  uint32_t durationUs = 0;
  float rssiDbm = -127.0F;
  float frequencyMHz = 0.0F;
  uint8_t radioId = 0;
  uint32_t receivedAtMs = 0;
};

struct LearnCaptureInfo {
  LearnState state = LearnState::IDLE;
  bool available = false;
  uint32_t sequence = 0;
  uint16_t pulseCount = 0;
  uint32_t durationUs = 0;
  float rssiDbm = -127.0F;
  float noiseFloorDbm = -127.0F;
  float frequencyMHz = 0.0F;
  uint8_t radioId = 0;
  uint32_t capturedAtMs = 0;
  uint32_t rejectedDuringLearn = 0;
  String lastRejectReason;
};

struct RadioChannelDiagnostics {
  bool initialized = false;
  bool receiving = false;
  uint32_t edges = 0;
  uint32_t rawCandidates = 0;
  uint32_t acceptedFrames = 0;
  uint32_t rejectedFrames = 0;
  uint16_t currentPulses = 0;

  // Frame-finalization diagnostics. These are intentionally read-only
  // counters/state so we can observe the capture engine without changing it.
  uint32_t ignoredGlitchEdges = 0;
  uint32_t ignoredWhileFrameReady = 0;
  uint32_t shortGapResets = 0;
  uint32_t gapFinalizedFrames = 0;
  uint32_t timeoutFinalizedFrames = 0;
  uint32_t stalePartialFinalizedFrames = 0;
  uint32_t bufferFullFrames = 0;
  uint32_t mergedSameSignPulses = 0;
  bool frameReady = false;
  uint32_t pendingAgeUs = 0;

  uint16_t lastFinalizedPulses = 0;
  uint32_t lastFinalizedDurationUs = 0;
  uint32_t lastFinalizedAtMs = 0;
  String lastFinalizeReason;
  String lastValidationResult;
};

struct RadioDiagnostics {
  uint32_t rawCandidates = 0;
  uint32_t acceptedFrames = 0;
  uint32_t rejectedFrames = 0;
  uint32_t backgroundFilteredFrames = 0;
  uint32_t ignoredGlitchEdges = 0;
  uint32_t gapFinalizedFrames = 0;
  uint32_t timeoutFinalizedFrames = 0;
  uint32_t bufferFullFrames = 0;
  uint32_t mergedSameSignPulses = 0;
  uint32_t txCount = 0;
  uint32_t txErrors = 0;
  float lastNoiseFloorDbm = -127.0F;
  String lastRejectReason;
};

class RadioManager {
 public:
  bool begin();
  void loop();

  // Start/stop every initialized CC1101 receiver.
  bool startReceive();
  bool stopReceive();

  bool isInitialized() const;
  bool isReceiving() const;
  bool isRadioActive(uint8_t radioId) const;

  float getRSSI();
  float getRadioRSSI(uint8_t radioId);
  float getFrequency() const;
  uint8_t getActiveRadioId() const;

  const char* getChipName() const;
  const char* getModeName() const;
  int16_t getLastError() const;

  RawFrameInfo getLastFrameInfo() const;
  uint16_t copyLastRaw(int16_t* destination, uint16_t capacity) const;
  bool hasRawFrame() const;

  bool startLearning();
  bool acceptLearnCapture();
  bool discardLearnCapture();
  bool testSendLearnCapture(uint8_t repeats);

  // Frequency determines the physical CC1101:
  // 433.x -> Radio 1, 868.x -> Radio 2.
  bool sendRaw(const int16_t* pulses, uint16_t pulseCount, uint8_t repeats,
               float frequencyMHz);

  // Step 24: transmit on an explicit radio at a slot-specific frequency, then
  // restore that radio's previous Operating frequency before RX resumes.
  bool sendRawTuned(const int16_t* pulses, uint16_t pulseCount, uint8_t repeats,
                    uint8_t radioId, float frequencyMHz);

  LearnCaptureInfo getLearnCaptureInfo() const;
  uint16_t copyLearnRaw(int16_t* destination, uint16_t capacity) const;
  const char* getLearnStateName() const;
  RadioDiagnostics getDiagnostics() const;
  RadioChannelDiagnostics getChannelDiagnostics(uint8_t radioId) const;

  // Generic low-level RSSI scan. It pauses only the selected radio and restores
  // that radio's fixed operating frequency afterwards.
  bool scanRssi(uint8_t radioId, float frequencyMHz, float& rssiDbm,
                uint16_t dwellMs = 80);

  // Retune one active CC1101 and keep the new center frequency as its normal RX
  // frequency for subsequent capture/analyzer operation.
  // System tuning starts a temporary 15-minute Operating-frequency session.
  // TX/scanner temporary retunes do not use this method.
  bool setOperatingFrequency(uint8_t radioId, float frequencyMHz);
  bool restoreDefaultFrequency(uint8_t radioId);
  float getOperatingFrequency(uint8_t radioId) const;
  float getDefaultFrequency(uint8_t radioId) const;
  bool isFrequencyTuned(uint8_t radioId) const;
  uint32_t getTuneSessionRemainingMs(uint8_t radioId) const;

 private:
  void finalizeFrame(uint8_t radioId);
  bool validateFrame(const int16_t* data, uint16_t count, uint32_t durationUs,
                     String& reason) const;
  bool validateLearnSignal(float frameRssi, float noiseFloor,
                           String& reason) const;
  void waitUs(uint32_t durationUs);

  int16_t lastError_ = 0;
  RadioDiagnostics diagnostics_;
};

extern RadioManager Radio;
