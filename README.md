# MarketSys pen routing

Firmware and host-interface work for a central controller and LoRa pen indicators.
Current deployed firmware: **0.4.2** on central and Nodes2/3; **0.4.3** on
Node1, with its boot light sequence checked through runtime GPIO reports.
All four uploads have byte-for-byte readback verification. Central supports addresses **1-50**; the
three physical nodes have distinct addresses 1, 2 and 3. See the
[deployment record](docs/indicator-revision-042.md) for backups, hashes and
verification. Node1 acknowledged OFF over air on 0.4.2; addressed testing of the updated
0.4.3 node and the other nodes remains pending. The binary host
interface and automatic head-entry pen flow remain unimplemented; indicators
still have a 10-second bench cutoff. This is not production firmware.

Node revision **0.4.3** adds a one-time boot light test: five current
indications, two seconds each, 250 ms flash phases, then OFF. Normal command
timings remain unchanged. Node1 is uploaded and GPIO-tested; Nodes2/3 have prepared images awaiting
upload. Central remains compatible on 0.4.2. See [boot-test revision](docs/indicator-revision-043.md).

## Development workspace

Open `/home/dbailey1564/Development/marketsys-pen-routing` through VS Code's
existing `SSH: Lin` connection. Build commands run on Lin (Linux x86_64), rather
than in Windows PowerShell.

```text
Development/
â”œâ”€â”€ marketsys-pen-routing/           Application repository
â””â”€â”€ Tools/
    â”œâ”€â”€ nanoDAP/                    Existing debugger reference; unchanged
    â”œâ”€â”€ RUI3-Best-Practice/          Official RAK application references
    â”œâ”€â”€ arduino-cli/1.5.1/          Pinned command-line builder
    â””â”€â”€ marketsys-rui3/             Isolated board packages and downloads
```

The repository's origin is
`https://github.com/dbailey1564/marketsys-pen-routing.git`. RAK's framework and
examples are dependencies/references; this repository is not a fork of nanoDAP
or RUI3, and has no upstream remote.

## Build

In VS Code, **Ctrl+Shift+B** runs **MarketSys: build environment check**.
Use **Terminal > Run Task** for the RAK receive/transmit reference builds or
the pinned-tool installation task. These tasks use Python 3 and Arduino CLI;
no Arduino IDE or Arduino extension is required to run them.

Equivalent commands from this repository on Lin:

```bash
python3 scripts/dev.py setup          # Initial install / restore pinned tools
python3 scripts/dev.py verify
python3 scripts/dev.py build-check
python3 scripts/dev.py radio-central
python3 scripts/dev.py radio-node --node-id 1   # Choose an explicit address 1..50
python3 scripts/dev.py reference-rx
python3 scripts/dev.py reference-tx
```

`toolchain.json` records the CLI version and archive checksum, RUI3 core version,
board target, and exact reference commit. The setup installs under the sibling
`Tools` folder without changing global Arduino settings. Existing reference
checkouts must match the pinned commit and have no local changes. A new
reference checkout is placed at that commit with detached HEAD.

Generated artifacts are under `build/`, excluded from Git. The build-check
sketch uses UART2 at 115200 in AT mode, leaves a five-second startup window,
sets a custom application-version string, and prints a repeating two-second
heartbeat; it contains no radio or indicator control commands. Its AT+VER query
reports the RUI3 SDK version, not that custom application-version string.
The RAK reference tasks compile the unchanged
official examples, including their example radio parameters. They do not
validate MarketSys radio settings and should not be treated as deployment images.
They use the separate evaluation-board `reference_fqbn` from `toolchain.json`:
the examples depend on LED_GREEN/LED_BLUE, which the SiP board does not define.
The MarketSys build-check task uses the photo-confirmed SiP target instead.

**There is no automatic upload task.** The four bench boards were uploaded
deliberately using the official UART uploader and individually readback-verified.
Node images are separated under `build/radio-node-N/`; never reuse an image
with the wrong compiled address. A successful compile does not establish wiring,
power, oscillator/module variant, serial-port access, bootloader compatibility,
radio operation, or LED performance on a physical board.

## Framework selection

- Arduino CLI **1.5.1**.
- RUI3 STM32 package **4.2.4** (`rak_rui:stm32`). This is a pinned starting
  baseline, not a claim that it is the newest available release.
- Compile target: **WisDuo RAK3272-SiP Board**, matching the board photo supplied
  on 2026-10-20: module **RAK3172-SiP**, breakout **RAK3272-SiP Ver B**.
  Earlier development checks used the non-SiP evaluation target before the
  hardware photo was available; those were not hardware-matched builds.
- Official reference commit: `3460377f44fe17885397335027d2fa8eb66dc11b`.

RAK distinguishes the RUI3 package index from its WisBlock Arduino index. This
setup uses the RUI3 index documented in the [official BSP installation
instructions](https://github.com/RAKWireless/RAKwireless-Arduino-BSP-Index#readme).
The framework is [RAK-STM32-RUI](https://github.com/RAKWireless/RAK-STM32-RUI),
and the examples are [RUI3-Best-Practice](https://github.com/RAKWireless/RUI3-Best-Practice).
The latter documents a RUI3 4.2.1 requirement and earlier P2P API changes.
Arduino CLI's [installation documentation](https://arduino.github.io/arduino-cli/1.5/installation/)
describes the standalone builder.

## Application planning

For the October 22 presentation, see [USB prototype power and wiring](docs/thursday-usb-prototype.md).

See [docs/first-upload.md](docs/first-upload.md) for the first programming result.
See [docs/sleep-timer-test.md](docs/sleep-timer-test.md) for the separate finite
timer/sleep harness, its failed radio-disabled build, and verified recovery.
See [docs/project-context.md](docs/project-context.md) for carried-forward
hardware, state, and protocol decisions. Current addressed firmware and
per-board hardware verification are in
[docs/indicator-revision-042.md](docs/indicator-revision-042.md).
Use `scripts/radio_link.py last --node 2` through the UART virtual environment
with central connected to command Node2. This is a diagnostic AT interface,
separate from the proposed
[application protocol](docs/interface-protocol-specification.md).

## Initial hardware access and backups

[docs/hardware-identification.md](docs/hardware-identification.md) records the
first successful SWD connection. `tools/identify-target.cfg` identifies the
target, and `tools/inspect-flash.cfg` reads flash geometry/protection settings.
Both suppress the stock examine handler's debug/watchdog register writes.

`python3 scripts/backup_flash.py --board proto-node2-v1 --expected-uid UID`
(select the actual board label and its recorded 24-hex-digit UID) reads that board's
256 KiB main flash twice, requires matching contents, and writes a SHA256
manifest. It checks RDP level 0 and never unlocks, erases, programs, resets, or
halts the target. It is specific to this validated STM32WL flash layout; do not
use it for a different device without reviewing the configuration.

Backups live outside the repository under the sibling directory
`Backups/marketsys/<board label>/<UTC timestamp>/`, with owner-only access.
Use `prototype-01` for central and `proto-nodeN-v1` for node N; verify physical
identity and retain separate backups before programming each board.
Backups can contain device credentials/settings: keep them out of Git and do
not share their raw contents. The manifest describes scope and limitations;
matching reads establish data consistency, not a tested restore procedure.
