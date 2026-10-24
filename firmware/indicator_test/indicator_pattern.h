#pragma once
#include <stdint.h>

namespace indicator {
enum Mode { OFF, ACTIVE, NEXT, STANDBY, HOLD_OPEN, FULL, FAULT,
            TEST_BLUE, TEST_RED, TEST_GREEN };
enum Color { DARK = -1, BLUE = 0, RED = 1, GREEN = 2 };
static const uint32_t PHASE_MS = 500;
static const uint32_t LIMIT_MS = 10000;

inline Color colorFor(Mode mode, uint32_t elapsed)
{
    if (elapsed >= LIMIT_MS) return DARK;
    const bool first = (elapsed / PHASE_MS) % 2 == 0;
    switch (mode) {
    case ACTIVE: return first ? GREEN : DARK;
    case NEXT: return first ? BLUE : DARK;
    case STANDBY: case TEST_BLUE: return BLUE;
    case HOLD_OPEN: return first ? GREEN : BLUE;
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
    case NEXT: return "NEXT";
    case STANDBY: return "STANDBY";
    case HOLD_OPEN: return "HOLD_OPEN";
    case FULL: return "FULL";
    case FAULT: return "FAULT";
    case TEST_BLUE: return "TEST_BLUE";
    case TEST_RED: return "TEST_RED";
    case TEST_GREEN: return "TEST_GREEN";
    default: return "OFF";
    }
}
}
