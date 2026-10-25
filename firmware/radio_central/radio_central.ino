#include <Arduino.h>
#include <BenchProtocol.h>
#include <BenchAddress.h>
#include <BenchRadio.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>

static bool ready = false, pending = false, transmitting = false;
static bench::Confirmations confirmations;
static bool dropArmed = false, dropThisTransaction = false;
static uint8_t attempts = 0;
static uint32_t deadline = 0;
static const char *result = "IDLE";
static bench::Frame request = {bench::SET, 1, bench::OFF, bench::APPLIED, 0};

static void report() {
    const auto &confirmation = confirmations.get(request.node);
    benchRadio::printConfig("CENTRAL", ready, "0.4.2");
    Serial.printf("MS_LINK=%s;TOKEN=%lu;COMMANDED=%s;CONFIRMED=%s;ATTEMPTS=%u;AGE_MS=%lu\r\n",
        result, (unsigned long)request.token, bench::name(request.state),
        confirmation.known ? bench::name(confirmation.state) : "UNKNOWN", attempts,
        confirmation.known ? (unsigned long)(millis() - confirmation.at) : 0UL);
    Serial.printf("MS_TARGET_NODE=%u;MAX_NODES=50\r\n", request.node);
    Serial.printf("MS_DROP_ARMED=%d;TRANSACTION_DROP_PENDING=%d\r\n", dropArmed, dropThisTransaction);
}
int dropCommand(SERIAL_PORT, char *, stParam *p) {
    if (p->argc == 1 && !strcmp(p->argv[0], "?")) { report(); return AT_OK; }
    if (pending) return AT_ERROR;
    if (p->argc != 1 || (strcmp(p->argv[0], "0") && strcmp(p->argv[0], "1")))
        return AT_PARAM_ERROR;
    dropArmed = !strcmp(p->argv[0], "1");
    report(); return AT_OK;
}
int penCommand(SERIAL_PORT, char *, stParam *p) {
    if (p->argc == 1 && !strcmp(p->argv[0], "?")) { report(); return AT_OK; }
    if (!ready || pending) return AT_ERROR;
    const char *arguments[3] = {nullptr, nullptr, nullptr};
    if (p->argc != 2 && p->argc != 3) return AT_PARAM_ERROR;
    for (int i = 0; i < p->argc; ++i) arguments[i] = p->argv[i];
    bench::Frame parsed;
    if (!bench::parseSet(p->argc, arguments, parsed)) return AT_PARAM_ERROR;
    request = parsed;
    pending = true; transmitting = false; attempts = 0;
    dropThisTransaction = dropArmed;
    dropArmed = false;
    deadline = millis(); result = "PENDING";
    report(); return AT_OK;
}
void setup() {
    Serial.begin(115200, RAK_AT_MODE);
    delay(5000);
    api.system.lpm.set(0);
    api.system.firmwareVersion.set("MS-Central-RF-0.4.2");
    bool commandReady = api.system.atMode.add((char *)"PEN",
        (char *)"Bench token:node(1-50):OFF|ACTIVE|LAST|HOLD_OPEN|FULL|FAULT",
        (char *)"PEN", penCommand, RAK_ATCMD_PERM_READ | RAK_ATCMD_PERM_WRITE);
    bool testReady = api.system.atMode.add((char *)"DROP",
        (char *)"Bench only: 1 ignores next transaction's first successful ACK; 0 disables",
        (char *)"DROP", dropCommand, RAK_ATCMD_PERM_READ | RAK_ATCMD_PERM_WRITE);
    ready = commandReady && testReady && benchRadio::begin();
    report();
}
void loop() {
    if (!ready) { delay(5); return; }
    bench::Frame ack;
    if (benchRadio::take(ack) && pending && bench::matchesAck(ack, request)) {
        const auto &confirmation = confirmations.get(request.node);
        if (dropThisTransaction && ack.state == request.state && ack.result <= bench::DUPLICATE) {
            dropThisTransaction = false;
            Serial.printf("MS_TEST_ACK_IGNORED=1;TOKEN=%lu;RESULT=%u;CONFIRMED=%s\r\n",
                (unsigned long)ack.token, ack.result,
                confirmation.known ? bench::name(confirmation.state) : "UNKNOWN");
            // Leave transaction, deadline and last confirmation untouched.
            return;
        }
        dropThisTransaction = false;
        confirmations.remember(ack, millis());
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
            pending = false; transmitting = false; dropThisTransaction = false;
            result = "FAILED"; report();
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
