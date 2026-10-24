// PROTO - Node1 V1 bench harness. No automatic color sequence or radio calls.
#include <Arduino.h>
#include <cstring>

static const uint32_t pins[] = {PB3, PB4, PB5}; // Blue, red, green.
static const char *names[] = {"BLUE", "RED", "GREEN"};
static int selected = -1;
static uint32_t started = 0;
static bool ready = false;
static const uint32_t ON_LIMIT_MS = 10000;

static void allOff()
{
    for (uint32_t pin : pins) digitalWrite(pin, LOW);
    selected = -1;
}

static void report()
{
    Serial.printf("NODE1_LED=%s;PB3=%d;PB4=%d;PB5=%d;LIMIT_MS=%lu\r\n",
                  selected < 0 ? "OFF" : names[selected],
                  digitalRead(PB3), digitalRead(PB4), digitalRead(PB5),
                  (unsigned long)ON_LIMIT_MS);
}

int ledCommand(SERIAL_PORT port, char *cmd, stParam *param)
{
    (void)port;
    (void)cmd;
    if (!ready) return AT_ERROR;
    if (param->argc == 1 && !strcmp(param->argv[0], "?")) {
        report();
        return AT_OK;
    }
    // A malformed write also leaves all outputs OFF.
    allOff();
    if (param->argc != 1) return AT_PARAM_ERROR;
    if (!strcmp(param->argv[0], "OFF")) {
        report();
        return AT_OK;
    }
    for (int index = 0; index < 3; ++index) {
        if (!strcmp(param->argv[0], names[index])) {
            delay(2); // Break before make when changing colors.
            started = millis();
            selected = index;
            digitalWrite(pins[index], HIGH);
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
    allOff();
    Serial.begin(115200, RAK_AT_MODE);
    delay(5000); // Preserve an AT/bootloader recovery window.
    api.system.lpm.set(0); // Bench harness needs loop() to enforce timeout.
    api.system.firmwareVersion.set("MS-Node1-LED-0.1.0");
    ready = api.system.atMode.add((char *)"LED",
        (char *)"Bench only: OFF|BLUE|RED|GREEN; automatic OFF after 10 seconds",
        (char *)"LED", ledCommand, RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
    Serial.println(ready ? "PROTO-Node1-V1 LED test ready; outputs OFF"
                         : "LED command registration failed; outputs OFF");
    report();
}

void loop()
{
    if (selected >= 0 && (uint32_t)(millis() - started) >= ON_LIMIT_MS) {
        allOff();
        Serial.println("NODE1_LED=AUTO_OFF");
    }
    delay(10);
}
