#include <cassert>
#include <iostream>
#define MARKETSYS_NODE_ID 1
#include "../firmware/radio_node/radio_node.ino"

static bench::Frame exchange(bench::Frame frame) {
    benchRadio::inject(frame);
    loop();
    fakeNow += 60;
    loop();
    return benchRadio::outgoing.back();
}

int main() {
    setup();
    fakeNow = 16000;
    loop();
    auto reply = exchange({bench::SLEEP, 1, bench::OFF, bench::APPLIED, 1});
    assert(reply.result == bench::APPLIED && (reply.flags & bench::SLEEP_ENABLED));
    assert(state == bench::OFF && sleepEnabled && !api.system.sleep.duration);
    loop();
    fakeNow = keepListeningUntil;
    loop();
    assert(benchRadio::receiveDuration == 0);
    assert(api.system.sleep.duration == windowAt - (fakeNow - 5));

    const uint32_t opening = windowAt;
    fakeNow = opening;
    loop();
    assert(windowAt == opening + 120000 && windowEnd == opening + 5000);
    assert(benchRadio::receiveDuration == 5000);
    fakeNow = opening + 1000;
    benchRadio::inject({bench::WAKE, 2, bench::OFF, bench::APPLIED, 2});
    loop();
    assert(sleepEnabled && windowEnd == opening + 5000 && benchRadio::receiveDuration == 4000);

    reply = exchange({bench::WAKE, 1, bench::OFF, bench::APPLIED, 3});
    assert(reply.result == bench::APPLIED && !(reply.flags & bench::SLEEP_ENABLED));
    assert(!sleepEnabled && state == bench::OFF && benchRadio::receiveDuration == 65533);
    reply = exchange({bench::SET, 1, bench::ACTIVE, bench::APPLIED, 4});
    assert(reply.state == bench::ACTIVE);
    reply = exchange({bench::SLEEP, 1, bench::OFF, bench::APPLIED, 5});
    assert(reply.result == bench::CONFLICT && !sleepEnabled && state == bench::ACTIVE);
    std::cout << "PASS: addressed sleep, bounded RX, unrelated traffic, awake ACK, and active-node rejection\n";
}
