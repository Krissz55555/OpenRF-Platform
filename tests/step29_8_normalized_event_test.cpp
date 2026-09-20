#include <array>
#include <cstdint>
#include <cstdio>

#include "protocol_engine.h"

namespace {
constexpr uint32_t kT = 400U;
constexpr size_t kFramePulses = 50U;
int16_t neg(uint32_t value) { return static_cast<int16_t>(-static_cast<int32_t>(value)); }

template <size_t N>
RawCapture captureFor(const std::array<int16_t, N>& pulses, uint8_t radio = 1U) {
  uint32_t duration = 0U;
  for (const int16_t p : pulses) duration += static_cast<uint32_t>(p < 0 ? -p : p);
  return RawCapture(pulses.data(), static_cast<uint16_t>(pulses.size()), duration,
                    -31.5F, 433.92F, radio, 1234U);
}

template <size_t N>
void appendEv(std::array<int16_t, N>& out, size_t base, uint32_t code) {
  for (size_t bit = 0; bit < 24U; ++bit) {
    const bool one = ((code >> (23U-bit)) & 1U) != 0U;
    out[base+bit*2U] = static_cast<int16_t>(one ? kT*3U : kT);
    out[base+bit*2U+1U] = neg(one ? kT : kT*3U);
  }
  out[base+48U] = static_cast<int16_t>(kT);
  out[base+49U] = neg(kT*31U);
}

template <size_t N>
void appendPt(std::array<int16_t, N>& out, size_t base,
              const std::array<uint8_t,12>& trits) {
  for (size_t i=0;i<trits.size();++i) {
    const size_t o=base+i*4U; const uint8_t s=trits[i];
    if (s==0U) { out[o]=kT; out[o+1]=neg(kT*3U); out[o+2]=kT; out[o+3]=neg(kT*3U); }
    else if (s==1U) { out[o]=kT*3U; out[o+1]=neg(kT); out[o+2]=kT*3U; out[o+3]=neg(kT); }
    else { out[o]=kT; out[o+1]=neg(kT*3U); out[o+2]=kT*3U; out[o+3]=neg(kT); }
  }
  out[base+48U]=kT; out[base+49U]=neg(kT*31U);
}

uint64_t base3(const std::array<uint8_t,12>& trits) {
  uint64_t code=0U; for (uint8_t t:trits) code=code*3U+t; return code;
}

bool evEvent() {
  constexpr uint32_t code=0xFFFF09U;
  std::array<int16_t,kFramePulses*3U> pulses{};
  for(size_t f=0;f<3U;++f) appendEv(pulses,f*kFramePulses,code);
  const auto o=protocolEngineObserve(captureFor(pulses));
  const auto& e=o.normalizedEvent;
  return o.decision==ProtocolEngineDecisionState::KNOWN && e.available &&
         e.protocol==ProtocolId::EV1527_PRINCETON && e.code==code &&
         e.symbolCount==24U && e.repeats==3U && e.radioId==1U &&
         e.rawPulseCount==pulses.size() && e.capturedAtMs==1234U;
}

bool ptEvent() {
  const std::array<uint8_t,12> trits={2,2,0,1,2,0,2,1,0,1,2,0};
  std::array<int16_t,kFramePulses*3U> pulses{};
  for(size_t f=0;f<3U;++f) appendPt(pulses,f*kFramePulses,trits);
  const auto o=protocolEngineObserve(captureFor(pulses,2U));
  const auto& e=o.normalizedEvent;
  return o.decision==ProtocolEngineDecisionState::KNOWN && e.available &&
         e.protocol==ProtocolId::PT2262_TRI_STATE && e.code==base3(trits) &&
         e.symbolCount==12U && e.repeats==3U && e.radioId==2U;
}

bool unknownHasNoEvent() {
  std::array<int16_t,24U> pulses{};
  for(size_t i=0;i<pulses.size();++i) pulses[i]=(i%2U)==0U ? static_cast<int16_t>(100U+i) : neg(170U+i*11U);
  const auto o=protocolEngineObserve(captureFor(pulses));
  return o.decision==ProtocolEngineDecisionState::UNKNOWN && !o.normalizedEvent.available &&
         o.normalizedEvent.protocol==ProtocolId::UNKNOWN;
}
}

int main() {
  if(!protocolEngineBegin()) return 1;
  if(!evEvent()) return 2;
  if(!ptEvent()) return 3;
  if(!unknownHasNoEvent()) return 4;
  std::puts("Step 29.8 normalized RF event tests: PASS");
  return 0;
}
