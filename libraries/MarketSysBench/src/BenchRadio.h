#pragma once
#include <Arduino.h>
#include <cstring>
#include "BenchProtocol.h"

namespace benchRadio {
static volatile bool received = false, sent = false;
static uint8_t inbox[bench::SIZE];
static uint8_t outbox[bench::SIZE];
static int16_t lastRssi = 0;
static int8_t lastSnr = 0;
static void rx(rui_lora_p2p_recv_t data) {
    if (data.Status != LORA_P2P_RXDONE || data.BufferSize != bench::SIZE || received) return;
    memcpy(inbox, data.Buffer, bench::SIZE);
    lastRssi = data.Rssi; lastSnr = data.Snr;
    received = true;
}
static void tx() { sent = true; }
inline bool begin() {
    // Read installed RUI3 APIs and pinned vendor P2P examples before changing.
    bool ok = api.lora.nwm.set();
    ok = api.lora.pfreq.set(915000000) && ok;
    ok = api.lora.psf.set(7) && ok;
    ok = api.lora.pbw.set(0) && ok; // 125 kHz in installed 4.2.4 API.
    ok = api.lora.pcr.set(1) && ok; // 4/5.
    ok = api.lora.ppl.set(8) && ok;
    ok = api.lora.ptp.set(10) && ok; // 10 dBm bench setting.
    ok = api.lora.encry.set(false) && ok;
    ok = api.lora.iqInver.set(false) && ok;
    ok = api.lora.syncword.set(0x1424) && ok;
    ok = api.lora.registerPRecvCallback(rx) && ok;
    ok = api.lora.registerPSendCallback(tx) && ok;
    return ok && api.lora.precv(65533); // Continuous RX, TX allowed.
}
inline bool take(bench::Frame &frame) {
    if (!received) return false;
    uint8_t copy[bench::SIZE];
    noInterrupts();
    memcpy(copy, inbox, bench::SIZE);
    received = false;
    interrupts();
    return bench::decode(copy, bench::SIZE, frame);
}
inline bool send(const bench::Frame &frame) {
    bench::encode(frame, outbox);
    sent = false;
    return api.lora.psend(bench::SIZE, outbox, false);
}
inline void printConfig(const char *role, bool ready, const char *version = "0.3.0") {
    Serial.printf("MS_RADIO=%s;READY=%d;FREQ=%lu;SF=%u;BW=%lu;CR=%u;POWER=%u;FW=%s\r\n",
        role, ready, (unsigned long)api.lora.pfreq.get(), api.lora.psf.get(),
        (unsigned long)api.lora.pbw.get(), api.lora.pcr.get(), api.lora.ptp.get(), version);
}
}
