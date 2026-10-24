#include "../firmware/indicator_test/indicator_pattern.h"
#include <cassert>
#include <iostream>
using namespace indicator;

int main()
{
    struct Expected { Mode mode; Color first; Color second; };
    const Expected states[] = {
        {OFF, DARK, DARK}, {ACTIVE, GREEN, DARK}, {NEXT, BLUE, DARK},
        {STANDBY, BLUE, BLUE}, {HOLD_OPEN, GREEN, BLUE}, {FULL, RED, RED},
        {FAULT, RED, DARK}, {TEST_BLUE, BLUE, BLUE}, {TEST_RED, RED, RED},
        {TEST_GREEN, GREEN, GREEN}
    };
    for (const Expected &state : states) {
        assert(colorFor(state.mode, 0) == state.first);
        assert(colorFor(state.mode, 499) == state.first);
        assert(colorFor(state.mode, 500) == state.second);
        assert(colorFor(state.mode, 999) == state.second);
        assert(colorFor(state.mode, 1000) == state.first);
        assert(colorFor(state.mode, 9999) == state.second);
        assert(colorFor(state.mode, 10000) == DARK);
        assert(colorFor(state.mode, 10001) == DARK);
        assert(colorFor(state.mode, UINT32_MAX) == DARK);
    }
    // Runtime uses unsigned elapsed time so patterns cross millis() rollover.
    uint32_t start = UINT32_MAX - 100;
    assert(colorFor(ACTIVE, uint32_t(uint32_t(398) - start)) == GREEN);
    assert(colorFor(ACTIVE, uint32_t(uint32_t(399) - start)) == DARK);
    assert(colorFor(HOLD_OPEN, uint32_t(uint32_t(899) - start)) == GREEN);
    std::cout << "PASS: seven-state phase boundaries, timeout, diagnostics, rollover\n";
}
