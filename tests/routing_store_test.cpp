#include <Arduino.h>
#include "../libraries/MarketSysBench/src/RoutingStore.h"
#include <cassert>
#include <iostream>
int main(){
 FlashMock flash;routing::Store<FlashMock> store(flash);routing::Record r;
 assert(store.load(r) && r.sequence==1 && r.configured[1] && r.configured[3] && !r.configured[4]);
 r.active=2;r.last=1;r.desired[2]=bench::ACTIVE;r.desired[1]=bench::LAST;
 assert(store.save(r));auto seq=r.sequence;
 routing::Store<FlashMock> restart(flash);routing::Record got;assert(restart.load(got));
 assert(got.active==2 && got.last==1 && got.desired[1]==bench::LAST && got.sequence==seq);
 flash.tear=true;r.desired[2]=bench::FULL;assert(!restart.save(r));flash.tear=false;
 routing::Store<FlashMock> powerLoss(flash);assert(powerLoss.load(got));
 assert(got.sequence==seq && got.desired[2]==bench::ACTIVE);
 got.auction=9;got.pendingReset=true;got.faults[3]=1;assert(powerLoss.save(got));
 routing::Store<FlashMock> again(flash);assert(again.load(r));assert(r.auction==9 && r.pendingReset && r.faults[3]);
 flash.readFail=true;assert(!again.load(r));flash.readFail=false;
 FlashMock unknown;unknown.bytes[0]=0;routing::Store<FlashMock> refuse(unknown);
 assert(!refuse.load(r) && unknown.writes==0);
 // Every single bit corruption in a serialized record is detected.
 uint8_t b[routing::RECORD_BYTES];routing::encode(got,b);
 for(size_t bit=0;bit<sizeof(b)*8;++bit){b[bit/8]^=1<<(bit%8);assert(!routing::decode(b,r));b[bit/8]^=1<<(bit%8);}
 FlashMock legacy;uint8_t old[routing::RECORD_BYTES];routing::Record initial;
 initial.sequence=77;initial.auction=3;initial.configured[1]=1;
 routing::encode(initial,old);memcpy(old,"MSR1",4);
 auto check=bench::crc(old,174);old[174]=uint8_t(check);old[175]=uint8_t(check>>8);
 assert(legacy.set(0,old,sizeof old));routing::Store<FlashMock> migration(legacy);
 assert(migration.load(r) && r.sequence==77 && r.auction==3 && !r.flowPending);
 r.flowPending=true;r.active=1;r.flowOldActive=0;r.flowStep=0;
 assert(migration.save(r));routing::Store<FlashMock> v2(legacy);
 assert(v2.load(got) && got.flowPending && got.active==1);
 std::cout<<"PASS: two-page durable state, cold load, torn write, pending clear, fault retention, corrupt/unknown refusal, all bit flips\n";
}
