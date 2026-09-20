#include <array>
#include <cstdint>
#include <cstdio>

#include "protocols/ht12e_decoder.h"
#include "protocols/ev1527_decoder.h"
#include "protocols/pt2262_decoder.h"
#include "protocols/nvkp01_decoder.h"

namespace {

int16_t neg(uint32_t v) { return static_cast<int16_t>(-static_cast<int32_t>(v)); }

template <size_t N>
RawCapture captureFor(const std::array<int16_t, N>& p) {
  uint32_t d=0; for (auto v:p) d += static_cast<uint32_t>(v<0?-v:v);
  return RawCapture(p.data(), static_cast<uint16_t>(N), d, -32.0F, 433.92F, 1U, 1000U);
}

void appendHt(std::array<int16_t,104>& p, size_t base, uint16_t word, uint16_t t=280U) {
  p[base] = neg(t*36U);
  p[base+1] = static_cast<int16_t>(t);
  for (size_t bit=0; bit<12; ++bit) {
    const bool one = ((word >> (11U-bit)) & 1U) != 0U;
    p[base+2+bit*2] = neg(one ? t*2U : t);
    p[base+3+bit*2] = static_cast<int16_t>(one ? t : t*2U);
  }
}

void appendEv(std::array<int16_t,150>& p, size_t base, uint32_t code, uint16_t t=400U) {
  for(size_t bit=0;bit<24;++bit){ bool one=((code>>(23-bit))&1U)!=0U; p[base+bit*2]=one?t*3:t; p[base+bit*2+1]=neg(one?t:t*3); }
  p[base+48]=t; p[base+49]=neg(t*31U);
}

bool validFourWords() {
  std::array<int16_t,104> p{};
  for(size_t i=0;i<4;++i) appendHt(p,i*26U,0xA5BU);
  Ht12eDecodeDiagnostics d;
  auto r=ht12eDecoder().decodeDetailed(captureFor(p),d);
  return r.matched() && d.codeAvailable && d.decodedWord==0xA5BU &&
         d.address==0xA5U && d.data==0xBU && d.matchingWords==4U;
}

bool noisyTolerance() {
  std::array<int16_t,104> p{};
  for(size_t i=0;i<4;++i) appendHt(p,i*26U,0x3C4U,300U);
  // modest real-world jitter on a few pulses
  p[2] = -330; p[3] = 570; p[28] = -570; p[29] = 315;
  return ht12eDecoder().decode(captureFor(p)).matched();
}

bool singleWordRejected() {
  std::array<int16_t,26> p{};
  p[0]=neg(280U*36U); p[1]=280;
  uint16_t w=0x123U;
  for(size_t bit=0;bit<12;++bit){bool one=((w>>(11-bit))&1U)!=0U;p[2+bit*2]=neg(one?560U:280U);p[3+bit*2]=one?280:560;}
  Ht12eDecodeDiagnostics d;
  auto r=ht12eDecoder().decodeDetailed(captureFor(p),d);
  return !r.matched() && d.rejectReason==Ht12eRejectReason::REPEAT_MISMATCH;
}

bool differentWordsRejected() {
  std::array<int16_t,52> p{};
  auto put=[&](size_t base,uint16_t w){p[base]=neg(10080);p[base+1]=280;for(size_t bit=0;bit<12;++bit){bool one=((w>>(11-bit))&1U)!=0U;p[base+2+bit*2]=neg(one?560U:280U);p[base+3+bit*2]=one?280:560;}};
  put(0,0x123); put(26,0x124);
  return !ht12eDecoder().decode(captureFor(p)).matched();
}

bool evNotHt12e() {
  std::array<int16_t,150> p{}; appendEv(p,0,0xFFFF09); appendEv(p,50,0xFFFF09); appendEv(p,100,0xFFFF09);
  return !ht12eDecoder().decode(captureFor(p)).matched();
}

bool nvNotHt12e() {
  const std::array<int16_t,23> p={-1502,708,-5709,684,-5722,653,-959,448,-353,448,-960,250,-167,626,-1550,644,-162,357,-654,960,-245,336,-464};
  return !ht12eDecoder().decode(captureFor(p)).matched();
}

} // namespace

int main(){
  if(!validFourWords()) return 1;
  if(!noisyTolerance()) return 2;
  if(!singleWordRejected()) return 3;
  if(!differentWordsRejected()) return 4;
  if(!evNotHt12e()) return 5;
  if(!nvNotHt12e()) return 6;
  std::puts("Step 30 HT12E decoder tests: PASS");
  return 0;
}
