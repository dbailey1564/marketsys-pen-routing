#include <Arduino.h>
#include <BenchProtocol.h>
#include <BenchAddress.h>
#ifndef MARKETSYS_NODE_ID
#error "Build with an explicit MARKETSYS_NODE_ID (1..50)"
#endif
static_assert(MARKETSYS_NODE_ID >= 1 && MARKETSYS_NODE_ID <= bench::MAX_NODES, "Invalid node ID");
#define MS_STRING_INNER(x) #x
#define MS_STRING(x) MS_STRING_INNER(x)
static const uint8_t nodeId = MARKETSYS_NODE_ID;
#include <BenchPattern.h>
#include <BootLightTest.h>
#include <BenchRadio.h>
#include <cstring>

static const uint32_t pins[] = {PB3, PB4, PB5};
static uint8_t state = bench::OFF;
static int color = indicator::DARK;
static uint32_t started = 0;
static bool ready = false, ackPending = false;
static uint32_t ackAt = 0;
static uint8_t ackTries = 0;
static bool rebootPending=false;
static uint32_t rebootAt=0;
static bench::Frame acknowledgement;
static bench::Dedup dedup;
static indicator::BootLightTest bootTest;

static void dark() {
    for (uint32_t pin : pins) digitalWrite(pin, LOW);
    color = indicator::DARK;
}
static void update() {
    const uint32_t now = millis();
    uint32_t elapsed = uint32_t(now - started);
    uint32_t phaseMs = indicator::PHASE_MS, limitMs = UINT32_MAX;
    if (bootTest.running()) {
        auto sample = bootTest.sample(now);
        if (!sample.running) {
            state = bench::OFF; started = now; elapsed = 0;
            Serial.println("MS_BOOT_TEST=DONE;STATE=OFF");
        } else {
            if (state != sample.mode) {
                state = sample.mode;
                Serial.printf("MS_BOOT_TEST=STEP;STATE=%s\r\n", bench::name(state));
            }
            elapsed = sample.elapsed;
            phaseMs = indicator::BOOT_PHASE_MS; limitMs = indicator::BOOT_STEP_MS;
        }
    } else if (elapsed >= 1000000000UL) {
        // Keep arithmetic bounded indefinitely; one billion is a multiple of
        // the normal 1000 ms flash cycle, preserving phase across millis wrap.
        started += 1000000000UL; elapsed -= 1000000000UL;
    }
    int next = indicator::colorFor(static_cast<indicator::Mode>(state), elapsed, phaseMs, limitMs);
    if (next != color) {
        dark();
        if (next >= 0) { delay(2); digitalWrite(pins[next], HIGH); color = next; }
    }
}
static void apply(uint8_t requested) {
    if (bootTest.running()) {
        bootTest.cancel();
        Serial.println("MS_BOOT_TEST=CANCELLED;REASON=COMMAND");
    }
    dark();
    state = requested;
    started = millis();
    update();
}
static void report() {
    update();
    benchRadio::printConfig("NODE" MS_STRING(MARKETSYS_NODE_ID), ready, "0.6.0");
    Serial.printf("MS_NODE_ID=%u;MAX_NODES=50\r\n", nodeId);
    Serial.printf("MS_NODE_STATE=%s;PB3=%d;PB4=%d;PB5=%d;TOKEN=%lu;LIMIT_MS=%lu;PHASE_MS=%lu;BOOT_TEST=%d\r\n",
        bench::name(state), digitalRead(PB3), digitalRead(PB4), digitalRead(PB5),
        (unsigned long)(dedup.valid ? dedup.token : 0),
        (unsigned long)(bootTest.running() ? indicator::BOOT_STEP_MS : 0),
        (unsigned long)(bootTest.running() ? indicator::BOOT_PHASE_MS : indicator::PHASE_MS),
        bootTest.running());
}
int nodeCommand(SERIAL_PORT, char *, stParam *p) {
    if (p->argc == 1 && !strcmp(p->argv[0], "?")) { report(); return AT_OK; }
    if (p->argc == 1 && !strcmp(p->argv[0], "OFF")) {
        apply(bench::OFF); report(); return AT_OK;
    }
    return AT_PARAM_ERROR;
}
void setup() {
    for (uint32_t pin : pins) { digitalWrite(pin, LOW); pinMode(pin, OUTPUT); }
    dark();
    Serial.begin(115200, RAK_AT_MODE);
    delay(5000);
    api.system.lpm.set(0);
    api.system.firmwareVersion.set("MS-Node" MS_STRING(MARKETSYS_NODE_ID) "-RF-0.6.0");
    bool commandReady = api.system.atMode.add((char *)"NODE", (char *)"Node radio status or OFF",
        (char *)"NODE", nodeCommand, RAK_ATCMD_PERM_READ | RAK_ATCMD_PERM_WRITE);
    ready = commandReady && benchRadio::begin();
    report();
    if (ready) {
        Serial.println("MS_BOOT_TEST=START;PHASE_MS=250;STEP_MS=2000;TOTAL_MS=10000");
        bootTest.start(millis());
        update();
    }
}
void loop() {
    update();
    if (!ready) { dark(); delay(5); return; }
    bench::Frame request;
    if (benchRadio::take(request) && bench::forNode(request, nodeId)) {
        if (request.type == bench::REBOOT) {
            acknowledgement = {bench::REBOOT_ACK, nodeId, bench::OFF, bench::APPLIED, request.token};
            ackAt = millis()+50; ackTries=0; ackPending=true;
            if(!rebootPending){rebootPending=true;rebootAt=millis()+1000;}
            Serial.println("MS_REBOOT=QUEUED");
        } else if (request.type == bench::SET && bootTest.running()) {
            // Preserve the complete startup test; the controller polls until ready.
            Serial.println("MS_NODE=BOOT_BUSY");
        } else {
        // QUERY is observational: never apply a state, cancel boot, refresh its
        // timeout, or replace the SET deduplication token.
        const bool query = request.type == bench::QUERY;
        bench::Result result = query ? bench::APPLIED : dedup.classify(request, state);
        if (!query && result == bench::APPLIED) { apply(request.state); dedup.remember(request); }
        acknowledgement = {uint8_t(query ? bench::STATUS : bench::ACK), nodeId,
            state, uint8_t(query && bootTest.running() ? bench::BOOTING : result), request.token};
        ackAt = millis() + 50; // Let central switch from TX to RX.
        ackTries = 0;
        ackPending = true;
        Serial.printf("MS_NODE_RX=%s;CURRENT=%s;TOKEN=%lu;RESULT=%u\r\n",
            request.type == bench::QUERY ? "QUERY" : bench::name(request.state), bench::name(state), (unsigned long)request.token, result);
        }
    }
    if (ackPending && int32_t(millis() - ackAt) >= 0) {
        // Refresh current state if the bench timeout elapsed during ACK delay.
        if (acknowledgement.type == bench::STATUS) {
            acknowledgement.state=state;
            acknowledgement.result=bootTest.running()?bench::BOOTING:bench::APPLIED;
        } else if (acknowledgement.type == bench::ACK && state != acknowledgement.state) {
            acknowledgement.state = state;
            if (acknowledgement.type == bench::ACK) acknowledgement.result = bench::EXPIRED;
        }
        if (benchRadio::send(acknowledgement) || ++ackTries >= 3) ackPending = false;
        else ackAt = millis() + 100;
    }
    if(rebootPending && int32_t(millis()-rebootAt)>=0){dark();api.system.reboot();}
    delay(5);
}
