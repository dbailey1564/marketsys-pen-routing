#pragma once
#include "RoutingStore.h"
namespace routing {
// Explicit bench invocation is a new head; app event identity is not defined here.
inline uint8_t flowNodes(const Record&r,uint8_t*nodes){
 uint8_t count=0;nodes[count++]=r.active;
 if(r.flowOldActive && r.flowOldActive!=r.active)nodes[count++]=r.flowOldActive;
 if(r.flowOldLast && r.flowOldLast!=r.active && r.flowOldLast!=r.flowOldActive)
  nodes[count++]=r.flowOldLast;
 return count;
}
inline bool plan(const Record&before,uint8_t destination,Record&after){
 if(!bench::validNode(destination)||!before.configured[destination]||before.pendingReset||before.flowPending)return false;
 if(before.desired[destination]==bench::FULL||before.desired[destination]==bench::FAULT)return false;
 if(before.active && (!before.configured[before.active] ||
    (before.desired[before.active]!=bench::ACTIVE && before.desired[before.active]!=bench::HOLD_OPEN)))return false;
 if(before.last && (!before.configured[before.last] || before.desired[before.last]!=bench::LAST || before.last==before.active))return false;
 after=before;
 after.flowOldActive=before.active;after.flowOldLast=before.last;after.flowStep=0;
 after.flowPending=true;after.flowFailed=false;
 if(destination==before.active){
  after.desired[destination]=bench::HOLD_OPEN;
  if(before.last)after.desired[before.last]=bench::OFF;
  after.last=0;
  return true;
 }
 if(before.last && before.last!=destination)after.desired[before.last]=bench::OFF;
 if(before.active)after.desired[before.active]=bench::LAST;
 after.active=destination;after.last=before.active;after.desired[destination]=bench::ACTIVE;
 return true;
}
}
