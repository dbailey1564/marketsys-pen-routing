#include <cassert>
#include <iostream>
#include "../firmware/radio_central/radio_central.ino"

static int command(const char *name, std::initializer_list<const char *> args) {
    stParam p{};
    for (auto arg : args) p.argv[p.argc++] = const_cast<char *>(arg);
    return api.system.atMode.handlers.at(name)(0, nullptr, &p);
}

int main() {
    setup();
    task = NO_TASK;
    startupDecision = startup::RESUME;
    maintenanceEnabled = false;
    assert(command("POWER", {"1","1","SLEEP"}) == AT_OK);
    loop();
    bench::Frame reply{bench::POWER_ACK, 1, bench::OFF, bench::APPLIED, 1};
    reply.flags = bench::SLEEP_ENABLED;
    benchRadio::inject(reply);
    loop();
    assert(asleep[1] && !pending);
    assert(command("DEST", {"1"}) == AT_ERROR);
    assert(command("POWER", {"2","1","WAKE"}) == AT_OK);
    const uint32_t start = fakeNow;
    loop();
    for (uint32_t elapsed = 2000; elapsed < 240000; elapsed += 2000) {
        fakeNow = start + elapsed;
        loop();
    }
    assert(pending && attempts == 120);
    reply.token = 2;
    reply.flags = 0;
    benchRadio::inject(reply);
    loop();
    assert(!pending && !asleep[1]);
    assert(command("POWER", {"3","1","WAKE"}) == AT_OK);
    fakeNow = wakeUntil;
    loop();
    assert(!pending && !strcmp(result,"FAILED"));
    std::cout << "PASS: sleep interlock, two-second retries, wake after a lost window, and deadline\n";
}
