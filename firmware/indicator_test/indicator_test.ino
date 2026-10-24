// PROTO - Node1 V1 bench harness. Commands only; no radio calls.
#include <Arduino.h>
#include <cstring>
#include "indicator_pattern.h"

static const uint32_t pins[] = {PB3, PB4, PB5}; // Blue, red, green.
static const char *colors[] = {"BLUE", "RED", "GREEN"};
static indicator::Mode mode = indicator::OFF;
static int selected = indicator::DARK;
static uint32_t started = 0;
static bool ready = false;

static void driveColor(int color)
{
    if (color == selected) return;
    for (uint32_t pin : pins) digitalWrite(pin, LOW);
    selected = indicator::DARK;
    if (color >= 0 && color < 3) {
        delay(2); // Break before make on every color change.
        digitalWrite(pins[color], HIGH);
        selected = color;
    }
}

static void stopDisplay()
{
    mode = indicator::OFF;
    for (uint32_t pin : pins) digitalWrite(pin, LOW);
    selected = indicator::DARK;
}

static void updateDisplay()
{
    uint32_t elapsed = (uint32_t)(millis() - started);
    if (mode != indicator::OFF && elapsed >= indicator::LIMIT_MS) {
        stopDisplay();
        Serial.println("NODE1_LED=AUTO_OFF");
    } else {
        driveColor(indicator::colorFor(mode, elapsed));
    }
}

static void report()
{
    updateDisplay();
    Serial.printf("NODE1_LED=%s;PB3=%d;PB4=%d;PB5=%d;LIMIT_MS=%lu\r\n",
                  selected < 0 ? "OFF" : colors[selected],
                  digitalRead(PB3), digitalRead(PB4), digitalRead(PB5),
                  (unsigned long)indicator::LIMIT_MS);
    Serial.printf("NODE1_STATE=%s;ELAPSED_MS=%lu;PHASE_MS=%lu;FW=0.2.0\r\n",
                  indicator::name(mode),
                  mode == indicator::OFF ? 0UL : (unsigned long)(millis() - started),
                  (unsigned long)indicator::PHASE_MS);
}

static void startDisplay(indicator::Mode requested)
{
    stopDisplay();
    started = millis();
    mode = requested;
    updateDisplay();
}

int ledCommand(SERIAL_PORT port, char *cmd, stParam *param)
{
    (void)port; (void)cmd;
    if (!ready) return AT_ERROR;
    if (param->argc == 1 && !strcmp(param->argv[0], "?")) {
        report();
        return AT_OK;
    }
    stopDisplay();
    if (param->argc != 1) return AT_PARAM_ERROR;
    if (!strcmp(param->argv[0], "OFF")) {
        report();
        return AT_OK;
    }
    const indicator::Mode tests[] = {indicator::TEST_BLUE, indicator::TEST_RED,
                                     indicator::TEST_GREEN};
    for (int index = 0; index < 3; ++index) {
        if (!strcmp(param->argv[0], colors[index])) {
            startDisplay(tests[index]);
            report();
            return AT_OK;
        }
    }
    return AT_PARAM_ERROR;
}

int stateCommand(SERIAL_PORT port, char *cmd, stParam *param)
{
    (void)port; (void)cmd;
    if (!ready) return AT_ERROR;
    if (param->argc == 1 && !strcmp(param->argv[0], "?")) {
        report();
        return AT_OK;
    }
    stopDisplay();
    if (param->argc != 1) return AT_PARAM_ERROR;
    for (int index = indicator::OFF; index <= indicator::FAULT; ++index) {
        indicator::Mode requested = static_cast<indicator::Mode>(index);
        if (!strcmp(param->argv[0], indicator::name(requested))) {
            startDisplay(requested);
            report();
            return AT_OK;
        }
    }
    return AT_PARAM_ERROR;
}

void setup()
{
    for (uint32_t pin : pins) {
        digitalWrite(pin, LOW);
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
    stopDisplay();
    Serial.begin(115200, RAK_AT_MODE);
    delay(5000); // Preserve an AT/bootloader recovery window.
    api.system.lpm.set(0); // Bench loop enforces patterns and timeout.
    api.system.firmwareVersion.set("MS-Node1-LED-0.2.0");
    bool ledReady = api.system.atMode.add((char *)"LED",
        (char *)"Bench: OFF|BLUE|RED|GREEN; auto OFF after 10 seconds",
        (char *)"LED", ledCommand, RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
    bool stateReady = api.system.atMode.add((char *)"STATE",
        (char *)"OFF|ACTIVE|NEXT|STANDBY|HOLD_OPEN|FULL|FAULT; bench timeout 10s",
        (char *)"STATE", stateCommand, RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
    ready = ledReady && stateReady;
    Serial.println(ready ? "PROTO-Node1-V1 seven-state test ready; outputs OFF"
                         : "Command registration failed; outputs OFF");
    report();
}

void loop()
{
    updateDisplay();
    delay(5);
}
