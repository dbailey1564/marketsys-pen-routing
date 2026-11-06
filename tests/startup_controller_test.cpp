#include <cassert>
#include <iostream>
#include "../firmware/radio_central/radio_central.ino"
static int command(const char*name,std::initializer_list<const char*>args){stParam p{};for(auto a:args)p.argv[p.argc++]=const_cast<char*>(a);return api.system.atMode.handlers.at(name)(0,nullptr,&p);}
static void start(){setup();benchRadio::outgoing.clear();fakeNow=taskAt;loop();loop();assert(task==STARTUP&&pending&&request.type==bench::QUERY);}
static void reply(bool survivor,bool mismatch=false){
 bench::Frame f={bench::STATUS,request.node,uint8_t(survivor?routingRecord.desired[request.node]:uint8_t(bench::OFF)),bench::APPLIED,request.token};
 f.auction=routingRecord.auction+(mismatch?1:0);f.lastToken=123;f.lastState=f.state;f.bootId=2;f.uptime=600000;
 f.flags=bench::HISTORY_STORAGE_OK|(survivor?(bench::HISTORY_VALID|bench::APPLIED_THIS_BOOT):0);
 benchRadio::inject(f);loop();loop();loop();
}
static void noWrites(){for(auto f:benchRadio::outgoing)assert(f.type==bench::QUERY);}
int main(){
 // Preserve nonzero controller intent for every scenario.
 setup();routingRecord.auction=9;routingRecord.active=1;routingRecord.desired[1]=bench::ACTIVE;assert(persist());
 start();assert(command("DEST",{"2"})==AT_ERROR);noWrites();
 bench::Frame booting={bench::STATUS,1,bench::ACTIVE,bench::BOOTING,request.token};
 benchRadio::inject(booting);loop();assert(task==STARTUP&&!pending&&taskNode==1);noWrites();
 fakeNow=taskAt;loop();loop();
 reply(true);reply(false);noWrites();reply(false);
 assert(startupDecision==startup::RESUME&&routingRecord.auction==9&&routingRecord.desired[1]==bench::ACTIVE);
 // A survivor with a conflicting auction pauses without overwriting node states.
 start();reply(true,true);reply(false);reply(false);noWrites();
 assert(startupDecision==startup::CONFLICT&&command("DEST",{"2"})==AT_ERROR);
 assert(routingRecord.auction==9&&routingRecord.desired[1]==bench::ACTIVE);
 // Every responding node restarted: bias to a durably saved new auction.
 start();reply(false);reply(false);noWrites();reply(false);
 assert(startupDecision==startup::NEW_AUCTION&&routingRecord.auction==10&&routingRecord.active==0&&routingRecord.pendingReset);
 routing::Store<FlashMock> reload(api.system.flash);routing::Record disk;assert(reload.load(disk)&&disk.pendingReset&&disk.desired[1]==bench::OFF);
 // Interrupted clear resumes the same auction rather than incrementing again.
 start();reply(true,true);reply(false);reply(false);assert(startupDecision==startup::RESUME&&routingRecord.auction==10);
 // No node replies: retain record and stop automatic writes, with saved faults.
 routingRecord.pendingReset=false;assert(persist());start();
 for(int i=0;i<100&&task==STARTUP;++i){fakeNow+=3000;loop();}
 noWrites();assert(startupDecision==startup::NO_RESPONSE&&routingRecord.auction==10&&routingRecord.faults[1]);
 assert(command("DEST",{"1"})==AT_ERROR);
 assert(command("AUCTION",{"CLEAR"})==AT_OK&&routingRecord.auction==11);
 // A failed save while selecting a new auction must leave previous intent intact.
 routingRecord.pendingReset=false;routingRecord.active=1;routingRecord.desired[1]=bench::ACTIVE;assert(persist());
 start();api.system.flash.fail=true;reply(false);reply(false);reply(false);noWrites();
 assert(!storageOk&&startupDecision==startup::CONFLICT&&routingRecord.auction==11&&routingRecord.desired[1]==bench::ACTIVE);
 std::cout<<"PASS: startup read-only barrier, survivor recovery, conflict block, saved new auction, interrupted clear, no-response fault, explicit clear\n";
}
