#include <cassert>
#include <iostream>
#include "../firmware/radio_central/radio_central.ino"
static int call(const char *name,std::initializer_list<const char*>args){stParam p{};for(auto a:args)p.argv[p.argc++]=const_cast<char*>(a);return api.system.atMode.handlers.at(name)(0,nullptr,&p);}
static void response(uint8_t state,uint8_t status=bench::APPLIED){
 benchRadio::inject({uint8_t(request.type==bench::QUERY?bench::STATUS:request.type==bench::REBOOT?bench::REBOOT_ACK:bench::ACK),request.node,state,status,request.token});loop();
}
int main(){
 setup();maintenanceEnabled=false;
 assert(storageOk && call("PEN",{"80","1","ACTIVE"})==AT_OK);loop();response(bench::ACTIVE);
 assert(routingRecord.desired[1]==bench::ACTIVE && routingRecord.active==1);
 routing::Store<FlashMock> reload(api.system.flash);routing::Record disk;assert(reload.load(disk) && disk.desired[1]==bench::ACTIVE);
 // Simulate controller startup state: no cached confirmations, but retained intent.
 confirmations=bench::Confirmations();commanded=bench::Confirmations();routingRecord=routing::Record();
 setup();maintenanceEnabled=true;fakeNow=maintenanceAt;loop();loop();
 assert(task==RECOVERY && request.node==1 && request.type==bench::QUERY);
 response(bench::ACTIVE,bench::BOOTING);assert(stage==WAIT_BOOT);
 fakeNow=taskAt;loop();loop();response(bench::OFF);assert(stage==NEED_SET);
 loop();loop();assert(request.type==bench::SET && request.state==bench::ACTIVE);response(bench::ACTIVE);
 // Finish reconciliation for the remaining two nodes.
 while(task!=NO_TASK){loop();if(pending)response(bench::OFF);fakeNow+=20;}
 assert(confirmations.get(1).state==bench::ACTIVE);
 maintenanceEnabled=false;
 assert(call("AUCTION",{"CLEAR"})==AT_OK && routingRecord.pendingReset && routingRecord.desired[1]==bench::OFF);
 assert(reload.load(disk) && disk.pendingReset && disk.active==0 && disk.desired[1]==bench::OFF);
 assert(call("PEN",{"81","1","FULL"})==AT_ERROR);
 maintenanceEnabled=true;int iterations=0;
 while(task!=NO_TASK && ++iterations<400){
  loop();
  if(pending && request.node!=2)response(request.type==bench::REBOOT?uint8_t(bench::OFF):request.state);
  fakeNow+=1100;
 }
 assert(iterations<400 && !routingRecord.pendingReset && routingRecord.faults[2] && taskFaults==1);
 int reboots=0;for(auto f:benchRadio::outgoing)if(f.type==bench::REBOOT){assert(f.node==2);++reboots;}
 assert(reboots==3 && Serial.out.find("MS_CONTROLLER_FAULT_NODE=2")!=std::string::npos);
 assert(reload.load(disk) && disk.faults[2] && !disk.pendingReset && disk.auction==1);
 maintenanceEnabled=false;api.system.flash.fail=true;
 assert(call("PEN",{"82","1","FULL"})==AT_ERROR && !storageOk);
 assert(routingRecord.desired[1]==bench::OFF);
 std::cout<<"PASS: durable controller intent, boot-aware restoration, saved clear before RF, bounded reboot, durable fault, failed save rejection\n";
}
