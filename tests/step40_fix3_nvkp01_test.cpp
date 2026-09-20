#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>
#include "step40_fix3_samples.h"
#include "protocol_engine.h"
#include "protocols/nvkp01_decoder.h"
#include "raw_match.h"

RawCapture capture(const std::vector<int16_t>& p, uint32_t now = 1000) {
  uint32_t duration = 0;
  for (auto x : p) duration += x < 0 ? -int32_t(x) : int32_t(x);
  return RawCapture(p.data(), static_cast<uint16_t>(p.size()), duration,
                    -35.0F, 433.92F, 1U, now);
}

int main() {
  assert(protocolEngineBegin());
  auto samples = fix3Samples();
  std::sort(samples.begin(), samples.end(), [](const NvSample& a, const NvSample& b) {
    return std::string(a.name) < std::string(b.name);
  });
  unsigned matched = 0;
  std::vector<int16_t> learned;
  uint32_t now = 1000;
  for (const auto& s : samples) {
    const auto before = s.pulses;
    Nvkp01DecodeDiagnostics d;
    const auto result = nvkp01V2Decoder().decodeDetailed(capture(s.pulses), d);
    std::printf("checking %s expected=%d actual=%d\n",s.name,s.expected,result.matched()); std::fflush(stdout);
    assert(before == s.pulses);
    assert(result.matched() == s.expected);
    if (s.expected) {
      ++matched;
      assert(d.codeAvailable && d.normalizedCode == 1U);
    } else {
      assert(!d.codeAvailable);
    }
    const auto o = protocolEngineObserve(capture(s.pulses, now));
    now += 1000;
    if (s.expected) {
      assert(o.decision == ProtocolEngineDecisionState::KNOWN);
      assert(o.selectedProtocol == ProtocolId::NVKP01_KINETIC);
    }
    std::printf("%s: %s (M=%u S=%u leader=%u reason=%s)\n", s.name,
        result.matched() ? "MATCH" : "NO_MATCH", d.markerPairs, d.syncPulses,
        d.fullLeader, nvkp01RejectReasonName(d.rejectReason));
    if (s.name[0] == 'B') {
      // Real four-decoder engine must yield UNKNOWN, not another false KNOWN
      // or AMBIGUOUS. This is radio.cpp's Learned RAW eligibility condition
      // for an accepted capture outside learn mode.
      assert(o.decision == ProtocolEngineDecisionState::UNKNOWN);
      assert(o.matchCount == 0U && !o.normalizedEvent.available);
      if (learned.empty()) learned = s.pulses;
      int16_t a[OpenRfRawMatch::kMaxPatternPulses]{};
      int16_t b[OpenRfRawMatch::kMaxPatternPulses]{};
      const auto pa = OpenRfRawMatch::prepare(learned.data(), learned.size(), a, 512);
      const auto pb = OpenRfRawMatch::prepare(s.pulses.data(), s.pulses.size(), b, 512);
      const auto raw = OpenRfRawMatch::comparePrepared(a, pa.pulseCount, b, pb.pulseCount);
      assert(raw.matched);
      std::printf("  Engine UNKNOWN; RAW match against B-01: %u%%\n", raw.similarity);
    }
  }
  assert(matched == 22U && samples.size() == 27U);
  // Negative controls: a leader alone is insufficient; polarity is not inferred.
  auto p = samples[0].pulses;
  for (auto& x : p) x = -x;
  assert(!nvkp01V2Decoder().decode(capture(p)).matched());
  p = {-1500,680,-5730,670,-500,500,-1500,650,-500,500,
       -1500,650,-500,500,-1500,650,-500,500,-1500,650};
  assert(!nvkp01V2Decoder().decode(capture(p)).matched());
  // Zero duration entries are not physical signed pulses, even if signs alternate.
  p = samples[0].pulses; p[1] = 0;
  assert(!nvkp01V2Decoder().decode(capture(p)).matched());
  std::puts("Step 40 FIX3: 27 captures + reject controls + UNKNOWN/RAW integration PASS");
}
