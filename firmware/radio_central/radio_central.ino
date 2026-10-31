#include <Arduino.h>
#include <BenchProtocol.h>
#include <BenchAddress.h>
#include <BenchRadio.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>

static bool ready = false, pending = false, transmitting = false;
static bench::Confirmations confirmations;
static bench::Confirmations commanded;
static bool scanning = false;
static uint8_t scanOk = 0, scanFailed = 0;
static void cached(uint8_t node) {
    const auto &c = confirmations.get(node);
    const auto &d = commanded.get(node);
    Serial.printf("MS_CACHE_NODE=%u;COMMANDED=%s;CONFIRMED=%s;KNOWN=%d;AGE_MS=%lu;SOURCE=CACHE\r\n",
        node, d.known ? bench::name(d.state) : "UNKNOWN",
        c.known ? bench::name(c.state) : "UNKNOWN", c.known,
        c.known ? (unsigned long)(millis()-c.at) : 0UL);
}
static bool dropArmed = false, dropThisTransaction = false;
static uint8_t attempts = 0;
static uint32_t deadline = 0;
static const char *result = "IDLE";
static bench::Frame request = {bench::SET, 1, bench::OFF, bench::APPLIED, 0};

static void report() {
    const auto &confirmation = confirmations.get(request.node);
    benchRadio::printConfig("CENTRAL", ready, "0.5.0");
    Serial.printf("MS_LINK=%s;TOKEN=%lu;COMMANDED=%s;CONFIRMED=%s;ATTEMPTS=%u;AGE_MS=%lu\r\n",
        result, (unsigned long)request.token, request.type == bench::QUERY ? "UNCHANGED" : bench::name(request.state),
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
    auto &desired = commanded.nodes[request.node];
    desired.known = true; desired.state = request.state; desired.at = millis();
    pending = true; transmitting = false; attempts = 0;
    dropThisTransaction = dropArmed;
    dropArmed = false;
    deadline = millis(); result = "PENDING";
    report(); return AT_OK;
}
static void startPoll(const bench::Frame &frame) {
    request = frame;
    pending = true; transmitting = false; attempts = 0;
    dropThisTransaction = false;
    deadline = millis(); result = "PENDING";
}
int pollCommand(SERIAL_PORT, char *, stParam *p) {
    const char *args[2] = {nullptr, nullptr};
    if (p->argc != 2) return AT_PARAM_ERROR;
    args[0] = p->argv[0]; args[1] = p->argv[1];
    bench::Frame frame;
    if (!bench::parsePoll(2, args, frame)) return AT_PARAM_ERROR;
    if (!ready || pending) return AT_ERROR;
    startPoll(frame); report(); return AT_OK;
}
int pollLastCommand(SERIAL_PORT, char *, stParam *p) {
    uint32_t node;
    if (p->argc != 1 || !bench::decimal(p->argv[0], bench::MAX_NODES, node) ||
        !bench::validNode(node)) return AT_PARAM_ERROR;
    cached(uint8_t(node)); return AT_OK;
}
int pollAllLastCommand(SERIAL_PORT, char *, stParam *p) {
    if (p->argc != 1 || strcmp(p->argv[0], "?")) return AT_PARAM_ERROR;
    for (uint8_t node = 1; node <= bench::MAX_NODES; ++node) cached(node);
    Serial.println("MS_CACHE_ALL=DONE;COUNT=50");
    return AT_OK;
}
int pollAllCommand(SERIAL_PORT, char *, stParam *p) {
    uint32_t token;
    if (p->argc != 1 || !bench::decimal(p->argv[0], UINT32_MAX, token) || !token)
        return AT_PARAM_ERROR;
    if (!ready || pending) return AT_ERROR;
    scanning = true; scanOk = scanFailed = 0;
    startPoll({bench::QUERY, 1, bench::OFF, bench::APPLIED, token});
    Serial.println("MS_POLL_ALL=START;COUNT=50");
    return AT_OK;
}
static void finishPoll(bool success) {
    Serial.printf("MS_POLL_NODE=%u;TOKEN=%lu;RESULT=%s;STATE=%s;ATTEMPTS=%u;SOURCE=LIVE\r\n",
        request.node, (unsigned long)request.token, success ? "OK" : "TIMEOUT",
        success ? bench::name(confirmations.get(request.node).state) : "UNKNOWN", attempts);
    if (!scanning) return;
    if (success) ++scanOk; else ++scanFailed;
    if (request.node == bench::MAX_NODES) {
        scanning = false;
        Serial.printf("MS_POLL_ALL=DONE;OK=%u;FAILED=%u;COUNT=50\r\n", scanOk, scanFailed);
    } else {
        uint32_t token = request.token + 1;
        if (!token) token = 1;
        startPoll({bench::QUERY, uint8_t(request.node + 1), bench::OFF, bench::APPLIED, token});
    }
}
void setup() {
    Serial.begin(115200, RAK_AT_MODE);
    delay(5000);
    api.system.lpm.set(0);
    api.system.firmwareVersion.set("MS-Central-RF-0.5.0");
    bool commandReady = api.system.atMode.add((char *)"PEN",
        (char *)"Bench token:node(1-50):OFF|ACTIVE|LAST|HOLD_OPEN|FULL|FAULT",
        (char *)"PEN", penCommand, RAK_ATCMD_PERM_READ | RAK_ATCMD_PERM_WRITE);
    bool testReady = api.system.atMode.add((char *)"DROP",
        (char *)"Bench only: 1 ignores next transaction's first successful ACK; 0 disables",
        (char *)"DROP", dropCommand, RAK_ATCMD_PERM_READ | RAK_ATCMD_PERM_WRITE);
    bool pollReady = api.system.atMode.add((char *)"POLL", (char *)"Live read-only token:node",
        (char *)"POLL", pollCommand, RAK_ATCMD_PERM_WRITE);
    pollReady = api.system.atMode.add((char *)"POLL_LAST", (char *)"Cached state by node",
        (char *)"POLL_LAST", pollLastCommand, RAK_ATCMD_PERM_WRITE) && pollReady;
    pollReady = api.system.atMode.add((char *)"POLL_ALL", (char *)"Live read-only all 50; initial token",
        (char *)"POLL_ALL", pollAllCommand, RAK_ATCMD_PERM_WRITE) && pollReady;
    pollReady = api.system.atMode.add((char *)"POLL_ALL_LAST", (char *)"Cached states, ?",
        (char *)"POLL_ALL_LAST", pollAllLastCommand, RAK_ATCMD_PERM_READ) && pollReady;
    ready = commandReady && testReady && pollReady && benchRadio::begin();
    report();
}
void loop() {
    if (!ready) { delay(5); return; }
    bench::Frame ack;
    if (benchRadio::take(ack) && pending && (bench::matchesAck(ack, request) || bench::matchesStatus(ack, request))) {
        const auto &confirmation = confirmations.get(request.node);
        if (request.type == bench::SET && dropThisTransaction && ack.state == request.state && ack.result <= bench::DUPLICATE) {
            dropThisTransaction = false;
            Serial.printf("MS_TEST_ACK_IGNORED=1;TOKEN=%lu;RESULT=%u;CONFIRMED=%s\r\n",
                (unsigned long)ack.token, ack.result,
                confirmation.known ? bench::name(confirmation.state) : "UNKNOWN");
            // Leave transaction, deadline and last confirmation untouched.
            return;
        }
        dropThisTransaction = false;
        confirmations.remember(ack, millis());
        result = (request.type == bench::QUERY || (ack.state == request.state && ack.result <= bench::DUPLICATE)) ? "ACK" : "REJECTED";
        pending = false; transmitting = false;
        Serial.printf("MS_ACK_RESULT=%u;RSSI=%d;SNR=%d\r\n", ack.result,
            benchRadio::lastRssi, benchRadio::lastSnr);
        report();
        if (request.type == bench::QUERY) { finishPoll(true); return; }
    }
    if (transmitting && benchRadio::sent) {
        transmitting = false;
        deadline = millis() + 1000;
    }
    if (pending && int32_t(millis() - deadline) >= 0) {
        if (attempts >= 3) {
            pending = false; transmitting = false; dropThisTransaction = false;
            result = "FAILED"; report();
            if (request.type == bench::QUERY) { finishPoll(false); return; }
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
