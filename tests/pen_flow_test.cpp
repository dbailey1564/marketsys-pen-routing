#include "../libraries/MarketSysBench/src/PenFlow.h"
#include <cassert>
#include <iostream>
int main(){
 routing::Record r,next;for(int n=1;n<=50;++n)r.configured[n]=1;
 assert(routing::plan(r,1,next) && next.active==1 && next.last==0 && next.desired[1]==bench::ACTIVE);
 next.flowPending=false;r=next;
 assert(routing::plan(r,1,next) && next.desired[1]==bench::HOLD_OPEN && next.last==0);
 next.flowPending=false;r=next;
 assert(routing::plan(r,2,next) && next.desired[1]==bench::LAST && next.desired[2]==bench::ACTIVE);
 next.flowPending=false;r=next;
 assert(routing::plan(r,3,next));
 uint8_t original[3];assert(routing::flowNodes(next,original)==3 && original[0]==3 && original[1]==2 && original[2]==1);
 assert(routing::plan(r,2,next) && next.desired[2]==bench::HOLD_OPEN && next.desired[1]==bench::OFF && next.last==0);
 uint8_t nodes[3];assert(routing::flowNodes(next,nodes)==2 && nodes[0]==2 && nodes[1]==1);
 next.flowPending=false;r=next;
 assert(routing::plan(r,3,next) && next.desired[1]==bench::OFF && next.desired[2]==bench::LAST && next.desired[3]==bench::ACTIVE);
 assert(routing::flowNodes(next,nodes)==2 && nodes[0]==3 && nodes[1]==2);
 next.flowPending=false;r=next;
 assert(routing::plan(r,2,next) && next.active==2 && next.last==3 && next.desired[2]==bench::ACTIVE);
 assert(routing::flowNodes(next,nodes)==2 && nodes[0]==2 && nodes[1]==3);
 for(int a=1;a<=50;++a)for(int b=1;b<=50;++b){if(a==b)continue;
  routing::Record base;base.active=a;base.last=b;
  for(int n=1;n<=50;++n)base.configured[n]=1;
  base.desired[a]=bench::ACTIVE;base.desired[b]=bench::LAST;
  assert(routing::plan(base,a,next) && next.active==a && next.last==0 && next.desired[a]==bench::HOLD_OPEN && next.desired[b]==bench::OFF);
  assert(routing::flowNodes(next,nodes)==2 && nodes[0]==a && nodes[1]==b);
  next.flowPending=false;routing::Record again;
  assert(routing::plan(next,a,again) && again.last==0 && again.desired[b]==bench::OFF);
  assert(routing::flowNodes(again,nodes)==1 && nodes[0]==a);
  assert(routing::plan(base,b,next));assert(next.active==b && next.last==a && next.desired[b]==bench::ACTIVE && next.desired[a]==bench::LAST);
 }
 assert(!routing::plan(r,0,next) && !routing::plan(r,51,next));
 r.configured[4]=0;assert(!routing::plan(r,4,next));r.configured[4]=1;
 r.desired[4]=bench::FULL;assert(!routing::plan(r,4,next));
 assert(routing::plan(r,1,next) && next.desired[4]==bench::FULL);
 r.pendingReset=true;assert(!routing::plan(r,1,next));
 std::cout<<"PASS: controller flow rules, repeated HOLD_OPEN, all 2450 return-to-LAST pairs, no stale OFF, unrelated FULL preservation, validation\n";
}
