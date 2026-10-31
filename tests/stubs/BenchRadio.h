#pragma once
#include <BenchProtocol.h>
#include <vector>
namespace benchRadio {
static bool sent=false, has=false, queueOk=true;
static int lastRssi=-50,lastSnr=9;
static bench::Frame incoming;
static std::vector<bench::Frame> outgoing;
inline bool begin(){return true;}
inline bool take(bench::Frame &f){if(!has)return false;f=incoming;has=false;return true;}
inline bool send(const bench::Frame&f){outgoing.push_back(f);sent=queueOk;return queueOk;}
inline void printConfig(const char*,bool,const char*){(void)lastRssi;(void)lastSnr;}
inline void inject(bench::Frame f){incoming=f;has=true;}
}
