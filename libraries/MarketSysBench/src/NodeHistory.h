#pragma once
#include "RoutingStore.h"
namespace startup {
// Node-local journal; controller routing pages are on a different physical board.
struct History { uint32_t sequence=0,bootId=0,auction=0,token=0; uint8_t state=bench::OFF,node=0; bool valid=false; };
template<class Flash> class HistoryStore {
 Flash &flash; int slot=-1;
 static void encode(const History&r,uint8_t*b){
  memset(b,0,32);memcpy(b,"MSN1",4);routing::put32(b+4,r.sequence);routing::put32(b+8,r.bootId);
  routing::put32(b+12,r.auction);routing::put32(b+16,r.token);b[20]=r.state;b[21]=r.node;b[22]=r.valid;
  auto c=bench::crc(b,30);b[30]=uint8_t(c);b[31]=uint8_t(c>>8);
 }
 static bool decode(const uint8_t*b,History&r,uint8_t node){
  if(memcmp(b,"MSN1",4)||bench::crc(b,30)!=uint16_t(b[30]|uint16_t(b[31])<<8)||
     !bench::validState(b[20])||b[21]!=node||b[22]>1)return false;
  for(int i=23;i<30;++i)if(b[i])return false;
  r.sequence=routing::get32(b+4);r.bootId=routing::get32(b+8);r.auction=routing::get32(b+12);
  r.token=routing::get32(b+16);r.state=b[20];r.node=b[21];r.valid=b[22];
  return !r.valid || r.token!=0;
 }
public:
 explicit HistoryStore(Flash&f):flash(f){}
 bool save(History&r){
  History next=r;next.sequence++;uint8_t data[32],check[32];encode(next,data);int target=slot==0?1:0;
  if(!flash.set(target*routing::SLOT_BYTES,data,32)||!flash.get(target*routing::SLOT_BYTES,check,32)||memcmp(data,check,32))return false;
  slot=target;r=next;return true;
 }
 bool boot(History&r,uint8_t node){
  uint8_t a[32],b[32];History ra,rb;
  if(!flash.get(0,a,32)||!flash.get(routing::SLOT_BYTES,b,32))return false;
  bool va=decode(a,ra,node),vb=decode(b,rb,node);
  if(va||vb){slot=(!va||(vb&&int32_t(rb.sequence-ra.sequence)>0))?1:0;r=slot?rb:ra;}
  else {for(int i=0;i<32;++i)if(a[i]!=255||b[i]!=255)return false;r=History();r.node=node;}
  if(r.bootId==UINT32_MAX)return false;
  ++r.bootId;return save(r);
 }
};
enum Decision { COLLECTING, RESUME, NEW_AUCTION, CONFLICT, NO_RESPONSE };
struct Assessment {
 uint8_t replies=0,survivors=0;uint32_t maxAuction=0;bool conflict=false,storageFault=false;
 void observe(const bench::Frame&f,uint32_t auction,uint8_t desired){
  ++replies;if(f.auction>maxAuction)maxAuction=f.auction;
  if(!(f.flags&bench::HISTORY_STORAGE_OK)||!f.bootId||f.result==bench::STORAGE_ERROR){conflict=storageFault=true;return;}
  if(f.flags&bench::APPLIED_THIS_BOOT){
   if(!(f.flags&bench::HISTORY_VALID)||!f.lastToken||f.auction!=auction||f.lastState!=desired||f.state!=desired)conflict=true;
   else ++survivors;
  } else if(f.state!=bench::OFF)conflict=true;
 }
 Decision decision()const{return conflict?CONFLICT:survivors?RESUME:replies?NEW_AUCTION:NO_RESPONSE;}
};
}
