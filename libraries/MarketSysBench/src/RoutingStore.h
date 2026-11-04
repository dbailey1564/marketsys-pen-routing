#pragma once
#include "BenchAddress.h"
#include <string.h>
namespace routing {
// Separate STM32WL 2 KiB erase pages in the RUI3 user-data partition.
static const uint32_t SLOT_BYTES=2048;
static const size_t RECORD_BYTES=176;
struct Record {
 uint32_t sequence=0, auction=0;
 uint8_t active=0,last=0;
 bool pendingReset=false;
 bool flowPending=false,flowFailed=false;
 uint8_t flowOldActive=0,flowOldLast=0,flowStep=0;
 uint8_t desired[51]={}, configured[51]={}, faults[51]={};
};
inline void put32(uint8_t*p,uint32_t v){for(int i=0;i<4;++i)p[i]=uint8_t(v>>(8*i));}
inline uint32_t get32(const uint8_t*p){uint32_t v=0;for(int i=0;i<4;++i)v|=uint32_t(p[i])<<(8*i);return v;}
inline void encode(const Record&r,uint8_t*b){
 memset(b,0,RECORD_BYTES);memcpy(b,"MSR2",4);put32(b+4,r.sequence);put32(b+8,r.auction);
 b[12]=r.active;b[13]=r.last;
 memcpy(b+14,r.desired,51);memcpy(b+65,r.configured,51);memcpy(b+116,r.faults,51);
 b[167]=r.pendingReset;
 b[168]=r.flowPending;b[169]=r.flowFailed;b[170]=r.flowOldActive;b[171]=r.flowOldLast;b[172]=r.flowStep;
 auto c=bench::crc(b,174);b[174]=uint8_t(c);b[175]=uint8_t(c>>8);
}
inline bool decode(const uint8_t*b,Record&r){
 const bool legacy=!memcmp(b,"MSR1",4);
 if((!legacy && memcmp(b,"MSR2",4))||bench::crc(b,174)!=uint16_t(b[174]|uint16_t(b[175])<<8))return false;
 if(b[12]>50||b[13]>50||b[14]||b[65]||b[116])return false;
 for(int i=1;i<=50;++i)if(!bench::validState(b[14+i])||b[65+i]>1||b[116+i]>1)return false;
 if(b[167]>1)return false;
 if(legacy){for(int i=168;i<174;++i)if(b[i])return false;}
 else if(b[168]>1||b[169]>1||b[170]>50||b[171]>50||b[172]>3||b[173]||
         (b[169]&&!b[168])||(b[168]&&(!b[12]||b[167])))return false;
 r.sequence=get32(b+4);r.auction=get32(b+8);r.active=b[12];r.last=b[13];r.pendingReset=b[167];
 r.flowPending=b[168];r.flowFailed=b[169];r.flowOldActive=b[170];r.flowOldLast=b[171];r.flowStep=b[172];
 memcpy(r.desired,b+14,51);memcpy(r.configured,b+65,51);memcpy(r.faults,b+116,51);return true;
}
template<class Flash> class Store {
 Flash &flash;int slot=-1;
public:
 explicit Store(Flash&f):flash(f){}
 // Empty flash is initialized. Unrecognized/corrupt nonempty data is never erased.
 bool load(Record&r){
  uint8_t a[RECORD_BYTES],b[RECORD_BYTES];Record ra,rb;
  if(!flash.get(0,a,sizeof a)||!flash.get(SLOT_BYTES,b,sizeof b))return false;
  bool va=decode(a,ra),vb=decode(b,rb);
  if(va||vb){slot=(!va||(vb&&int32_t(rb.sequence-ra.sequence)>0))?1:0;r=slot?rb:ra;return true;}
  for(size_t i=0;i<RECORD_BYTES;++i)if(a[i]!=255||b[i]!=255)return false;
  r=Record();for(int n=1;n<=3;++n)r.configured[n]=1;
  return save(r);
 }
 bool save(Record&r){
  Record next=r;next.sequence++;
  uint8_t data[RECORD_BYTES],check[RECORD_BYTES];encode(next,data);
  int target=slot==0?1:0;
  if(!flash.set(target*SLOT_BYTES,data,sizeof data)||
     !flash.get(target*SLOT_BYTES,check,sizeof check)||memcmp(data,check,sizeof data))return false;
  slot=target;r.sequence=next.sequence;return true;
 }
};
}
