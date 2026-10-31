#include <cassert>
#include <iostream>
#define MARKETSYS_NODE_ID 2
#include "../firmware/radio_node/radio_node.ino"
static void query(uint32_t token){benchRadio::inject({bench::QUERY,2,bench::OFF,bench::APPLIED,token});loop();fakeNow+=50;loop();}
int main(){
 setup();assert(bootTest.running());auto start=started;
 query(1);assert(bootTest.running() && !dedup.valid && started==start);
 assert(benchRadio::outgoing.back().type==bench::STATUS);
 benchRadio::inject({bench::SET,2,bench::FULL,bench::APPLIED,100});loop();fakeNow+=50;loop();
 assert(!bootTest.running() && state==bench::FULL && dedup.token==100);
 start=started;
 fakeNow=start+9900;query(101);
 assert(state==bench::FULL && started==start && dedup.token==100 && levels[1]==HIGH);
 fakeNow=start+10000;query(102);
 assert(state==bench::OFF && started==start && dedup.token==100);
 assert(benchRadio::outgoing.back().type==bench::STATUS && benchRadio::outgoing.back().state==bench::OFF);
 benchRadio::inject({bench::SET,2,bench::FULL,bench::APPLIED,100});loop();fakeNow+=50;loop();
 assert(state==bench::OFF && benchRadio::outgoing.back().result==bench::EXPIRED);
 auto n=benchRadio::outgoing.size();
 benchRadio::inject({bench::QUERY,1,bench::OFF,bench::APPLIED,103});loop();fakeNow+=50;loop();
 assert(benchRadio::outgoing.size()==n);
 // Poll straddling an automatic OFF reports the state at response time.
 benchRadio::inject({bench::SET,2,bench::ACTIVE,bench::APPLIED,104});loop();fakeNow+=50;loop();
 fakeNow=started+9990;query(105);
 assert(benchRadio::outgoing.back().state==bench::OFF && benchRadio::outgoing.back().result==bench::APPLIED);
 std::cout<<"PASS: real node QUERY never cancels boot, resets timer, changes pins, or destroys SET dedup; timeout edge and node isolation\n";
}
