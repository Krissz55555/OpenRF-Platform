#include <array>
#include <cstdint>
#include <cstdio>

#include "protocol_engine.h"
#include "protocol_registry.h"
#include "protocols/ev1527_decoder.h"
#include "protocols/pt2262_decoder.h"
#include "protocols/nvkp01_decoder.h"

namespace {

template <size_t N>
RawCapture captureFor(const std::array<int16_t, N>& pulses, uint32_t atMs) {
  uint32_t duration = 0U;
  for (const int16_t p : pulses) duration += static_cast<uint32_t>(p < 0 ? -p : p);
  return RawCapture(pulses.data(), static_cast<uint16_t>(pulses.size()),
                    duration, -30.0F, 433.92F, 1U, atMs);
}

int16_t neg(uint32_t v) { return static_cast<int16_t>(-static_cast<int32_t>(v)); }

void appendEv(std::array<int16_t, 150>& out, size_t base, uint32_t code) {
  constexpr uint32_t t = 400U;
  for (size_t bit=0; bit<24U; ++bit) {
    const bool one=((code>>(23U-bit))&1U)!=0U;
    out[base+bit*2U]=static_cast<int16_t>(one?t*3U:t);
    out[base+bit*2U+1U]=neg(one?t:t*3U);
  }
  out[base+48U]=static_cast<int16_t>(t);
  out[base+49U]=neg(t*31U);
}

void appendPt(std::array<int16_t,150>& out, size_t base) {
  constexpr uint32_t t=400U;
  const uint8_t trits[12]={2,2,0,1,2,0,2,1,0,1,2,0};
  for(size_t i=0;i<12U;++i){
    const size_t o=base+i*4U;
    if(trits[i]==0U){out[o]=t;out[o+1]=neg(t*3U);out[o+2]=t;out[o+3]=neg(t*3U);}
    else if(trits[i]==1U){out[o]=t*3U;out[o+1]=neg(t);out[o+2]=t*3U;out[o+3]=neg(t);}
    else {out[o]=t;out[o+1]=neg(t*3U);out[o+2]=t*3U;out[o+3]=neg(t);}
  }
  out[base+48U]=t; out[base+49U]=neg(t*31U);
}

bool registryThree() {
  if(!protocolEngineBegin() || !protocolEngineBegin()) return false;
  ProtocolRegistry& r=protocolEngineRegistry();
  return r.count()>=3U && r.at(0)==&ev1527Decoder() &&
         r.at(1)==&pt2262Decoder() && r.at(2)==&nvkp01V2Decoder();
}

bool evKnown() {
  std::array<int16_t,150> p{};
  appendEv(p,0,0xFFFF09U); appendEv(p,50,0xFFFF09U); appendEv(p,100,0xFFFF09U);
  const auto o=protocolEngineObserve(captureFor(p,1000U));
  return o.decision==ProtocolEngineDecisionState::KNOWN &&
         o.selectedProtocol==ProtocolId::EV1527_PRINCETON &&
         o.normalizedEvent.available && o.normalizedEvent.code==0xFFFF09U;
}

bool ptKnown() {
  std::array<int16_t,150> p{};
  appendPt(p,0); appendPt(p,50); appendPt(p,100);
  const auto o=protocolEngineObserve(captureFor(p,2000U));
  return o.decision==ProtocolEngineDecisionState::KNOWN &&
         o.selectedProtocol==ProtocolId::PT2262_TRI_STATE &&
         o.normalizedEvent.available;
}

bool nvKnownAndActionable() {
  const std::array<int16_t,23> p={
      -1502,708,-5709,684,-5722,653,-959,448,-353,448,-960,250,
      -167,626,-1550,644,-162,357,-654,960,-245,336,-464};
  const auto o=protocolEngineObserve(captureFor(p,3000U));
  return o.decision==ProtocolEngineDecisionState::KNOWN &&
         o.selectedProtocol==ProtocolId::NVKP01_KINETIC &&
         o.normalizedEvent.available && o.normalizedEvent.code==1U &&
         o.actionableDryRun.state==V2ActionableDryRunState::WOULD_EMIT;
}

} // namespace

int main(){
  if(!registryThree()) return 1;
  if(!evKnown()) return 2;
  if(!ptKnown()) return 3;
  if(!nvKnownAndActionable()) return 4;
  std::puts("Step 29.12 three-module integration tests: PASS");
  return 0;
}
