// Timer/sleep bench harness only. Default SDK; no application radio calls.
#include <Arduino.h>

volatile uint32_t period_ticks = 0;
volatile bool close_due = false;
uint32_t handled_ticks = 0;
uint32_t phase_cycle = 0;
uint32_t phase_epoch = 0;
uint32_t window_epoch = 0;
bool field_phase = false;
bool window_open = false;
bool finished = false;
uint8_t original_lpm = 0;
uint8_t original_level = 1;

void period_callback(void *) { ++period_ticks; }
void close_callback(void *) { close_due = true; }

void finish_test(bool success, const char *reason)
{
    api.system.timer.stop(RAK_TIMER_0);
    api.system.timer.stop(RAK_TIMER_1);
    bool level_ok = api.system.lpmlvl.set(original_level);
    bool lpm_ok = api.system.lpm.set(original_lpm);
    finished = true;
    window_open = false;
    Serial.printf("MS_SLEEP %s reason=%s restore_lpm=%u restore_level=%u\r\n",
                  success ? "COMPLETE" : "FAIL", reason, lpm_ok, level_ok);
    Serial.flush();
}

bool start_phase(bool field)
{
    field_phase = field;
    phase_cycle = 0;
    handled_ticks = period_ticks;
    phase_epoch = millis();
    uint32_t period = field ? 120000UL : 10000UL;
    Serial.printf("MS_SLEEP PHASE name=%s period_ms=%lu window_ms=%lu epoch=%lu\r\n",
                  field ? "FIELD" : "FAST", period, field ? 5000UL : 1000UL, phase_epoch);
    Serial.flush();
    return api.system.timer.start(RAK_TIMER_0, period, NULL);
}

void setup()
{
    Serial.begin(115200, RAK_AT_MODE);
    delay(5000); // Preserve an upload/recovery opportunity after reset.
    original_lpm = api.system.lpm.get();
    original_level = api.system.lpmlvl.get();
    Serial.printf("MS_SLEEP START original_lpm=%u original_level=%u radio=unused_by_harness\r\n",
                  original_lpm, original_level);
    if (original_level != 1 && original_level != 2) {
        finished = true;
        Serial.println("MS_SLEEP FAIL reason=unexpected_original_sleep_level");
        return;
    }
    if (!api.system.timer.create(RAK_TIMER_0, period_callback, RAK_TIMER_PERIODIC)
        || !api.system.timer.create(RAK_TIMER_1, close_callback, RAK_TIMER_ONESHOT)
        || !api.system.lpmlvl.set(2) || !api.system.lpm.set(1) || !start_phase(false)) {
        finish_test(false, "setup");
    }
}

void loop()
{
    if (finished) {
        delay(20); // Stay available for AT commands and the next deliberate upload.
        return;
    }
    uint32_t observed_ticks = period_ticks;
    if (observed_ticks != handled_ticks) {
        if (observed_ticks - handled_ticks != 1 || window_open) {
            finish_test(false, "missed_period_or_overlapping_window");
            return;
        }
        handled_ticks = observed_ticks;
        ++phase_cycle;
        window_open = true;
        close_due = false;
        window_epoch = millis();
        Serial.printf("MS_SLEEP WINDOW_OPEN phase=%s cycle=%lu tick_ms=%lu phase_elapsed_ms=%lu\r\n",
                      field_phase ? "FIELD" : "FAST", phase_cycle, window_epoch,
                      uint32_t(window_epoch - phase_epoch));
        Serial.flush();
        if (!api.system.timer.start(RAK_TIMER_1, field_phase ? 5000UL : 1000UL, NULL)) {
            finish_test(false, "window_timer");
            return;
        }
    }
    if (close_due) {
        close_due = false;
        uint32_t now = millis();
        Serial.printf("MS_SLEEP WINDOW_CLOSE phase=%s cycle=%lu tick_ms=%lu duration_ms=%lu\r\n",
                      field_phase ? "FIELD" : "FAST", phase_cycle, now,
                      uint32_t(now - window_epoch));
        Serial.flush();
        window_open = false;
        if (phase_cycle == 2) {
            if (!api.system.timer.stop(RAK_TIMER_0)) {
                finish_test(false, "stop_phase_timer");
                return;
            }
            if (field_phase) {
                finish_test(true, "two_fast_and_two_field_cycles");
                return;
            }
            if (!start_phase(true)) {
                finish_test(false, "start_field_timer");
                return;
            }
        }
    }
    // This is only a timed stand-in for a future radio window: no RX/TX or GPIO.
    // Sleep can return early on UART/SDK interrupts. Only timer flags advance
    // phases, so serial activity never restarts the periodic timer.
    api.system.sleep.all();
}
