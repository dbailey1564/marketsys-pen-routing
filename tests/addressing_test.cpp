#include "../libraries/MarketSysBench/src/BenchAddress.h"
#include <cassert>
#include <iostream>
int main() {
    bench::Frame f;
    for (unsigned id = 1; id <= 50; ++id) {
        auto address = std::to_string(id);
        const char *args[] = {"4294967295", address.c_str(), "LAST"};
        assert(bench::parseSet(3, args, f));
        assert(f.node == id && f.token == UINT32_MAX && f.state == bench::LAST);
        uint8_t wire[bench::SIZE]; bench::encode(f, wire);
        bench::Frame decoded; assert(bench::decode(wire, sizeof(wire), decoded));
        for (unsigned receiver = 1; receiver <= 50; ++receiver)
            assert(bench::forNode(decoded, receiver) == (receiver == id));
        bench::Frame ack = {bench::ACK, uint8_t(id), bench::LAST, bench::APPLIED, f.token};
        assert(bench::matchesAck(ack, f));
        ++ack.token; assert(!bench::matchesAck(ack, f)); --ack.token;
        ack.node = id == 50 ? 1 : id+1; assert(!bench::matchesAck(ack, f));
    }
    for (const char *bad : {"0", "51", "255", "-1", "+1", "", "2x", "999999999999999999999"}) {
        const char *args[] = {"1", bad, "OFF"}; assert(!bench::parseSet(3, args, f));
    }
    for (const char *bad : {"0", "4294967296", "-1", "+1", "", "1x"}) {
        const char *args[] = {bad, "50", "OFF"}; assert(!bench::parseSet(3, args, f));
    }
    for (const char *bad : {"NEXT", "STANDBY", "INVALID", "off", ""}) {
        const char *args[] = {"1", "2", bad}; assert(!bench::parseSet(3, args, f));
    }
    const char *legacy[] = {"123", "ACTIVE"};
    assert(bench::parseSet(2, legacy, f) && f.node == 1);
    assert(!bench::parseSet(1, legacy, f)); assert(!bench::parseSet(4, legacy, f));
    bench::Confirmations states;
    for (uint8_t id = 1; id <= 50; ++id) assert(!states.get(id).known);
    bench::Frame a = {bench::ACK, 1, bench::ACTIVE, bench::APPLIED, 1};
    bench::Frame b = {bench::ACK, 50, bench::LAST, bench::APPLIED, 2};
    states.remember(a, 100); states.remember(b, 200);
    assert(states.get(1).state == bench::ACTIVE && states.get(1).at == 100);
    assert(states.get(50).state == bench::LAST && states.get(50).at == 200);
    assert(!states.get(2).known);
    b.node = 0; states.remember(b, 300); assert(!states.get(0).known);
    b.node = 51; states.remember(b, 400); assert(!states.get(51).known);
    std::cout << "PASS: all 50 addresses, cross-node isolation, command limits, ACK matching, per-node confirmation\n";
}
