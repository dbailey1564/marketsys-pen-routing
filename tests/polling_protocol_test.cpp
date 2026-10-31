#include "../libraries/MarketSysBench/src/BenchAddress.h"
#include <cassert>
#include <iostream>
int main(){
 for(uint8_t id=1;id<=50;++id){
  bench::Frame q={bench::QUERY,id,bench::OFF,bench::APPLIED,123},r;
  uint8_t wire[bench::SIZE];bench::encode(q,wire);assert(bench::decode(wire,sizeof wire,r));
  for(uint8_t other=1;other<=50;++other)assert(bench::forNode(r,other)==(id==other));
  for(uint8_t s=0;s<=6;++s){if(!bench::validState(s))continue;
   bench::Frame reply={bench::STATUS,id,s,bench::APPLIED,123};bench::encode(reply,wire);
   assert(bench::decode(wire,sizeof wire,r) && bench::matchesStatus(r,q));
   assert(!bench::matchesAck(r,q));
  }
  q.state=bench::ACTIVE;bench::encode(q,wire);assert(!bench::decode(wire,sizeof wire,r));
  q.state=bench::OFF;q.result=bench::EXPIRED;bench::encode(q,wire);assert(!bench::decode(wire,sizeof wire,r));
 }
 std::cout<<"PASS: QUERY/STATUS codec, address isolation, every current state, invalid query fields\n";
}
