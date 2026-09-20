#include <cassert>
#include <cstdio>
#include "v2_authoritative_action.h"
#include "normalized_event_dedup.h"
NormalizedRfEvent event(ProtocolId protocol, uint32_t time, uint64_t code=1, uint8_t radio=1) {
 NormalizedRfEvent e; e.available=true;e.protocol=protocol;e.code=code;
 e.radioId=radio;e.capturedAtMs=time;return e;
}
void check(const NormalizedRfEvent& e, bool emit){
 assert(v2AuthoritativeActionRoute(e).emitV2Action==emit);
 assert((normalizedEventDedupObserve(e).state==NormalizedEventDedupState::EMIT)==emit);
}
int main(){
 const auto nv=ProtocolId::NVKP01_KINETIC, ev=ProtocolId::EV1527_PRINCETON;
 check(event(nv,1000),true); // First capture must emit, not wait for confirmation.
 check(event(nv,1399),false);
 check(event(nv,1799),false); // Inclusive 400ms, sliding inactivity.
 check(event(nv,2200),true); // 401ms silence is a new press.
 check(event(nv,2300,2),true); // Different code is independent.
 check(event(nv,2400,2,2),true); // Different radio is independent.
 check(event(ev,3000),true);
 check(event(ev,3300),false);
 check(event(ev,3601),true); // Other protocols still 300ms.
 check(event(nv,0xffffff00U,3),true);
 check(event(nv,0x90U,3),false); // 400ms across wrap.
 check(event(nv,0x221U,3),true); // 401ms across wrap.
 NormalizedRfEvent none;
 assert(!v2AuthoritativeActionRoute(none).emitV2Action);
 assert(normalizedEventDedupObserve(none).state==NormalizedEventDedupState::NOT_APPLICABLE);
 assert(v2AuthoritativeActionGetStatus().nvkpConfirmationPendingCount==0);
 std::puts("FIX3 production route + normalized dedup: first capture / 399 / 400 / 401 / key / wrap PASS");
}
