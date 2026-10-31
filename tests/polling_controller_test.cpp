#include <cassert>
#include <iostream>
#include "../firmware/radio_central/radio_central.ino"
static int call(const char *name, std::initializer_list<const char*> args) {
 stParam p{};for(auto a:args)p.argv[p.argc++]=const_cast<char*>(a);
 return api.system.atMode.handlers.at(name)(0,nullptr,&p);
}
static void answer(uint8_t state) {
 loop(); auto f=request;
 benchRadio::inject({bench::STATUS,f.node,state,bench::APPLIED,f.token});loop();
}
int main(){
 setup(); assert(ready && api.system.atMode.handlers.size()==6);
 assert(call("POLL_LAST",{"1"})==AT_OK);
 assert(Serial.out.find("CONFIRMED=UNKNOWN;KNOWN=0")!=std::string::npos);
 assert(call("POLL",{"0","1"})==AT_PARAM_ERROR);
 assert(call("POLL",{"1","51"})==AT_PARAM_ERROR);
 assert(call("POLL",{"77","2"})==AT_OK);
 assert(call("PEN",{"1","1","ACTIVE"})==AT_ERROR);
 loop();
 benchRadio::inject({bench::ACK,2,bench::ACTIVE,bench::APPLIED,77});loop();assert(pending);
 benchRadio::inject({bench::STATUS,1,bench::ACTIVE,bench::APPLIED,77});loop();assert(pending);
 benchRadio::inject({bench::STATUS,2,bench::ACTIVE,bench::APPLIED,78});loop();assert(pending);
 benchRadio::inject({bench::STATUS,2,bench::FULL,bench::APPLIED,77});loop();
 assert(!pending && confirmations.get(2).state==bench::FULL);
 assert(!commanded.get(2).known);
 auto count=benchRadio::outgoing.size();
 assert(call("POLL_LAST",{"2"})==AT_OK);assert(call("POLL_ALL_LAST",{"?"})==AT_OK);
 assert(benchRadio::outgoing.size()==count);
 assert(call("POLL",{"78","2"})==AT_OK);
 for(int i=0;i<20 && pending;++i){fakeNow+=2100;loop();}
 assert(!pending && attempts==3 && confirmations.get(2).state==bench::FULL);
 assert(Serial.out.find("RESULT=TIMEOUT;STATE=UNKNOWN")!=std::string::npos);
 assert(call("POLL_ALL",{"4294967295"})==AT_OK);
 answer(bench::ACTIVE);assert(request.node==2 && request.token==1);
 // Alternate replies and timeouts; a missing node must not stop the scan.
 while(scanning){
   if(request.node%2){answer(bench::OFF);}
   else {auto n=request.node;while(scanning && request.node==n){fakeNow+=2100;loop();}}
 }
 assert(scanOk==25 && scanFailed==25 && !pending);
 assert(Serial.out.find("MS_POLL_ALL=DONE;OK=25;FAILED=25;COUNT=50")!=std::string::npos);
 for(const auto &f:benchRadio::outgoing)assert(f.type==bench::QUERY);
 assert(call("PEN",{"90","2","FULL"})==AT_OK);loop();
 benchRadio::inject({bench::STATUS,2,bench::FULL,bench::APPLIED,90});loop();assert(pending);
 benchRadio::inject({bench::ACK,2,bench::FULL,bench::APPLIED,90});loop();
 assert(!pending && commanded.get(2).state==bench::FULL);
 std::cout<<"PASS: real controller handlers, live/cache isolation, wrong replies, retries, all-50 scan, token wrap, SET regression\n";
}
