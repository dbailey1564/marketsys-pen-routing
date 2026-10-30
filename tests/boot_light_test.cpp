#include "../libraries/MarketSysBench/src/BootLightTest.h"
#include <cassert>
#include <iostream>
using namespace indicator;
int main() {
    BootLightTest test;
    assert(!test.running() && !test.sample(100).running);
    const Mode states[] = {ACTIVE, LAST, HOLD_OPEN, FULL, FAULT};
    const Color steady[] = {GREEN, BLUE, GREEN, RED, RED};
    // Deliberately start across millis() rollover.
    const uint32_t began = UINT32_MAX - 1000;
    test.start(began);
    for (uint32_t step = 0; step < 5; ++step) {
        for (uint32_t elapsed : {0u, 249u, 250u, 499u, 500u, 1749u, 1750u, 1999u}) {
            auto s = test.sample(uint32_t(began + step*2000 + elapsed));
            assert(s.running && s.mode == states[step] && s.elapsed == elapsed);
            bool flashing = s.mode == HOLD_OPEN || s.mode == FAULT;
            Color expected = flashing && (elapsed/250)%2 ? DARK : steady[step];
            assert(colorFor(s.mode, s.elapsed, BOOT_PHASE_MS, BOOT_STEP_MS) == expected);
        }
    }
    auto done = test.sample(uint32_t(began + 10000));
    assert(!done.running && done.mode == OFF);
    assert(!test.running() && !test.sample(uint32_t(began + 12000)).running);
    // Missed loop iterations cannot extend a phase or restart the sequence.
    test.start(100); auto late = test.sample(8600);
    assert(late.running && late.mode == FAULT && late.elapsed == 500);
    assert(!test.sample(10100).running);
    // A real command cancels startup for good, even before first update.
    test.start(100); test.cancel();
    assert(!test.sample(100).running && !test.sample(1000).running);
    test.start(100); assert(test.sample(4600).mode == HOLD_OPEN);
    test.cancel(); assert(!test.sample(5000).running && !test.sample(15000).running);
    // Accelerated boot limits must not change normal command behavior.
    assert(colorFor(HOLD_OPEN, 250) == GREEN);
    assert(colorFor(HOLD_OPEN, 500) == DARK);
    assert(colorFor(FULL, 2000) == RED);
    assert(colorFor(FULL, 9999) == RED && colorFor(FULL, 10000) == DARK);
    assert(colorFor(FULL, 2000, BOOT_PHASE_MS, BOOT_STEP_MS) == DARK);
    std::cout << "PASS: five boot states, 250ms phases, 2s steps, final OFF, rollover, late update, cancellation, normal timings\n";
}
