# Timer/sleep harness

Date: 2026-10-20. Target: prototype-01, RAK3272-SiP Ver B / RAK3172-SiP.

## Purpose and build

This harness tests timer callback scheduling, timekeeping through requested
sleep, and an early UART interruption. It does not exercise radio reception or
the LED driver and cannot establish electrical sleep depth or battery current.

- Source: firmware/sleep_test/sleep_test.ino.
- CLI 1.5.1, RUI3 core 4.2.4.
- FQBN: rak_rui:stm32:WisDuoRAK3272SiPBoard (default SDK support).
- The harness makes no application radio calls. SDK radio support is compiled in.
- Compiler report: 158,888 B program storage; 33,864 B static RAM;
  14,776 B reported RAM remaining.
- Corrected binary: 173,936 B, SHA256
  `af6ef4b70dcd044d027183caa1b73afacf7e23283fed9239322470b4e133392c`.
- Original bring-up binary remains separate under build/build-check, with SHA256
  `e4772686fa466ac79132418576681da9f6d323fb36f8836393f4409d5dde3bca`.

Compile using `python3 scripts/dev.py sleep-test`, or the corresponding VS Code
task. Compilation never uploads. The official RUI3 UART uploader was invoked
separately for the deliberate hardware test.

## Finite test sequence

1. Five-second startup/recovery window with UART2 at 115200, AT mode.
2. Record existing low-power enable and level settings. Request level 2 and
   low-power enable 1. These API setters persist settings; this test deliberately
   changes them and restores the originals on normal completion or handled
   failure. Unexpected power loss/reset during the test can prevent restoration.
3. Two FAST cycles: periodic timer at 10,000 ms, simulated window 1,000 ms.
4. Two FIELD cycles: periodic timer at 120,000 ms, simulated window 5,000 ms.
5. Stop timers, restore saved low-power settings, and stay available for UART
   recovery. The harness remains installed until deliberately replaced.

The periodic callback increments a counter; the window callback sets a flag.
Serial printing and timer phase changes occur in loop, not in callbacks.
Early returns from `sleep.all()` do not restart the periodic timer. The simulated
window also uses sleep; it is an event-timing stand-in, **not five seconds of
electrically awake radio reception**.

Pinned SDK sources inspected: RAKSystem.h/cpp (timer create/start/stop),
sleep.h/cpp (sleep and persistent low-power controls), and wiring_time.cpp
(`millis()` uses the RTC timestamp). Manufacturer patterns are in
[RAK's low-power examples](https://github.com/RAKWireless/RUI3-Best-Practice/tree/main/RUI3-LowPower-Example).

## Host-side observation

scripts/observe_sleep_test.py uses the dedicated pyserial 3.5 environment.
It timestamps serial lines on Lin and checks both RTC-based device timestamps
and host monotonic intervals. Twenty seconds after the first FIELD window closes,
it sends a UART wake edge followed by the read-only AT query and requires OK.
The next periodic deadline must remain on schedule.

Bench tolerances: +/-1 second for the initial/periodic timer and host interval;
simulated windows must be at least their requested duration and at most 500 ms
longer. These are deliberately coarse bring-up criteria, not the final five-minute
RF activation acceptance test or a calibrated oscillator specification.

Logs/artifacts, excluded from Git:
build/sleep-test-build.log, build/sleep-test-upload.log,
build/sleep-test-observation.jsonl, build/sleep-test-observer.log,
build/sleep-test-summary.json.

## Failed radio-disabled attempt and recovery

The first build used supportlora=4, binary size 61,392 B, SHA256
`1cce650e122519ecb43cc8c9fc38b68a8f3db6ceadc4522e07b2587c6edf4537`.
The official uploader completed, but no startup serial output appeared.
SWD inspection found PC 0x08009b8a in HardFault_Handler, CFSR 0x00020000
(INVSTATE), HFSR 0x40000000. The stacked PC resolved to uhal_timer_init
at uhal_timer.c:103, before sketch setup. This implicates SDK startup in that
build configuration; the precise underlying cause is not established.

scripts/recover_sleep_test.py restored the previously verified application
allocation 0x08006000..0x08036fff and its matching 2 KiB update-record page
0x08004000..0x080047ff. Full before/after snapshots and comparisons verified
those regions and unchanged bootloader code and remaining settings. The restored
heartbeat and AT / AT+VER responses were observed. This tests recovery to the
custom bring-up firmware, not restoration to the original factory version.

Recovery evidence on Lin: Development/Backups/marketsys/prototype-01/
20261021T032832.288564Z-sleep-recovery/. Original factory and first-upload
snapshots remain preserved. The corrected build uses the previously working
default SDK configuration.

## Corrected harness results

The observer completed with passed=true after 270.538 host seconds.

| Check | Observed result |
| --- | --- |
| FAST opening interval | 10,000 device ms; 10.026 host s |
| FAST windows | 1,007 device ms each |
| First FIELD opening | 120,007 device ms after phase start |
| FIELD opening interval | 120,000 device ms; 120.156 host s |
| FIELD windows | 5,007 device ms each |
| UART interruption | AT sent at host 170.684 s; OK observed at 170.885 s |
| Completion | Two cycles per phase, both restore setters returned success |

Original settings were lpm=0, lpmlvl=1. Device and host timestamps meet the
coarse bench tolerances; neither clock is calibrated here. UART activity did
not shift the device's periodic schedule. The installed harness remains idle
after completion until reset; a reset reruns the finite test. Radio support is
compiled into the default SDK, but the harness makes no application RF calls.
These results do not establish electrical sleep depth, current, RF wake latency,
or five-minute central-to-node activation. Those remain separate measurements.

Raw evidence remains on Lin in build/sleep-test-retry-observer.log and
build/sleep-test-summary.json, with uploader evidence in
build/sleep-test-retry-upload.log. scripts/verify_sleep_test.py performs a
separate nonhalting application readback against the recorded binary hash.
Readback passed: all 173,936 bytes matched. Post-test AT and AT+VER queries
returned OK, with SDK version RUI_4.2.4_RAK3272-SiP.
