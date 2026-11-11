#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <cstdarg>
#include <map>
#include <cstring>
static uint32_t fakeNow = 0;
inline uint32_t millis() { return fakeNow; }
inline void delay(uint32_t ms) { fakeNow += ms; }
static int levels[3] = {};
enum {PB3=0, PB4=1, PB5=2, LOW=0, HIGH=1, OUTPUT=1, RAK_AT_MODE=1,
 AT_OK=0, AT_ERROR=-1, AT_PARAM_ERROR=-2, RAK_ATCMD_PERM_READ=1, RAK_ATCMD_PERM_WRITE=2};
#define SERIAL_PORT int
struct stParam { int argc; char *argv[4]; };
inline void digitalWrite(uint32_t p,int v){levels[p]=v;}
inline int digitalRead(uint32_t p){return levels[p];}
inline void pinMode(uint32_t,int){}
struct SerialMock {
 std::string out;
 void begin(int,int){}
 void println(const char*s){out+=s;out+='\n';}
 void printf(const char*fmt,...) {char b[2048]; va_list a;va_start(a,fmt);vsnprintf(b,sizeof b,fmt,a);va_end(a);out+=b;}
} Serial;
struct SetMock { void set(int){}; void set(const char*){}; };
struct AtMock {
 std::map<std::string,int(*)(int,char*,stParam*)> handlers;
 bool add(char*n,char*,char*,int(*f)(int,char*,stParam*),int){handlers[n]=f;return true;}
};
struct FlashMock {
 uint8_t bytes[4096];bool fail=false,tear=false,readFail=false;int writes=0;
 FlashMock(){memset(bytes,255,sizeof bytes);}
 bool get(uint32_t offset,uint8_t*b,uint32_t n){if(readFail||offset+n>sizeof bytes)return false;memcpy(b,bytes+offset,n);return true;}
 bool set(uint32_t offset,uint8_t*b,uint32_t n){
  ++writes;if(fail||offset+n>sizeof bytes)return false;
  memset(bytes+(offset/2048)*2048,255,2048);
  if(tear){memcpy(bytes+offset,b,n/2);return false;}
  memcpy(bytes+offset,b,n);return true;
 }
};
struct { struct {SetMock firmwareVersion; struct {float get(){return 3.275f;}} bat; struct {bool set(uint8_t){return true;}} lpm; struct {bool set(uint8_t){return true;} uint8_t get(){return 1;}} lpmlvl; struct {uint32_t duration=0; void all(uint32_t ms){duration=ms;}} sleep; AtMock atMode;FlashMock flash;int rebootCount=0;
 void reboot(){++rebootCount;}
 } system;} api;
