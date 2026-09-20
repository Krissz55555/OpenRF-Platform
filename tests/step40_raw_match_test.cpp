#include <array>
#include <cstdio>

#include "raw_match.h"

using namespace OpenRfRawMatch;

namespace {

void buildFrame(std::array<int16_t, 40>& frame) {
  for (size_t i = 0; i < frame.size(); ++i) {
    const bool high = (i % 2U) == 0U;
    const int16_t mag = static_cast<int16_t>(
        (i % 7U == 0U) ? 2400 : ((i % 3U == 0U) ? 1200 : 400));
    frame[i] = high ? mag : static_cast<int16_t>(-mag);
  }
}

bool repeatedBurstMatchesAcrossHoldLength() {
  std::array<int16_t, 40> frame{};
  buildFrame(frame);

  std::array<int16_t, 120> learned{};
  std::array<int16_t, 200> incoming{};
  for (size_t r = 0; r < 3; ++r) {
    for (size_t i = 0; i < frame.size(); ++i) learned[r * frame.size() + i] = frame[i];
  }
  for (size_t r = 0; r < 5; ++r) {
    for (size_t i = 0; i < frame.size(); ++i) {
      int32_t v = frame[i];
      // Deterministic +/- 8% timing jitter.
      const int32_t pct = static_cast<int32_t>((i + r) % 5U) - 2;
      v += (v * pct * 4) / 100;
      incoming[r * frame.size() + i] = static_cast<int16_t>(v);
    }
  }

  std::array<int16_t, kMaxPatternPulses> a{};
  std::array<int16_t, kMaxPatternPulses> b{};
  const PreparedPattern pa = prepare(learned.data(), learned.size(), a.data(), a.size());
  const PreparedPattern pb = prepare(incoming.data(), incoming.size(), b.data(), b.size());
  if (!pa.repeatReduced || !pb.repeatReduced) return false;
  if (pa.pulseCount != frame.size() || pb.pulseCount != frame.size()) return false;
  const MatchResult result = comparePrepared(a.data(), pa.pulseCount, b.data(), pb.pulseCount);
  return result.matched && result.similarity >= 90U;
}

bool differentPatternRejected() {
  std::array<int16_t, 40> aFrame{};
  std::array<int16_t, 40> bFrame{};
  buildFrame(aFrame);
  buildFrame(bFrame);
  for (size_t i = 4; i < bFrame.size(); i += 6) {
    bFrame[i] = static_cast<int16_t>(bFrame[i] > 0 ? 3100 : -3100);
  }

  std::array<int16_t, kMaxPatternPulses> a{};
  std::array<int16_t, kMaxPatternPulses> b{};
  const PreparedPattern pa = prepare(aFrame.data(), aFrame.size(), a.data(), a.size());
  const PreparedPattern pb = prepare(bFrame.data(), bFrame.size(), b.data(), b.size());
  const MatchResult result = comparePrepared(a.data(), pa.pulseCount, b.data(), pb.pulseCount);
  return !result.matched;
}

bool polarityMismatchRejected() {
  std::array<int16_t, 24> aRaw{};
  std::array<int16_t, 24> bRaw{};
  for (size_t i = 0; i < aRaw.size(); ++i) {
    const int16_t v = (i % 2U == 0U) ? 500 : -1200;
    aRaw[i] = v;
    bRaw[i] = static_cast<int16_t>(-v);
  }
  std::array<int16_t, kMaxPatternPulses> a{};
  std::array<int16_t, kMaxPatternPulses> b{};
  const PreparedPattern pa = prepare(aRaw.data(), aRaw.size(), a.data(), a.size());
  const PreparedPattern pb = prepare(bRaw.data(), bRaw.size(), b.data(), b.size());
  return !comparePrepared(a.data(), pa.pulseCount, b.data(), pb.pulseCount).matched;
}

bool edgeShiftTolerated() {
  std::array<int16_t, 30> learned{};
  std::array<int16_t, 31> incoming{};
  for (size_t i = 0; i < learned.size(); ++i) {
    learned[i] = (i % 2U == 0U) ? static_cast<int16_t>(400 + (i % 5U) * 110)
                                : static_cast<int16_t>(-(850 + (i % 4U) * 90));
    incoming[i + 1U] = learned[i];
  }
  incoming[0] = -200;

  std::array<int16_t, kMaxPatternPulses> a{};
  std::array<int16_t, kMaxPatternPulses> b{};
  const PreparedPattern pa = prepare(learned.data(), learned.size(), a.data(), a.size());
  const PreparedPattern pb = prepare(incoming.data(), incoming.size(), b.data(), b.size());
  const MatchResult result = comparePrepared(a.data(), pa.pulseCount, b.data(), pb.pulseCount);
  return result.matched;
}

}  // namespace

int main() {
  if (!repeatedBurstMatchesAcrossHoldLength()) return 1;
  if (!differentPatternRejected()) return 2;
  if (!polarityMismatchRejected()) return 3;
  if (!edgeShiftTolerated()) return 4;
  std::puts("Step 40 RAW matcher tests: PASS");
  return 0;
}
