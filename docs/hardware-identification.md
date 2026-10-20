# First SWD target identification

Observed on 2026-10-20 with one photographed RAK3272-SiP Ver B / RAK3172-SiP
board connected to the RAKDAP1 on Lin. The user reported making the four-wire
connection; no independent continuity or voltage measurement was performed.

## Connection specified to the user

| Programmer | Target |
|---|---|
| 3V3 | 3V3 |
| GND | GND |
| TMS/IO | PA13 / SWDIO |
| TCK/CK | PA14 / SWCLK |

nRST was left out of the specified connection. The programmer's 5V pin was not
part of the connection. The user was instructed to attach an antenna before
powering the target and to use the programmer as the only power source for the
bare board.

## Actual OpenOCD results

- OpenOCD 0.12.0; CMSIS-DAP firmware 0254.
- Probe USB identity: 0d28:0204.
- SWD clock: 100 kHz.
- Debug-port ID (DPIDR): `0x6ba02477`.
- Detected processor: Cortex-M4 r0p1.
- Reported debug resources: 6 breakpoints, 4 watchpoints.
- Device-ID register read: `0xE0042000 = 0x10016497`.
- Target table state: `unknown`; this procedure does not establish application
  execution state or firmware health.
- Identification procedure completed with exit status 0.

`tools/identify-target.cfg` contains no reset, halt, erase, or programming
commands. It replaces the stock STM32WL examine-end handler with an empty
handler to avoid its debug-mode/watchdog register writes. Debug access itself
still exchanges SWD control transactions; this is not electrically passive
observation. No flash contents were changed by this procedure.

Raw log: `build/target-identification.log` (excluded from Git).

## Original application version queried over UART

After the user added the UART2 wires (programmer TX to PA3, RX to PA2), the
board answered at 115200 baud, 8N1, no flow control:

```text
AT
OK
AT+VER=?
AT+VER=RUI_4.0.6_RAK3272-SiP
OK
```

This establishes the reported application version, not an independently read
bootloader version. The current development package is RUI3 4.2.4.
`scripts/query_firmware.py` sends only
these two queries and restores the host's prior serial settings.
Raw log: `build/firmware-query.log` (excluded from Git).

The original backup was compared to the 4.2.4 package's prebuilt bootloader:
all **12,608 reference bytes matched**, with zero differences. The separate
four-byte bootloader-active reference at 0x08004000 also matched. This supports
preserving the current bootloader for the first application upload. It is not
a completed upgrade or an application execution test. Comparison log:
`build/bootloader-comparison.log`; helper: `scripts/compare_bootloader.py`.

## Remaining work

Next: upload and verify the minimal SiP build while preserving the matched
bootloader and existing settings regions.
The SWD identification does not validate radio operation, LED circuitry, power
margins, or recharge behavior.

## Verified main-flash backup

Two complete 262,144-byte reads matched byte for byte on 2026-10-20.
OpenOCD reported 256 KiB single-bank flash and RDP level 0 (0xAA).
FLASH_OPTR was read as 0x3ffff0aa. No protection change was made.

- Location on Lin:
  `/home/dbailey1564/Development/Backups/marketsys/prototype-01/20261020T193401.957087Z/`.
- Original image: `original-main-flash.bin`.
- Independent second read: `verification-read.bin`.
- SHA256 for both:
  `17edd1313d7e11163100b740c254091bcc6fe2d42798211a4c94eac5e54ba53b`.
- Scope: main flash, including bytes currently present in bootloader,
  application, and settings areas. Not a complete option-byte, OTP, or ROM backup.
- No reset, halt, erase, flash programming, or protection changes were issued.
- Restore has not been tested.

The backup folder is private and outside Git; do not add raw images to this
repository. Copying these potentially credential-bearing images to another
computer requires explicit destination approval. The user subsequently approved
the Windows copy. Both images were copied to
`C:\Users\Daniel\Documents\MarketSys-Backups\prototype-01\20261020T193401.957087Z\`
and their SHA256 hashes were verified against the recorded hash above.
Verified copies now exist on both Lin and Windows, outside the application Git
repository. The original blocked transfer made no copy; the later authorized
transfer succeeded.

Primary hardware references:
[RAK3272-SiP datasheet](https://docs.rakwireless.com/product-categories/wisduo/rak3272-sip-breakout-board/datasheet/),
[RAKDAP1 quick start](https://docs.rakwireless.com/product-categories/accessories/rakdap1/quickstart/).
