#include <cassert>
#include <iostream>
#define MARKETSYS_NODE_ID 2
#include "../firmware/radio_node/radio_node.ino"
static void query(uint32_t token){benchRadio::inject({bench::QUERY,2,bench::OFF,bench::APPLIED,token});loop();fakeNow+=50;loop();}
int main(){
 setup();assert(bootTest.running());auto start=started;
 query(1);assert(bootTest.running() && !dedup.valid && started==start);
 assert(benchRadio::outgoing.back().type==bench::STATUS && benchRadio::outgoing.back().result==bench::BOOTING);
 benchRadio::inject({bench::SET,2,bench::FULL,bench::APPLIED,100});loop();
 assert(bootTest.running() && !dedup.valid); // SET cannot cut the test short.
 fakeNow=15010;loop();assert(!bootTest.running() && state==bench::OFF);
 benchRadio::inject({bench::SET,2,bench::FULL,bench::APPLIED,100});loop();fakeNow+=50;loop();
 assert(state==bench::FULL && dedup.token==100 && levels[1]==HIGH);
 start=started;fakeNow=start+10001;query(101);
 assert(state==bench::FULL && started==start && dedup.token==100 && levels[1]==HIGH);
 fakeNow=start+86400000;query(102);assert(state==bench::FULL && levels[1]==HIGH);
 fakeNow=start+1000000001UL;loop();assert(state==bench::FULL && levels[1]==HIGH);
 fakeNow=0xfffffff0;loop();fakeNow+=1000;loop();assert(state==bench::FULL && levels[1]==HIGH);
 benchRadio::inject({bench::SET,2,bench::FULL,bench::APPLIED,100});loop();fakeNow+=50;loop();
 assert(benchRadio::outgoing.back().result==bench::DUPLICATE);
 auto n=benchRadio::outgoing.size();
 benchRadio::inject({bench::QUERY,1,bench::OFF,bench::APPLIED,103});loop();fakeNow+=50;loop();
 assert(benchRadio::outgoing.size()==n);
 benchRadio::inject({bench::REBOOT,2,bench::OFF,bench::APPLIED,104});loop();
 auto scheduled=rebootAt;fakeNow+=50;loop();
 assert(benchRadio::outgoing.back().type==bench::REBOOT_ACK && api.system.rebootCount==0);
 benchRadio::inject({bench::REBOOT,2,bench::OFF,bench::APPLIED,104});loop();assert(rebootAt==scheduled);
 fakeNow=scheduled;loop();assert(api.system.rebootCount==1 && levels[0]==LOW && levels[1]==LOW && levels[2]==LOW);
 std::cout<<"PASS: actual node boot readiness, complete boot, hold beyond 10s/day/millis wrap, dedup, isolation, delayed reboot\n";
}
