#pragma once
#include <stdint.h>

namespace indicator {
enum Mode { OFF = 0, ACTIVE = 1, LAST = 2, HOLD_OPEN = 4, FULL = 5, FAULT = 6,
            TEST_BLUE = 7, TEST_RED = 8, TEST_GREEN = 9 };
enum Color { DARK = -1, BLUE = 0, RED = 1, GREEN = 2 };
static const uint32_t PHASE_MS = 500;
static const uint32_t LIMIT_MS = 10000;

inline Color colorFor(Mode mode, uint32_t elapsed,
                      uint32_t phaseMs = PHASE_MS, uint32_t limitMs = LIMIT_MS)
{
    if (elapsed >= limitMs) return DARK;
    const bool first = (elapsed / phaseMs) % 2 == 0;
    switch (mode) {
    case ACTIVE: return GREEN;
    case LAST: case TEST_BLUE: return BLUE;
    case HOLD_OPEN: return first ? GREEN : DARK;
    case FULL: case TEST_RED: return RED;
    case FAULT: return first ? RED : DARK;
    case TEST_GREEN: return GREEN;
    default: return DARK;
    }
}
inline const char *name(Mode mode)
{
    switch (mode) {
    case ACTIVE: return "ACTIVE";
    case LAST: return "LAST";
    case HOLD_OPEN: return "HOLD_OPEN";
    case FULL: return "FULL";
    case FAULT: return "FAULT";
    case TEST_BLUE: return "TEST_BLUE";
    case TEST_RED: return "TEST_RED";
    case TEST_GREEN: return "TEST_GREEN";
    case OFF: return "OFF";
    default: return "INVALID";
    }
}
}
