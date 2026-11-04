#include <cassert>
#include <iostream>
#include "../firmware/radio_central/radio_central.ino"
static uint8_t hardware[51]={};
static int call(const char*n,std::initializer_list<const char*>args){stParam p{};for(auto a:args)p.argv[p.argc++]=const_cast<char*>(a);return api.system.atMode.handlers.at(n)(0,nullptr,&p);}
static void drive(uint8_t missing=0){
 int count=0;
 while((task==FLOW || (routingRecord.flowPending&&!routingRecord.flowFailed)) && ++count<1000){
  loop();
  if(pending && attempts && request.node!=missing){
   if(request.type==bench::SET)hardware[request.node]=request.state;
   benchRadio::inject({uint8_t(request.type==bench::QUERY?bench::STATUS:bench::ACK),request.node,hardware[request.node],bench::APPLIED,request.token});loop();
  }
  fakeNow+=100;
 }
 assert(count<1000);
}
int main(){
 setup();maintenanceAt=fakeNow+1000000;
 assert(call("DEST",{"1"})==AT_OK && routingRecord.flowPending);
 routing::Store<FlashMock> disk(api.system.flash);routing::Record saved;
 assert(disk.load(saved) && saved.flowPending && saved.active==1);
 drive();assert(hardware[1]==bench::ACTIVE && !routingRecord.flowPending);
 assert(call("DEST",{"1"})==AT_OK);drive();assert(hardware[1]==bench::HOLD_OPEN);
 assert(call("DEST",{"2"})==AT_OK);drive();assert(hardware[1]==bench::LAST && hardware[2]==bench::ACTIVE);
 auto before=benchRadio::outgoing.size();
 assert(call("DEST",{"3"})==AT_OK);drive(3);
 assert(routingRecord.flowFailed && routingRecord.flowPending && routingRecord.faults[3]);
 for(size_t i=before;i<benchRadio::outgoing.size();++i)assert(benchRadio::outgoing[i].node==3);
 auto total=benchRadio::outgoing.size();fakeNow+=60000;for(int i=0;i<20;++i)loop();
 assert(benchRadio::outgoing.size()==total); // ordinary maintenance cannot bypass pause.
 assert(call("DEST",{"1"})==AT_ERROR);
 // Restart loads a paused operation, never treats it as another head.
 pending=false;task=NO_TASK;taskInFlight=false;routingRecord=routing::Record();setup();
 assert(routingRecord.flowPending && routingRecord.flowFailed);
 loop();assert(!pending);
 assert(call("FLOW",{"RETRY"})==AT_OK);drive();
 assert(hardware[1]==bench::OFF && hardware[2]==bench::LAST && hardware[3]==bench::ACTIVE);
 assert(!routingRecord.flowPending && disk.load(saved) && !saved.flowPending && saved.active==3 && saved.last==2);
 assert(call("DEST",{"2"})==AT_OK);
 // Reset after target becomes ACTIVE but before the old ACTIVE becomes LAST.
 int guard=0;
 while(routingRecord.flowStep==0 && ++guard<100){
  loop();
  if(pending && attempts){
   if(request.type==bench::SET)hardware[request.node]=request.state;
   benchRadio::inject({uint8_t(request.type==bench::QUERY?bench::STATUS:bench::ACK),request.node,hardware[request.node],bench::APPLIED,request.token});loop();
  }
  fakeNow+=10;
 }
 assert(guard<100 && routingRecord.flowPending && routingRecord.flowStep==1);
 pending=false;task=NO_TASK;taskInFlight=false;routingRecord=routing::Record();setup();
 assert(routingRecord.flowPending && !routingRecord.flowFailed && routingRecord.flowStep==1);
 drive();assert(hardware[2]==bench::ACTIVE && hardware[3]==bench::LAST);
 int writes=api.system.flash.writes;
 assert(call("DEST",{"0"})==AT_PARAM_ERROR && call("DEST",{"4"})==AT_PARAM_ERROR);
 assert(api.system.flash.writes==writes);
 api.system.flash.fail=true;
 assert(call("DEST",{"1"})==AT_ERROR && !pending && !storageOk);
 std::cout<<"PASS: actual destination handler, durable progression, failure pause, no maintenance bypass, restart/retry, return-to-LAST, invalid inputs and failed-save rejection\n";
}
