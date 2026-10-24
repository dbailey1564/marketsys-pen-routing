#include "../libraries/MarketSysBench/src/BenchProtocol.h"
#include "../libraries/MarketSysBench/src/BenchPattern.h"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
    const uint8_t standard[] = "123456789";
    assert(bench::crc(standard, 9) == 0x29b1); // CRC-16/CCITT-FALSE check value.
    uint8_t wire[bench::SIZE];
    bench::Frame original = {bench::SET, 1, bench::ACTIVE, bench::APPLIED, 0x12345678};
    bench::encode(original, wire);
    assert(wire[8] == 0x78 && wire[9] == 0x56 && wire[10] == 0x34 && wire[11] == 0x12);
    bench::Frame decoded;
    assert(bench::decode(wire, sizeof(wire), decoded));
    assert(decoded.token == original.token && decoded.state == original.state);
    for (size_t bit = 0; bit < bench::SIZE * 8; ++bit) {
        uint8_t damaged[bench::SIZE];
        memcpy(damaged, wire, sizeof(wire));
        damaged[bit / 8] ^= 1 << (bit % 8);
        assert(!bench::decode(damaged, sizeof(damaged), decoded));
    }
    assert(!bench::decode(wire, bench::SIZE - 1, decoded));
    assert(!bench::decode(wire, bench::SIZE + 1, decoded));
    for (int field : {2, 3, 4, 5, 6, 7}) {
        uint8_t invalid[bench::SIZE]; memcpy(invalid, wire, sizeof(wire));
        invalid[field] = field == 4 ? 0 : 255;
        uint16_t crc = bench::crc(invalid, 12);
        invalid[12] = uint8_t(crc); invalid[13] = uint8_t(crc >> 8);
        assert(!bench::decode(invalid, sizeof(invalid), decoded));
    }
    bench::Dedup cache;
    assert(cache.classify(original, bench::OFF) == bench::APPLIED);
    cache.remember(original);
    assert(cache.classify(original, bench::ACTIVE) == bench::DUPLICATE);
    assert(cache.classify(original, bench::OFF) == bench::EXPIRED);
    auto changed = original; changed.state = bench::FULL;
    assert(cache.classify(changed, bench::ACTIVE) == bench::CONFLICT);
    ++changed.token;
    assert(cache.classify(changed, bench::ACTIVE) == bench::APPLIED);
    for (int s = bench::OFF; s <= bench::FAULT; ++s)
        assert(!strcmp(bench::name(s), indicator::name(static_cast<indicator::Mode>(s))));
    std::cout << "PASS: codec, CRC golden vector, every single-bit corruption, validation, duplicate/expired/conflict handling, state mapping\n";
}
