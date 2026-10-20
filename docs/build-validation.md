# Build environment validation

Validated on **2026-10-20** on Lin (Debian 13, Linux x86_64), over SSH from the
Windows development machine. These are compile results, not hardware test results.

## Versions and source

- Arduino CLI: **1.5.1**.
- CLI Linux x86_64 release archive SHA256:
  `28a8e119c498a25607821c36cb2dc49e8463941b261a0d99091baa7bc692dd2b`.
- RUI3: **rak_rui:stm32@4.2.4**.
- Compiler dependency: **arm-none-eabi-gcc 9-2019q4**.
- MarketSys target, corrected after inspecting the supplied board photo:
  **rak_rui:stm32:WisDuoRAK3272SiPBoard**.
- Unchanged vendor-reference target:
  **rak_rui:stm32:WisDuoRAK3172EvaluationBoard**. The references require
  LED_GREEN/LED_BLUE, absent from the SiP variant definition. Both targets use
  the pinned core's default menu options, including LoRaWAN and P2P support.
- Official examples: **RAKWireless/RUI3-Best-Practice** at
  `3460377f44fe17885397335027d2fa8eb66dc11b`.

## Successful checks

| Compile task | Program storage | Static RAM | Reported RAM remaining |
|---|---:|---:|---:|
| Minimal build check (SiP target) | 157,216 B (78%) | 33,656 B (69%) | 14,984 B |
| Official P2P receive (evaluation target) | 163,432 B (81%) | 34,312 B (70%) | 14,328 B |
| Official P2P transmit (evaluation target) | 163,400 B (81%) | 34,304 B (70%) | 14,336 B |

The initial minimal check on the evaluation target used 157,880 B flash and
33,800 B static RAM; it preceded the photo-based hardware correction.
An attempted unmodified receive-example build on the SiP target failed because
LED_GREEN/LED_BLUE were undefined. The reference tasks therefore retain their
vendor evaluation target; this does not establish P2P application compatibility
on the SiP hardware. Their GPIO assumptions need adaptation before deployment.

Reported application limits: **200,704 B program storage**, **48,640 B RAM**.
These are the package's application allocations, not a statement of the MCU's
total physical resources. Remaining RAM is the build report's static estimate;
it does not establish runtime stack/heap headroom or peak usage.

Also verified:

- `python3 scripts/dev.py verify` accepts the installed pinned versions.
- `python3 scripts/dev.py setup` succeeds when rerun against the configured
  environment. It does not overwrite mismatched reference work.
- The reference checkout remains clean at the recorded commit.
- The existing nanoDAP checkout remains clean on its original master branch.
- `git diff --check` passes for tracked changes.

Build logs and generated binary images are in the ignored `build/` directory.
Rerun a compile task to regenerate its artifacts; the CLI prints its result in
the terminal. Logging redirection is optional.

## Limits and next milestone

At the initial compile milestone no board was flashed or serial port opened.
Subsequent programming of prototype-01 and UART/readback verification are
documented in first-upload.md. There has been no radio,
indicator, battery, recharge, or system-level test. The central/node application
logic and host framing remain unimplemented. Module variant and programming
connections need confirmation before hardware programming.

Primary references: [RAK RUI3 package installation](https://github.com/RAKWireless/RAKwireless-Arduino-BSP-Index#readme),
[RAK official examples](https://github.com/RAKWireless/RUI3-Best-Practice),
[Arduino CLI](https://arduino.github.io/arduino-cli/1.5/installation/).
