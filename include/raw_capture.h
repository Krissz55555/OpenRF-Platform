#pragma once

#include <stddef.h>
#include <stdint.h>

// Read-only, zero-copy view of one complete finalized RAW RF capture.
//
// The view does not own the pulse buffer. It is valid only during the
// synchronous Core 1 processing of the capture that created it. Consumers must
// not retain data(), begin(), end(), or the RawCapture itself after that call
// returns. A consumer that crosses a task/core boundary must copy the capture.
//
// Pulses are the untouched signed durations produced by capture finalization.
// This interface deliberately performs no splitting, trimming, normalization,
// decoding, protocol selection, or other transformation.
class RawCapture final {
 public:
  RawCapture(const int16_t* pulses,
             uint16_t pulseCount,
             uint32_t durationUs,
             float rssiDbm,
             float frequencyMHz,
             uint8_t radioId,
             uint32_t capturedAtMs)
      : pulses_(pulses),
        pulseCount_(pulseCount),
        durationUs_(durationUs),
        rssiDbm_(rssiDbm),
        frequencyMHz_(frequencyMHz),
        radioId_(radioId),
        capturedAtMs_(capturedAtMs) {}

  const int16_t* data() const { return pulses_; }
  const int16_t* begin() const { return pulses_; }
  const int16_t* end() const {
    return pulses_ == nullptr ? nullptr : pulses_ + pulseCount_;
  }
  const int16_t& operator[](size_t index) const { return pulses_[index]; }

  uint16_t pulseCount() const { return pulseCount_; }
  uint32_t durationUs() const { return durationUs_; }
  float rssiDbm() const { return rssiDbm_; }
  float frequencyMHz() const { return frequencyMHz_; }
  uint8_t radioId() const { return radioId_; }
  uint32_t capturedAtMs() const { return capturedAtMs_; }
  bool empty() const { return pulses_ == nullptr || pulseCount_ == 0; }

 private:
  const int16_t* const pulses_;
  const uint16_t pulseCount_;
  const uint32_t durationUs_;
  const float rssiDbm_;
  const float frequencyMHz_;
  const uint8_t radioId_;
  const uint32_t capturedAtMs_;
};
