#pragma once
#include <stdint.h>
#include <stddef.h>

namespace bench {
// 0x03 was STANDBY and is retired. Never renumber the surviving states.
enum State { OFF = 0, ACTIVE = 1, LAST = 2, HOLD_OPEN = 4, FULL = 5, FAULT = 6 };
inline bool validState(uint8_t state) { return state <= FAULT && state != 3; }
enum Type { SET = 1, ACK = 2 };
enum Result { APPLIED, DUPLICATE, EXPIRED, CONFLICT };
static const size_t SIZE = 14;
struct Frame { uint8_t type, node, state, result; uint32_t token; };
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
    data[0] = 'M'; data[1] = 'S'; data[2] = 1; data[3] = f.type;
    data[4] = f.node; data[5] = f.state; data[6] = f.result; data[7] = 0;
    for (int i = 0; i < 4; ++i) data[8+i] = uint8_t(f.token >> (8*i));
    uint16_t check = crc(data, 12);
    data[12] = uint8_t(check); data[13] = uint8_t(check >> 8);
}
inline bool decode(const uint8_t *data, size_t length, Frame &f) {
    if (length != SIZE || data[0] != 'M' || data[1] != 'S' || data[2] != 1 ||
        (data[3] != SET && data[3] != ACK) || data[4] == 0 || !validState(data[5]) ||
        data[6] > CONFLICT || data[7] != 0 || (data[3] == SET && data[6] != APPLIED) ||
        crc(data, 12) != uint16_t(data[12] | uint16_t(data[13]) << 8)) return false;
    f.type = data[3]; f.node = data[4]; f.state = data[5]; f.result = data[6];
    f.token = 0;
    for (int i = 0; i < 4; ++i) f.token |= uint32_t(data[8+i]) << (8*i);
    return f.token != 0;
}
struct Dedup {
    bool valid = false;
    uint32_t token = 0;
    uint8_t requested = OFF;
    Result classify(const Frame &f, uint8_t current) const {
        if (!valid || f.token != token) return APPLIED;
        if (f.state != requested) return CONFLICT;
        return current == requested ? DUPLICATE : EXPIRED;
    }
    void remember(const Frame &f) { valid = true; token = f.token; requested = f.state; }
};
}
