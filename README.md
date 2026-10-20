# MarketSys pen routing

Firmware and host-interface work for a central controller and LoRa pen indicators.
The current milestone is a verified development environment and a minimal
serial bring-up application on prototype-01, not deployable MarketSys firmware.
The original main flash is backed up on Lin and Windows.

## Development workspace

Open `/home/dbailey1564/Development/marketsys-pen-routing` through VS Code's
existing `SSH: Lin` connection. Build commands run on Lin (Linux x86_64), rather
than in Windows PowerShell.

```text
Development/
├── marketsys-pen-routing/           Application repository
└── Tools/
    ├── nanoDAP/                    Existing debugger reference; unchanged
    ├── RUI3-Best-Practice/          Official RAK application references
    ├── arduino-cli/1.5.1/          Pinned command-line builder
    └── marketsys-rui3/             Isolated board packages and downloads
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

**There is no automatic upload task.** Prototype-01 was uploaded deliberately
using the official UART uploader. A successful compile does not establish wiring,
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

See [docs/first-upload.md](docs/first-upload.md) for the first programming result.
See [docs/project-context.md](docs/project-context.md) for carried-forward
hardware, state, and protocol decisions. Application implementation and physical
testing remain subsequent work.

## Initial hardware access and backups

[docs/hardware-identification.md](docs/hardware-identification.md) records the
first successful SWD connection. `tools/identify-target.cfg` identifies the
target, and `tools/inspect-flash.cfg` reads flash geometry/protection settings.
Both suppress the stock examine handler's debug/watchdog register writes.

`python3 scripts/backup_flash.py` reads the currently connected prototype's
256 KiB main flash twice, requires matching contents, and writes a SHA256
manifest. It checks RDP level 0 and never unlocks, erases, programs, resets, or
halts the target. It is specific to this validated STM32WL flash layout; do not
use it for a different device without reviewing the configuration.

Backups live outside the repository under the sibling directory
`Backups/marketsys/prototype-01/<UTC timestamp>/`, with owner-only access.
Treat the connected board as prototype-01 and label it before swapping boards.
Backups can contain device credentials/settings: keep them out of Git and do
not share their raw contents. The manifest describes scope and limitations;
matching reads establish data consistency, not a tested restore procedure.
