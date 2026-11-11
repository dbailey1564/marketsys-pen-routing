#pragma once
#include <stdint.h>
#include <stddef.h>

namespace bench {
// 0x03 was STANDBY and is retired. Never renumber the surviving states.
enum State { OFF = 0, ACTIVE = 1, LAST = 2, HOLD_OPEN = 4, FULL = 5, FAULT = 6 };
inline bool validState(uint8_t state) { return state <= FAULT && state != 3; }
enum Type { SET = 1, ACK = 2, QUERY = 3, STATUS = 4, REBOOT = 5, REBOOT_ACK = 6, SLEEP = 7, WAKE = 8, POWER_ACK = 9 };
enum Result { APPLIED, DUPLICATE, EXPIRED, CONFLICT, BOOTING, STORAGE_ERROR };
static const size_t SIZE = 34;
struct Frame { uint8_t type, node, state, result; uint32_t token; uint32_t auction=0, lastToken=0, bootId=0, uptime=0; uint8_t lastState=OFF, flags=0;
 Frame(uint8_t t=0,uint8_t n=0,uint8_t s=OFF,uint8_t r=APPLIED,uint32_t k=0):type(t),node(n),state(s),result(r),token(k){}
};
enum HistoryFlags { HISTORY_VALID=1, APPLIED_THIS_BOOT=2, HISTORY_STORAGE_OK=4, SLEEP_ENABLED=8 };
inline bool powerRequest(uint8_t type) { return type == SLEEP || type == WAKE; }
static const uint32_t WAKE_PERIOD_MS = 120000, RX_WINDOW_MS = 5000;
static const uint32_t WAKE_RETRY_MS = 2000, WAKE_LIMIT_MS = 300000;
inline const char *name(uint8_t state) {
    switch (state) {
    case ACTIVE: return "ACTIVE"; case LAST: return "LAST";
    case HOLD_OPEN: return "HOLD_OPEN";
    case FULL: return "FULL"; case FAULT: return "FAULT";
    case OFF: return "OFF";
    default: return "INVALID";
    }
}
inline uint16_t crc(const uint8_t *data, size_t length) {
    uint16_t value = 0xffff;
    for (size_t i = 0; i < length; ++i) {
        value ^= uint16_t(data[i]) << 8;
        for (int bit = 0; bit < 8; ++bit)
            value = (value & 0x8000) ? uint16_t((value << 1) ^ 0x1021) : uint16_t(value << 1);
    }
    return value;
}
inline void encode(const Frame &f, uint8_t *data) {
    data[0] = 'M'; data[1] = 'S'; data[2] = 2; data[3] = f.type;
    data[4] = f.node; data[5] = f.state; data[6] = f.result; data[7] = 0;
    for (int i = 0; i < 4; ++i) data[8+i] = uint8_t(f.token >> (8*i));
    const uint32_t extra[]={f.auction,f.lastToken,f.bootId,f.uptime};
    for(int k=0;k<4;++k)for(int i=0;i<4;++i)data[12+4*k+i]=uint8_t(extra[k]>>(8*i));
    data[28]=f.lastState;data[29]=f.flags;data[30]=data[31]=0;
    uint16_t check = crc(data, SIZE-2);
    data[SIZE-2] = uint8_t(check); data[SIZE-1] = uint8_t(check >> 8);
}
inline bool decode(const uint8_t *data, size_t length, Frame &f) {
    if (length != SIZE || data[0] != 'M' || data[1] != 'S' || data[2] != 2 ||
        (data[3] != SET && data[3] != ACK && data[3] != QUERY && data[3] != STATUS && data[3] != REBOOT && data[3] != REBOOT_ACK && !powerRequest(data[3]) && data[3] != POWER_ACK) || (data[4] == 0 || data[4] > 50) || !validState(data[5]) ||
        data[6] > STORAGE_ERROR || (data[3] == ACK && data[6] > CONFLICT && data[6] != STORAGE_ERROR) || data[7] != 0 || ((data[3] == SET || data[3] == QUERY || data[3] == REBOOT || data[3] == REBOOT_ACK) && data[6] != APPLIED) ||
        ((data[3] == QUERY || data[3] == REBOOT || data[3] == REBOOT_ACK) && data[5] != OFF) ||
        (data[3] == STATUS && data[6] != APPLIED && data[6] != BOOTING && data[6] != STORAGE_ERROR) ||
        (!validState(data[28]) || (data[29]&~15) || data[30] || data[31]) ||
        ((powerRequest(data[3]) || data[3] == POWER_ACK) && (data[5] != OFF ||
         (powerRequest(data[3]) && data[6] != APPLIED) ||
         (data[3] == POWER_ACK && data[6] != APPLIED && data[6] != CONFLICT))) ||
        crc(data, SIZE-2) != uint16_t(data[SIZE-2] | uint16_t(data[SIZE-1]) << 8)) return false;
    f.type = data[3]; f.node = data[4]; f.state = data[5]; f.result = data[6];
    f.token = 0;
    for (int i = 0; i < 4; ++i) f.token |= uint32_t(data[8+i]) << (8*i);
    uint32_t *extra[]={&f.auction,&f.lastToken,&f.bootId,&f.uptime};
    for(int k=0;k<4;++k){*extra[k]=0;for(int i=0;i<4;++i)*extra[k]|=uint32_t(data[12+4*k+i])<<(8*i);}
    f.lastState=data[28];f.flags=data[29];
    return f.token != 0;
}
struct Dedup {
    bool valid = false;
    uint32_t token = 0, auction = 0;
    uint8_t requested = OFF;
    Result classify(const Frame &f, uint8_t current) const {
        if (!valid || f.token != token || f.auction != auction) return APPLIED;
        if (f.state != requested) return CONFLICT;
        return current == requested ? DUPLICATE : EXPIRED;
    }
    void remember(const Frame &f) { valid = true; token = f.token; auction = f.auction; requested = f.state; }
};
}
