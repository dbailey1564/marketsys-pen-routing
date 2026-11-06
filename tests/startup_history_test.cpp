#include "stubs/Arduino.h"
#include "../libraries/MarketSysBench/src/NodeHistory.h"
#include <cassert>
#include <iostream>
int main(){
 FlashMock flash;startup::History h;startup::HistoryStore<FlashMock> store(flash);
 assert(store.boot(h,1)&&h.bootId==1&&!h.valid);
 h.auction=7;h.token=123;h.state=bench::ACTIVE;h.valid=true;assert(store.save(h));
 startup::HistoryStore<FlashMock> restart(flash);startup::History restored;
 assert(restart.boot(restored,1)&&restored.bootId==2&&restored.token==123&&restored.state==bench::ACTIVE);
 flash.tear=true;restored.state=bench::FULL;assert(!restart.save(restored));flash.tear=false;
 startup::HistoryStore<FlashMock> afterTear(flash);assert(afterTear.boot(restored,1)&&restored.state==bench::ACTIVE);
 startup::HistoryStore<FlashMock> wrongNode(flash);assert(!wrongNode.boot(restored,2));
 FlashMock corrupt;corrupt.bytes[0]=0;startup::HistoryStore<FlashMock> bad(corrupt);int writes=corrupt.writes;
 assert(!bad.boot(restored,1)&&corrupt.writes==writes);
 startup::Assessment a;assert(a.decision()==startup::NO_RESPONSE);
 bench::Frame f={bench::STATUS,1,bench::OFF,bench::APPLIED,3};
 f.bootId=1;f.flags=bench::HISTORY_STORAGE_OK;
 a.observe(f,7,bench::ACTIVE);assert(a.decision()==startup::NEW_AUCTION);
 // Rebooted node can retain an old indication in history but must report current OFF.
 f.auction=7;f.lastToken=123;f.lastState=bench::ACTIVE;f.flags|=bench::HISTORY_VALID;
 a=startup::Assessment();a.observe(f,7,bench::ACTIVE);assert(a.decision()==startup::NEW_AUCTION);
 f.flags|=bench::APPLIED_THIS_BOOT;f.state=bench::ACTIVE;
 a=startup::Assessment();a.observe(f,7,bench::ACTIVE);assert(a.decision()==startup::RESUME);
 bench::Frame fresh=f;fresh.flags=bench::HISTORY_STORAGE_OK;fresh.state=bench::OFF;
 a.observe(fresh,7,bench::LAST);assert(a.decision()==startup::RESUME);
 f.auction=8;a.observe(f,7,bench::ACTIVE);assert(a.decision()==startup::CONFLICT);
 for(int scenario=0;scenario<4;++scenario){
  f.auction=7;f.flags=bench::HISTORY_STORAGE_OK|bench::HISTORY_VALID|bench::APPLIED_THIS_BOOT;f.lastToken=123;f.lastState=f.state=bench::ACTIVE;
  if(scenario==0)f.lastState=bench::FULL;
  if(scenario==1)f.state=bench::OFF;
  if(scenario==2)f.flags&=~bench::HISTORY_STORAGE_OK;
  if(scenario==3)f.lastToken=0;
  a=startup::Assessment();a.observe(f,7,bench::ACTIVE);assert(a.decision()==startup::CONFLICT);
 }
 uint8_t bytes[bench::SIZE];bench::Frame decoded;f.flags=7;f.lastToken=999;f.uptime=UINT32_MAX;f.bootId=42;
 bench::encode(f,bytes);assert(bench::decode(bytes,sizeof bytes,decoded));
 assert(decoded.lastToken==999&&decoded.uptime==UINT32_MAX&&decoded.bootId==42&&decoded.flags==7);
 for(size_t bit=0;bit<sizeof bytes*8;++bit){bytes[bit/8]^=1<<(bit%8);assert(!bench::decode(bytes,sizeof bytes,decoded));bytes[bit/8]^=1<<(bit%8);}
 bytes[2]=1;assert(!bench::decode(bytes,sizeof bytes,decoded));
 std::cout<<"PASS: node journal, torn write, boot identity, startup decisions, extended CRC-protected status\n";
}
