#pragma once

#include <Arduino.h>

constexpr uint16_t OPENRF_MAX_RAW_PULSES = 2000;

enum class RadioMode : uint8_t { OFFLINE, IDLE, RX, TX, ERROR };
enum class LearnState : uint8_t { IDLE, WAITING_FOR_SIGNAL, PREVIEW_READY, ACCEPTED_RAM };

struct RawFrameInfo {
  bool available = false;
  uint32_t sequence = 0;
  uint16_t pulseCount = 0;
  uint32_t durationUs = 0;
  float rssiDbm = -127.0F;
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
  uint32_t capturedAtMs = 0;
  uint32_t rejectedDuringLearn = 0;
  String lastRejectReason;
};

struct RadioDiagnostics {
  uint32_t rawCandidates = 0;
  uint32_t acceptedFrames = 0;
  uint32_t rejectedFrames = 0;
  uint32_t backgroundFilteredFrames = 0;
  uint32_t txCount = 0;
  uint32_t txErrors = 0;
  float lastNoiseFloorDbm = -127.0F;
  String lastRejectReason;
};

class RadioManager {
 public:
  bool begin();
  void loop();
  bool startReceive();
  bool stopReceive();
  bool isInitialized() const;
  bool isReceiving() const;
  float getRSSI();
  float getFrequency() const;
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
  bool sendRaw(const int16_t* pulses, uint16_t pulseCount, uint8_t repeats);
  LearnCaptureInfo getLearnCaptureInfo() const;
  uint16_t copyLearnRaw(int16_t* destination, uint16_t capacity) const;
  const char* getLearnStateName() const;
  RadioDiagnostics getDiagnostics() const;

 private:
  void finalizeFrame();
  bool validateFrame(const int16_t* data, uint16_t count, uint32_t durationUs,
                     String& reason) const;
  bool validateLearnSignal(float frameRssi, String& reason) const;
  void waitUs(uint32_t durationUs);

  bool initialized_ = false;
  RadioMode mode_ = RadioMode::OFFLINE;
  float frequencyMHz_ = 433.92F;
  float lastRssi_ = -127.0F;
  float framePeakRssi_ = -127.0F;
  uint32_t lastRssiReadMs_ = 0;
  int16_t lastError_ = 0;
  RadioDiagnostics diagnostics_;
};

extern RadioManager Radio;
