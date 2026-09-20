#include <cassert>
#include <cstdio>
#include <vector>
#include "protocol_engine.h"
#include "protocols/nvkp01_decoder.h"
RawCapture capture(const std::vector<int16_t>& p) {
 uint32_t d=0;for(auto x:p)d+=x<0?-int32_t(x):int32_t(x);
 return RawCapture(p.data(),p.size(),d,-24,433.92F,1,1000);
}
bool match(const std::vector<int16_t>& p) {
 return nvkp01V2Decoder().decode(capture(p)).matched();
}
int main(){
 assert(protocolEngineBegin());
std::vector<std::vector<int16_t>> samples={
{-1515,679,-1577,209,-1354,232,-175,662,-1521,674,-957,450,-344,428,-163,645,-159,249,-349,439,-1571,641,-164,446,-562,226,-551,253,-173,1003,-188,550,-1645,1178,-646,150,-634,162,-229,353,-463,3537,-854,166},
{-1486,689,-735,640,-160,676,-931,252,-159,624,-1562,631,-172,441,-564,229,-366,367,-235,361,-446,163,-651,164,-2088,180,-169,381,-608,172,-643,557,-630,163,-647,169,-2012,184,-216,150,-874,802}};

 for(const auto& p:samples){
  const auto original=p;
  Nvkp01DecodeDiagnostics d;
  assert(nvkp01V2Decoder().decodeDetailed(capture(p),d).matched());
  assert(d.compactStructure && !d.syncStructure && d.normalizedCode==1U);
  const auto o=protocolEngineObserve(capture(p));
  assert(o.decision==ProtocolEngineDecisionState::KNOWN);
  assert(o.selectedProtocol==ProtocolId::NVKP01_KINETIC);
  assert(p==original);
 }
 auto p=samples[0]; p[20]=-1100; assert(!match(p)); // no reinforcing M2
 p=samples[0]; p[20]=-1750; p[21]=700; assert(!match(p)); // M2 period exceeds 2350 us
 p=samples[0]; p[34]=-500; assert(!match(p)); // only one reverse cell
 p=samples[0]; p.insert(p.begin()+36,{-400,400}); assert(!match(p)); // R cells separated
 p=samples[0]; p[15]=400; assert(!match(p)); // no forward cell after header
 p=samples[0]; p[10]=-500; assert(!match(p)); // no adjacent header
 p=samples[0]; for(auto& x:p)x=-x; assert(!match(p));
 // Modest uniform drift keeps the complete relationships intact.
 for(int percent:{-3,3}) {
  p=samples[0]; for(auto& x:p)x=int32_t(x)*(100+percent)/100;
  assert(match(p));
 }
 std::puts("FIX4 PASS: both new captures KNOWN/NVKP01, RAW immutable, 7 near-miss rejects, +/-3% scale");
}
