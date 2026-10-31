#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <cstdarg>
#include <map>
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
struct { struct {SetMock lpm,firmwareVersion; AtMock atMode;} system;} api;
