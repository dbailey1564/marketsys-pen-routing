# First application upload: prototype-01

Date: 2026-10-20. Hardware: photographed RAK3272-SiP Ver B / RAK3172-SiP.
This is a serial bring-up milestone, not completed MarketSys firmware.

## Prepared image

- Arduino CLI 1.5.1 and rak_rui:stm32@4.2.4.
- Board: rak_rui:stm32:WisDuoRAK3272SiPBoard.
- Source: firmware/build_check/build_check.ino.
- Application linked at 0x08006000; pinned linker allocation is 196 KiB.
- Binary size: 172,512 bytes (includes layout padding; distinct from the
  compiler's section-based program-storage summary).
- SHA256: `e4772686fa466ac79132418576681da9f6d323fb36f8836393f4409d5dde3bca`.
- Compiler summary: 157,888 B program storage, 33,840 B static RAM,
  14,800 B reported RAM remaining.

## Upload

User authorized proceeding after the verified original backup and UART check.
The official RUI3 `uploader_ymodem` 1.0.1 transferred the application over the
RAKDAP1 UART, entered boot mode, received the final ACK, and reported
`Upgrade Complete`. Its dependency pyserial 3.5 is installed in a dedicated
virtual environment under `Development/Tools/marketsys-uart-venv`.
Raw log: build/first-upload.log (excluded from Git).

## Runtime observations

- Startup: `MarketSys RAK3272-SiP build check started`.
- Repeated output: `MarketSys RAK3272-SiP build check alive`.
- `AT` returns `OK`.
- `AT+VER=?` returns `AT+VER=RUI_4.2.4_RAK3272-SiP` and `OK`.
- RUI startup reports `Current Work Mode: LoRaWAN.` No mode-change or radio
  transmission commands were sent in this bring-up procedure.
- The custom application version is set to `MarketSys-SiP-check-0.1.0` in the
  sketch. AT+VER reports the SDK version rather than that application string.

Raw log: build/firmware-query-after-upload.log (excluded from Git).

## Scope

The application opens UART2 at 115200 in AT mode, waits five seconds at startup,
and prints a heartbeat every two seconds. It contains no indicator control or
LoRa send/receive setup. No other prototype board has been flashed by this work.
Power/recharge design and RF parameters remain open.

## Post-upload SWD readback

The complete main flash was read into a new private snapshot on Lin:
`Development/Backups/marketsys/prototype-01/20261020T195515.754373Z-post-first-upload/`.

- All 172,512 application-image bytes match at 0x08006000.
- All 12,608 bootloader-code bytes match the official 4.2.4 reference; the
  separate four-byte activation marker at 0x08004000 also matches.
- The full reserved bootloader region is not identical: 12 bytes changed at
  **0x08004004–0x0800400F**, immediately after the activation marker. This is
  consistent with upload bookkeeping; the individual field meanings were not
  independently established. No other byte in 0x08000000–0x08005FFF changed.
- Above the application's linker allocation, only 25 bytes changed, at
  **0x0803F1FB–0x0803F213**. These are exactly the new application-version
  string `MarketSys-SiP-check-0.1.0`, set by the sketch.
- Post-upload main-flash SHA256:
  `3f74352ff11e134cc110dd32bbdb7bcbe38fdff05b38d0dce77a190755e4e309`.

The first verifier deliberately flagged the reserved-region difference and
stopped with an error after saving the snapshot. Offline analysis established
the limited ranges above and confirmed unchanged bootloader code. The verifier
was updated to allow only this reviewed 12-byte update-record range; all other
bootloader-region differences still fail it. That update was syntax checked;
the on-device read was not unnecessarily repeated.

Logs: build/first-upload-readback.log, build/first-upload-changes.log,
build/bootloader-comparison-after-upload.log. The original two-copy backup is
untouched, and no restore was attempted. Bootloader code preservation does not
mean every reserved/settings byte remains unchanged.

Original flash preservation is documented in hardware-identification.md.
