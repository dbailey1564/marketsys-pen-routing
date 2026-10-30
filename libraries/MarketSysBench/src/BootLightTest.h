#pragma once
#include "BenchPattern.h"

namespace indicator {
static const uint32_t BOOT_DELAY_MS = 10000;
static const uint32_t BOOT_PHASE_MS = 250;
static const uint32_t BOOT_STEP_MS = 2000;
static const uint8_t BOOT_STEPS = 5;
static const uint32_t BOOT_TOTAL_MS = BOOT_STEP_MS * BOOT_STEPS;
struct BootSample { Mode mode; uint32_t elapsed; bool running; bool waiting; };
// One nonblocking run after successful node setup; commands can cancel it.
class BootLightTest {
    bool running_ = false;
    uint32_t began_ = 0;
    bool waiting_ = false;
public:
    void start(uint32_t now) { began_ = now; running_ = true; waiting_ = true; }
    bool running() const { return running_; }
    bool waiting() const { return running_ && waiting_; }
    void cancel() { running_ = false; waiting_ = false; }
    BootSample sample(uint32_t now) {
        if (!running_) return {OFF, 0, false, false};
        uint32_t elapsed = uint32_t(now - began_);
        if (elapsed < BOOT_DELAY_MS) return {OFF, 0, true, true};
        waiting_ = false;
        elapsed -= BOOT_DELAY_MS;
        if (elapsed >= BOOT_TOTAL_MS) { running_ = false; return {OFF, 0, false, false}; }
        static const Mode states[BOOT_STEPS] = {ACTIVE, LAST, HOLD_OPEN, FULL, FAULT};
        return {states[elapsed / BOOT_STEP_MS], elapsed % BOOT_STEP_MS, true, false};
    }
};
}
