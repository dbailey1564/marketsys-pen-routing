#pragma once
#include "RoutingStore.h"

namespace host {
struct Destination {
    uint32_t session, transaction;
    uint8_t pen, alpha;
};

class Receiver {
    static constexpr size_t SIZE = 16;
    uint8_t bytes[SIZE];
    size_t used = 0;
public:
    enum Result { MORE, VALID, INVALID };
    Result feed(uint8_t byte, Destination &out) {
        if (!used && byte != 0xaa) return MORE;
        if (used == 1 && byte != 0x55) {
            used = byte == 0xaa ? 1 : 0;
            return MORE;
        }
        bytes[used++] = byte;
        if (used != SIZE) return MORE;
        const uint16_t crc = uint16_t(bytes[14] | uint16_t(bytes[15]) << 8);
        if (bytes[2] != 2 || bytes[3] != 0x10 || bench::crc(bytes+2,12) != crc) {
            // Keep the earliest possible next sync after the rejected prefix.
            size_t start=1;
            while (start < SIZE && (bytes[start]!=0xaa ||
                   (start+1 < SIZE && bytes[start+1]!=0x55))) ++start;
            used=SIZE-start;
            memmove(bytes,bytes+start,used);
            return INVALID;
        }
        used=0;
        out={routing::get32(bytes+4),routing::get32(bytes+8),bytes[12],bytes[13]};
        return VALID;
    }
};
}
