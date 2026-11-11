#include <cassert>
#include <iostream>
#define MARKETSYS_NODE_ID 1
#include "../firmware/radio_node/radio_node.ino"
static bench::Frame exchange(bench::Frame f){benchRadio::inject(f);loop();fakeNow+=60;loop();return benchRadio::outgoing.back();}
int main(){
 setup();assert(historyOk&&history.bootId==1&&!appliedThisBoot);
 fakeNow=16000;loop();assert(!bootTest.running());
 auto initial=exchange({bench::QUERY,1,bench::OFF,bench::APPLIED,1});
 assert(initial.flags==bench::HISTORY_STORAGE_OK&&initial.state==bench::OFF);
 assert(initial.supplyMv==3275);
 uint8_t wire[bench::SIZE];
 bench::encode(initial,wire);
 bench::Frame decoded;
 assert(bench::decode(wire,sizeof wire,decoded)&&decoded.supplyMv==3275);
 bench::Frame set={bench::SET,1,bench::ACTIVE,bench::APPLIED,25};set.auction=7;
 auto ack=exchange(set);assert(ack.result==bench::APPLIED&&ack.auction==7&&ack.lastToken==25&&ack.lastState==bench::ACTIVE&&ack.flags==7);
 int writes=api.system.flash.writes;auto duplicate=exchange(set);
 assert(duplicate.result==bench::DUPLICATE&&api.system.flash.writes==writes);
 auto polled=exchange({bench::QUERY,1,bench::OFF,bench::APPLIED,26});
 assert(polled.lastToken==25&&polled.auction==7&&api.system.flash.writes==writes);
 // Emulate reset of RAM with flash retained, including a new boot identity.
 state=bench::OFF;dedup=bench::Dedup();bootTest=indicator::BootLightTest();appliedThisBoot=false;ackPending=false;rebootPending=false;fakeNow=0;
 setup();assert(historyOk&&history.bootId==2&&history.valid&&history.token==25&&!appliedThisBoot);
 fakeNow=16000;loop();auto rebooted=exchange({bench::QUERY,1,bench::OFF,bench::APPLIED,27});
 assert(rebooted.state==bench::OFF&&rebooted.lastState==bench::ACTIVE&&rebooted.flags==(bench::HISTORY_VALID|bench::HISTORY_STORAGE_OK));
 // Stale auction cannot relight a restarted node.
 set.auction=6;set.token=28;auto count=benchRadio::outgoing.size();benchRadio::inject(set);loop();fakeNow+=60;loop();
 assert(state==bench::OFF&&benchRadio::outgoing.size()==count);
 // A failed journal write must never apply or acknowledge success for the SET.
 set.auction=8;set.token=29;api.system.flash.tear=true;ack=exchange(set);
 assert(ack.result==bench::STORAGE_ERROR&&state==bench::OFF&&!historyOk&&!appliedThisBoot);
 auto failed=exchange({bench::QUERY,1,bench::OFF,bench::APPLIED,30});assert(failed.result==bench::STORAGE_ERROR&&!(failed.flags&bench::HISTORY_STORAGE_OK));
 std::cout<<"PASS: actual node persisted history, boot ID, read-only poll, duplicate wear avoidance, stale auction rejection, save failure blocks output\n";
}
