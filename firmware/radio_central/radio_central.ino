#include <Arduino.h>
#include <BenchProtocol.h>
#include <BenchRadio.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>

static bool ready = false, pending = false, transmitting = false;
static bool confirmedKnown = false;
static uint8_t confirmed = bench::OFF, attempts = 0;
static uint32_t deadline = 0, confirmedAt = 0;
static const char *result = "IDLE";
static bench::Frame request = {bench::SET, 1, bench::OFF, bench::APPLIED, 0};

static void report() {
    benchRadio::printConfig("CENTRAL", ready);
    Serial.printf("MS_LINK=%s;TOKEN=%lu;COMMANDED=%s;CONFIRMED=%s;ATTEMPTS=%u;AGE_MS=%lu\r\n",
        result, (unsigned long)request.token, bench::name(request.state),
        confirmedKnown ? bench::name(confirmed) : "UNKNOWN", attempts,
        confirmedKnown ? (unsigned long)(millis() - confirmedAt) : 0UL);
}
int penCommand(SERIAL_PORT, char *, stParam *p) {
    if (p->argc == 1 && !strcmp(p->argv[0], "?")) { report(); return AT_OK; }
    if (!ready || pending) return AT_ERROR;
    if (p->argc != 2 || !p->argv[0][0] || strlen(p->argv[0]) > 10) return AT_PARAM_ERROR;
    for (const char *s = p->argv[0]; *s; ++s) if (*s < '0' || *s > '9') return AT_PARAM_ERROR;
    errno = 0;
    unsigned long token = strtoul(p->argv[0], nullptr, 10);
    if (errno == ERANGE || token == 0) return AT_PARAM_ERROR;
    for (uint8_t s = bench::OFF; s <= bench::FAULT; ++s) {
        if (!strcmp(p->argv[1], bench::name(s))) {
            request = {bench::SET, 1, s, bench::APPLIED, uint32_t(token)};
            pending = true; transmitting = false; attempts = 0;
            deadline = millis(); result = "PENDING";
            report(); return AT_OK;
        }
    }
    return AT_PARAM_ERROR;
}
void setup() {
    Serial.begin(115200, RAK_AT_MODE);
    delay(5000);
    api.system.lpm.set(0);
    api.system.firmwareVersion.set("MS-Central-RF-0.3.0");
    bool commandReady = api.system.atMode.add((char *)"PEN",
        (char *)"Bench node 1: decimal token:OFF|ACTIVE|NEXT|STANDBY|HOLD_OPEN|FULL|FAULT",
        (char *)"PEN", penCommand, RAK_ATCMD_PERM_READ | RAK_ATCMD_PERM_WRITE);
    ready = commandReady && benchRadio::begin();
    report();
}
void loop() {
    if (!ready) { delay(5); return; }
    bench::Frame ack;
    if (benchRadio::take(ack) && pending && ack.type == bench::ACK &&
        ack.node == request.node && ack.token == request.token) {
        confirmedKnown = true; confirmed = ack.state; confirmedAt = millis();
        result = (ack.state == request.state && ack.result <= bench::DUPLICATE) ? "ACK" : "REJECTED";
        pending = false; transmitting = false;
        Serial.printf("MS_ACK_RESULT=%u;RSSI=%d;SNR=%d\r\n", ack.result,
            benchRadio::lastRssi, benchRadio::lastSnr);
        report();
    }
    if (transmitting && benchRadio::sent) {
        transmitting = false;
        deadline = millis() + 1000;
    }
    if (pending && int32_t(millis() - deadline) >= 0) {
        if (attempts >= 3) {
            pending = false; transmitting = false; result = "FAILED"; report();
        } else {
            ++attempts;
            transmitting = benchRadio::send(request);
            // Bound a missing TX callback as well as missing acknowledgements.
            deadline = millis() + (transmitting ? 2000 : 1000);
            Serial.printf("MS_TX_ATTEMPT=%u;TOKEN=%lu;QUEUED=%d\r\n",
                attempts, (unsigned long)request.token, transmitting);
        }
    }
    delay(5);
}
