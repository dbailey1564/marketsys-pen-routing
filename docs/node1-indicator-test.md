# PROTO - Node1 V1: indicator bench firmware

October 24, 2026. The user connected Node1 alone to the DAPLink on Lin.
The breadboard and LED driver remained disconnected during programming.

## Identification and backup

SWD reported STM32WL device ID 0x10016497. UART reported original firmware
RUI_4.0.6_RAK3272-SiP. A separate Node1 backup was made on Lin:

`/home/dbailey1564/Development/Backups/marketsys/proto-node1-v1/20261024T192523.820282Z/`

Two independent 262144-byte reads matched, SHA256:
`17edd1313d7e11163100b740c254091bcc6fe2d42798211a4c94eac5e54ba53b`.
Read protection was level 0; no unlock was performed. Backup includes main
flash, not OTP/system ROM or a complete option-byte image. Restore is untested.
The backed-up bootloader matched all 12608 packaged bootloader bytes plus
the four-byte activation marker. Private backup images remain outside Git.

## Installed application

Source: `firmware/indicator_test/indicator_test.ino`.
Build: `python3 scripts/dev.py indicator-test`, pinned RUI3 4.2.4, SiP FQBN.
Upload used RAK's installed uploader_ymodem.py and the DAPLink UART.
Application SHA256:
`4260f24e80da0deebdd8ef0a24116cd10a3358bb5ca480bf23bd9ecabdd49e12`.

This is an LED bench harness, not the seven-state or radio application.
PB3 (J8 pin 2) selects blue; PB4 (J8 pin 3) selects red; PB5 (J8 pin 4)
selects green. All are initialized LOW. No color starts automatically.
The application disables low-power mode for this bench test and sets its
custom firmware version to MS-Node1-LED-0.1.0. AT+VER reports the SDK version,
RUI_4.2.4_RAK3272-SiP, rather than that custom version.

UART is 115200 baud, 8N1, AT mode:

| Command | Behavior |
| --- | --- |
| ATC+LED=? | Report selected color, GPIO readbacks, and timeout |
| ATC+LED=OFF | All outputs LOW |
| ATC+LED=BLUE | PB3 HIGH; PB4/PB5 LOW |
| ATC+LED=RED | PB4 HIGH; PB3/PB5 LOW |
| ATC+LED=GREEN | PB5 HIGH; PB3/PB4 LOW |

Every color write first clears all outputs and inserts a 2 ms switching gap.
Each activation expires after 10 seconds, enforced by the running loop.
This timeout is software behavior, not an independent hardware cutoff.
Malformed LED writes leave all outputs OFF and return an error.

On Lin, use the marketsys-uart-venv Python with `scripts/node1_led.py` and
one of `status`, `off`, `blue`, `red`, `green`, or `verify`.
The verify action actively exercises all three GPIO outputs; use it with
the LED driver disconnected unless those activations are intended.

## Verification and limits

`build/node1-led-verification.log` records successful individual color
readbacks, mutual exclusion, 10-second automatic OFF, invalid-command OFF,
and final OFF. These are MCU digital input readbacks of its output pins,
not meter measurements or confirmation of actual LED illumination.
`scripts/verify_node1_upload.py` compares the installed application and
preserved bootloader bytes with Node1's own backup. Its private readback
and verification record are stored alongside the Node1 backup directories.
The completed readback record is
`20261024T192956.632521Z-post-indicator-upload/verification.json`:
all 173816 application bytes matched the compiled image. Bootloader bytes
outside its reviewed update-record range (0x08004004-0x0800400F) were unchanged.
The update record and settings region changed during the normal upload and
application initialization; the settings region is not claimed preserved.
No bootloader replacement or protection change was performed.

Pending physical work: resistor-installed all-OFF validation, full bench
circuit hookup, and actual LED ON/OFF tests. Leave the breadboard disconnected
until that hookup step. Driver EN and whole-node OFF current remain unverified.

Hardware references:
[RAK3272-SiP pinout](https://docs.rakwireless.com/product-categories/wisduo/rak3272-sip-breakout-board/datasheet/),
[FDC6401N pinout](https://www.onsemi.com/download/data-sheet/pdf/fdc6401n-d.pdf),
and `docs/indicator-bench-wiring.md` for the corrected physical circuit.
