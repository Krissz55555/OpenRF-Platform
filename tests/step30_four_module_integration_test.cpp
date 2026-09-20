#include <array>
#include <cstdint>
#include <cstdio>

#include "protocol_engine.h"
#include "protocol_registry.h"
#include "protocols/ev1527_decoder.h"
#include "protocols/pt2262_decoder.h"
#include "protocols/nvkp01_decoder.h"
#include "protocols/ht12e_decoder.h"
#include "protocols/known_protocol_library.h"

namespace {
int16_t neg(uint32_t v){return static_cast<int16_t>(-static_cast<int32_t>(v));}
template<size_t N> RawCapture cap(const std::array<int16_t,N>&p,uint32_t t){uint32_t d=0;for(auto v:p)d+=static_cast<uint32_t>(v<0?-v:v);return RawCapture(p.data(),static_cast<uint16_t>(N),d,-30.0F,433.92F,1U,t);}
void appendHt(std::array<int16_t,104>& p,size_t b,uint16_t w,uint16_t t=280){p[b]=neg(t*36U);p[b+1]=t;for(size_t bit=0;bit<12;++bit){bool one=((w>>(11U-bit))&1U)!=0U;p[b+2+bit*2]=neg(one?t*2U:t);p[b+3+bit*2]=one?t:t*2U;}}

bool registryFour(){
  if(!protocolEngineBegin() || !protocolEngineBegin()) return false;
  auto&r=protocolEngineRegistry();
  return knownProtocolLibraryCount()==4U && r.count()==4U &&
    r.at(0)==&ev1527Decoder() && r.at(1)==&pt2262Decoder() &&
    r.at(2)==&nvkp01V2Decoder() && r.at(3)==&ht12eDecoder();
}

bool htKnownNormalizedActionable(){
  std::array<int16_t,104> p{}; for(size_t i=0;i<4;++i) appendHt(p,i*26U,0x5A3U);
  const auto o=protocolEngineObserve(cap(p,5000U));
  return o.registeredDecoders==4U && o.evaluatedDecoders==4U && o.matchCount==1U &&
    o.decision==ProtocolEngineDecisionState::KNOWN && o.selectedProtocol==ProtocolId::HT12E &&
    o.ht12eDiagnosticsAvailable && o.ht12eStatus==ProtocolMatchStatus::MATCH &&
    o.normalizedEvent.available && o.normalizedEvent.protocol==ProtocolId::HT12E &&
    o.normalizedEvent.code==0x5A3U && o.normalizedEvent.symbolCount==12U &&
    o.actionableDryRun.state==V2ActionableDryRunState::WOULD_EMIT;
}

bool repeatedHtCollapsed(){
  std::array<int16_t,104> p{}; for(size_t i=0;i<4;++i) appendHt(p,i*26U,0x5A3U);
  (void)protocolEngineObserve(cap(p,10000U));
  const auto o=protocolEngineObserve(cap(p,10150U));
  return o.dedup.state==NormalizedEventDedupState::COLLAPSED &&
    o.actionableDryRun.state==V2ActionableDryRunState::SUPPRESSED_DUPLICATE;
}
}
int main(){
 if(!registryFour()) return 1;
 if(!htKnownNormalizedActionable()) return 2;
 if(!repeatedHtCollapsed()) return 3;
 std::puts("Step 30 four-module integration tests: PASS");
 return 0;
}
