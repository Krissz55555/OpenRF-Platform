#include <cassert>
#include <cstdio>
#include <vector>
#include <string>
#include "step40_fix3_samples.h"
#include "protocols/nvkp01_decoder.h"
#include "protocols/known_protocol_library.h"
#include "protocol_tx.h"
uint32_t seed=0x40f13003U;
uint32_t rnd(){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
bool match(const std::vector<int16_t>&p){
 uint32_t d=0;for(auto x:p)d+=x<0?-int32_t(x):int32_t(x);
 return nvkp01V2Decoder().decode(RawCapture(p.data(),p.size(),d,-35,433.92F,1,1000)).matched();
}
int main(){
 // Envelope-sized unstructured timings, not trivially sub-threshold noise.
 unsigned eligible=0;
 for(unsigned i=0;i<10000;++i){
  std::vector<int16_t> p(30+rnd()%21);uint32_t d=0;
  for(unsigned j=0;j<p.size();++j){int16_t x=100+rnd()%1700;d+=x;p[j]=j%2?x:-x;}
  if(d>=18000 && d<=60000) ++eligible;
  assert(!match(p));
 }
 unsigned encoded=0;
 for(auto id:{ProtocolId::EV1527_PRINCETON,ProtocolId::PT2262_TRI_STATE,ProtocolId::HT12E}){
  for(unsigned i=0;i<100;++i){
   ProtocolTxRequest r;r.protocol=id;r.symbolCount=id==ProtocolId::EV1527_PRINCETON?24:12;
   r.code=rnd()%(id==ProtocolId::EV1527_PRINCETON?0x1000000U:id==ProtocolId::PT2262_TRI_STATE?531441U:4096U);
   r.pulseLengthUs=180U+rnd()%521;r.requestedRepeats=1;
   int16_t pulses[128];ProtocolTxPlan plan;
   if(!knownProtocolLibraryTxEncoder(id)->encode(r,pulses,128,plan)) continue;
   ++encoded; std::vector<int16_t> p(pulses,pulses+plan.pulseCount);
   assert(!match(p));for(auto&x:p)x=-x;assert(!match(p));
  }
 }
 assert(encoded>=200);
 // Bounded merge must reject signed overflow and zeros, not fabricate a marker.
 auto p=fix3Samples().front().pulses;
 p[0]=30000;p[1]=30000;assert(!match(p));
 p=fix3Samples().front().pulses;p[2]=0;assert(!match(p));
 // Sign inversion must not turn either mechanical phase into a valid frame.
 for(auto s:fix3Samples()){for(auto&x:s.pulses)x=-x;assert(!match(s.pulses));}
 std::printf("FIX3 negatives PASS: 10000 synthetic captures (%u in envelope), %u encoded other-protocol frames + inversion\n",eligible,encoded);
}
