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

## Initial application: 0.1.0 (superseded by 0.2.0 below)

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

## Physical LED test results

After the bench hookup, the user reported all three dies dark at initial OFF.
The user then confirmed blue and red each illuminated for approximately
10 seconds and went completely dark, with the other colors remaining dark.
The initial green command produced no illumination. The user identified the
green source connection on IC2 P2 instead of P5, corrected it, and confirmed
the repeated green test illuminated for approximately 10 seconds and returned
completely dark, with red and blue remaining dark. This correction did not
require a firmware change. Serial queries after every activation confirmed
PB3/PB4/PB5 all LOW. These observations establish the three individual color
ON/OFF bench tests; they do not establish zero OFF current.

Bench power: DAPLink 3V3 powers Node1; breadboard 5 V powers driver IN+;
Node1 GND, DAPLink GND and driver IN- share supply ground. Breadboard 3.3 V
is unused. Driver L- remains the MOSFET source return, separate from the
supply-ground junction. Driver EN remains disconnected and insulated.

No flash write was needed for those initial physical tests.

## Seven-state bench application: 0.2.0

The existing sketch now implements the seven agreed indicator states.
Wiring is unchanged. `ATC+STATE=?` reports requested state, elapsed time,
actual selected color, GPIO digital readbacks, phase timing, and application
version 0.2.0. `ATC+STATE=<name>` accepts these exact uppercase names:

| State | Pattern |
| --- | --- |
| OFF | All outputs LOW |
| ACTIVE | Green 500 ms, dark 500 ms, repeat |
| NEXT | Blue 500 ms, dark 500 ms, repeat |
| STANDBY | Solid blue |
| HOLD_OPEN | Green 500 ms, blue 500 ms, repeat |
| FULL | Solid red |
| FAULT | Red 500 ms, dark 500 ms, repeat |

The 500 ms phases are provisional demo choices, not approved production
timings. Color changes clear all outputs before a 2 ms gap and then enable
the selected color. Scheduling uses elapsed time, so this gap slightly
shortens the illuminated part of a phase. Only one color is driven at a time.
All non-OFF states expire after 10 seconds for this bench harness; this is
not a production command-expiry or radio-failure policy. Reissuing a state
restarts its phase and timeout. Invalid state writes clear the outputs and
return an error. Queries do not restart the state. Startup remains OFF.
The earlier ATC+LED color commands remain available for diagnosis and are
reported as TEST_BLUE/TEST_RED/TEST_GREEN by the state query.

Application SHA256:
`da29bd337e05cfce2aa8c967c55ede4a5a4ceaee7b5a35eb6e4207dc7157503c`.
Custom firmware version is MS-Node1-LED-0.2.0; AT+VER still reports the SDK.

Host tests in `tests/indicator_pattern_test.cpp` passed checks of all states,
phase boundaries, cutoff boundaries, manual diagnostics, and unsigned timer
rollover. `scripts/node1_states.py` supports `status`, each lowercase state
name, and `verify`. Verification actively exercises LEDs and ends at OFF.
It samples both phases and checks GPIO exclusivity, the timeout, and
invalid-command OFF behavior. Its readbacks do not replace visual checks.

Completed 0.2.0 verification: `build/node1-states-verification.log` passed
all seven states, both phases, GPIO exclusivity, the 10-second timeout and
invalid-command OFF. Flash verification record:
`20261024T202935.668017Z-post-indicator-upload/verification.json` in Node1's
private backup area. All 174480 application bytes matched the compiled image.
Bootloader code and activation marker were unchanged, excluding its reviewed
update-record range. Full readback SHA256:
`abc05878ca2f3ab80b8abbdddb55b878641d0043376f74b0829d202b82927fc3`.
The update record and settings differed from the original factory backup;
settings preservation is not claimed. Final automated state was OFF.

## Completed visual seven-state checks

The user confirmed the initial OFF condition and each separately commanded
pattern: ACTIVE flashing green, NEXT flashing blue, STANDBY solid blue,
HOLD_OPEN alternating green/blue, FULL solid red, and FAULT flashing red.
For each non-OFF state the user confirmed approximately 10 seconds of the
expected illumination followed by complete darkness, with unselected colors
remaining dark. Each post-test serial query reported state OFF and all three
GPIO readbacks LOW. Final confirmed condition after FAULT was OFF.
This establishes visual bench operation of all seven states on Node1 V1;
it does not establish measured zero leakage or production runtime.

Pending: radio/host integration, driver EN control, and measured whole-node
OFF current. Phase timing and the 10-second timeout remain bench settings.

Hardware references:
[RAK3272-SiP pinout](https://docs.rakwireless.com/product-categories/wisduo/rak3272-sip-breakout-board/datasheet/),
[FDC6401N pinout](https://www.onsemi.com/download/data-sheet/pdf/fdc6401n-d.pdf),
and `docs/indicator-bench-wiring.md` for the corrected physical circuit.
