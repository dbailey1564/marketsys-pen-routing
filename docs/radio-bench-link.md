# First controller-to-Node1 radio link

October 24, 2026. This follows completed physical and visual verification of
all seven Node1 indicator states. One DAPLink is available, currently on Node1.

## Firmware and radio choices

`firmware/radio_node` is PROTO - Node1 V1's receiver/indicator application.
`firmware/radio_central` is the controller's command/ACK/retry application.
Build with `python3 scripts/dev.py radio-node` and `radio-central`.
Both use the existing pinned RUI3 4.2.4 SiP toolchain, including its normal
default LoRa support setting. Do not select the previously failing P2P-only
supportlora=4 option. Shared headers are in `libraries/MarketSysBench`.

Bench settings: raw LoRa P2P at 915000000 Hz, SF7, 125 kHz bandwidth (RUI3
pbw index 0), coding rate 4/5 (pcr 1), preamble 8, 10 dBm power, normal IQ,
syncword 0x1424, encryption disabled. These are provisional lab choices, not
a completed production RF/channel plan. Both devices use continuous receive
with TX allowed (precv 65533), no sleep schedule. The implementation follows
the installed RAKLoRa.cpp and service_lora_p2p.c plus the pinned vendor examples:
https://github.com/RAKWireless/RUI3-Best-Practice/tree/3460377f44fe17885397335027d2fa8eb66dc11b/LoRa_P2P

Node1 retains PB3 blue, PB4 red, PB5 green, one color at a time, 2 ms switching
gap, 500 ms phases, OFF startup, and the 10-second bench cutoff. This cutoff
is not a production failure policy. State names and values are OFF=0, ACTIVE=1,
NEXT=2, STANDBY=3, HOLD_OPEN=4, FULL=5, FAULT=6. Patterns are unchanged from
the verified indicator harness. No cattle, buyer, or routing metadata is sent.

Node1 image SHA256:
`0f6b3736798a464fcc4b1851470954dd2b5c13df0606f6563b1821ebec9205e1`.
Initial controller 0.3.0 image SHA256 (superseded by 0.3.1 below):
`daa355302123ccb06d87539379c8ceddf27747442de6d7da45c4bf2734a5593c`.
Custom application versions are MS-Node1-RF-0.3.0 and MS-Central-RF-0.3.0.

Deployment requirement: after the first startup changes saved mode from
LoRaWAN to P2P, restart once with `scripts/restart_radio.py` (ATZ) or a power
cycle. Installed service_lora.c initializes service_lora_p2p_init at SDK
startup, whereas RAKLoRa.cpp nwm.set only changes the saved mode. READY=1
alone is not an end-to-end radio test. Node1 received this restart during
its power-source transfer. The controller required the explicit ATZ below.

## Prototype radio frame (not the host binary protocol)

The frame is exactly 14 bytes, serialized explicitly without struct padding:

| Byte(s) | Meaning |
| --- | --- |
| 0-1 | ASCII MS |
| 2 | Version 1 |
| 3 | Type: SET=1, ACK=2 |
| 4 | Node address, currently 1 |
| 5 | Commanded state in SET; applied current state in ACK |
| 6 | Result: APPLIED=0, DUPLICATE=1, EXPIRED=2, CONFLICT=3 |
| 7 | Reserved, must be zero |
| 8-11 | Nonzero 32-bit transaction token, little-endian |
| 12-13 | CRC-16/CCITT-FALSE over bytes 0-11, little-endian |

The host bench helper chooses a fresh random token for each new command.
Central reuses it for at most three total attempts. ACK deadline is 1000 ms
after the TX callback; a missing TX callback is bounded at 2000 ms. Node1 waits
50 ms before ACK transmission. The controller matches ACK type/address/token,
reports ACK only for APPLIED/DUPLICATE with the requested state, and reports
REJECTED for a conflicting/expired result. Exhausted attempts report FAILED.
Initial status is UNKNOWN until a matching ACK is received.

Node1 remembers its most recent token and requested state in RAM. A duplicate
does not restart the indicator or the bench timeout. A duplicate after expiry
reports OFF/EXPIRED. Reusing a token for a different state returns CONFLICT.
This one-entry prototype cache does not reject arbitrarily old/reordered
transactions, persist across reset, or provide authentication. Ordering,
session/restart semantics, polling, and security require production design.

The central CONFIRMED field means last node-reported state with AGE_MS; it is
not a live poll or optical confirmation. In particular, the node's automatic
bench cutoff does not send a spontaneous OFF update. Command OFF explicitly
to obtain a fresh OFF ACK. Failed commands retain the prior confirmed state.
ACK confirms firmware state application, not measured LED current/illumination.

## Bench UART controls

On Node1: `ATC+NODE=?` reports role/configuration and state/GPIO/token.
`ATC+NODE=OFF` locally clears the indicators. The old LED/STATE AT commands
belong to the earlier harness and are not registered in the radio image.

On central: `ATC+PEN=?` reports link status. `ATC+PEN=<decimal-token>:<STATE>`
commands node 1. New commands are rejected while a transaction is pending.
This is a bench interface; USB binary framing/CRC decisions remain open.

Use marketsys-uart-venv Python with `scripts/radio_link.py node-status`,
`central-status`, or a lowercase state name. State commands require central
attached and wait for a matching terminal result. `--token` allows deliberate
duplicate/conflict tests. No end-to-end command is sent automatically at boot.

## One-programmer power transfer

1. Unplug DAPLink USB and switch off breadboard power.
2. Remove DAPLink's six wires from Node1. Keep Node1's GPIO/LED wiring and
   its separate breadboard-GND connection in place.
3. Connect breadboard 3.3 V to Node1's printed 3V3 pin formerly powered by
   DAPLink. Breadboard GND remains connected to Node1 GND and driver IN-.
   Driver IN+ stays on 5 V. Driver L- remains the separate MOSFET source return.
4. Transfer DAPLink to central: 3V3->3V3, GND->GND, TMS/IO->PA13,
   TCK/CK->PA14, UART TX->PA3, UART RX->PA2. Leave 5V/nRST/TDI/TDO unused.
5. Verify both antennas attached. Power the breadboard; Node1 should remain
   dark. Connect DAPLink USB to Lin for the controller.

Never parallel DAPLink 3V3 with the breadboard 3.3 V output. A radio link does
not require a ground wire between the two separately powered boards.
Board labels follow the [RAK3272-SiP pinout](https://docs.rakwireless.com/product-categories/wisduo/rak3272-sip-breakout-board/datasheet/).

## Verification status

Both applications compiled. Host tests passed the standard CRC check vector,
all single-bit frame corruptions, wrong-length/header validation, endianness,
duplicate/expired/conflict behavior, and state-name mapping. The earlier pattern
boundary/timeout tests remain applicable to the copied BenchPattern definition.
Node1 was uploaded with the official RAK uploader; its UART report confirmed
READY=1, 915000000 Hz, SF7, BW0, CR1, power 10, all three GPIO LOW, state OFF.
Node1's original factory and earlier full readback backups remain available.
Its post-upload readback is
`20261024T204405.227060Z-post-radio-node-upload/verification.json` in Node1's
private backup area: all 175536 application bytes matched the compiled image,
with bootloader code and activation marker unchanged outside the reviewed
update-record range. Readback SHA256:
`b5092d22a00826bcb4b5f16ec0446ef0f81c3ce5331d200c73d533869c4dd4d5`.
Settings changed from the factory backup as expected for radio configuration;
settings preservation is not claimed. Restore remains untested.

## Controller transfer and installation

The user confirmed the programmer was transferred and separately powered
Node1 remained dark. UART answered from the controller, but initial SWD access
failed. AT queries found LPM=1, LPMLVL=2 from the prior sleep harness. Setting
LPM=0 alone did not restore SWD. Entering the existing RUI bootloader with
AT+BOOT restored debug access without rewiring or replacement firmware.
This supports the sleep harness as the cause; it is not a general SWD fault
diagnosis. Bootloader entry changes the boot state, and LPM was deliberately
changed before the fresh backup; the original factory backup remains intact.

A fresh pre-radio backup on Lin is
`/home/dbailey1564/Development/Backups/marketsys/prototype-01/20261024T205854.538791Z/`.
Both 262144-byte reads matched, SHA256
`7249865b1b1f607440db52952a3378b7e991583ac39f6cd4221a64fbff4f846d`.
The read UID matched the original controller's recorded UID; protection
remained level 0. The controller image was installed through RAK's official
UART uploader. Helpers prepare_controller_radio.py and
enter_controller_bootloader.py document the preparation; neither uploads an
image. verify_controller_upload.py checks the installed image and UID.

Controller readback record:
`20261024T210129.913260Z-post-radio-central-upload/verification.json` under
its private backup area. UID matched, all 175168 application bytes matched
the prepared image, and bootloader code/activation marker were unchanged
outside the reviewed update-record range. Full readback SHA256:
`4c50119d5c056213fd7c623868b7a657afffe22a7c31dc7329892fba2f7e2d66`.
Settings differed from the factory backup; preservation is not claimed.

## First over-air results

The first OFF request (token 1024202601) exhausted three queued attempts with
no P2P TX callback or ACK. The controller reported FAILED/CONFIRMED=UNKNOWN,
not success. SDK startup inspection identified the saved-mode transition
requirement above. ATZ restarted central in LoRa P2P mode. The repeated OFF
test with fresh token 1024202602 received APPLIED/OFF on attempt 1, RSSI -19,
SNR 13. The matching address/token and frame CRC were validated by firmware.
These are one bench exchange's reported signal values, not range results.

Repeating token 1024202602 with OFF received DUPLICATE/OFF on attempt 1.
Reusing that token for FULL received CONFLICT/OFF and central reported
REJECTED rather than ACK. The helper's exception in this deliberately
rejected test was expected. Logs are build/radio-first-off.log,
radio-central-restart.log, radio-first-off-after-restart.log,
radio-duplicate-off.log, and radio-token-conflict.log.

ACTIVE with fresh token 1024202603 received APPLIED/ACTIVE on attempt 1.
After the 10-second bench timeout, repeating that transaction received
EXPIRED/OFF on attempt 1 and central correctly reported REJECTED with
CONFIRMED=OFF. It did not restart the pattern. A fresh OFF transaction
(1024202604) received APPLIED/OFF on attempt 1. Logs:
build/radio-active-visual.log, radio-expired-duplicate.log, radio-final-off.log.
The user subsequently confirmed flashing green and complete darkness after
the approximately 10-second activation, with red and blue remaining dark.

The basic bidirectional radio path and duplicate/conflict/expired handling
passed. Final node-reported state is OFF.
Deliberate lost-ACK/retry and unavailable-node testing were subsequently
completed below.
The initial three-attempt failure exercised the bounded watchdog path but
does not establish a controlled lost-ACK test.

## Completed wireless visual state verification

The user confirmed all six non-OFF patterns separately over the radio link,
including the return to complete darkness after approximately 10 seconds and
the unselected dies remaining dark. Every state command and subsequent fresh
OFF command received an APPLIED ACK on the first attempt:

| State | State-command token | Observed pattern | Subsequent OFF token |
| --- | --- | --- | --- |
| ACTIVE | 1024202603 | Flashing green | 1024202604 |
| NEXT | 1334762276 | Flashing blue | 148992949 |
| STANDBY | 2077549842 | Solid blue | 4132318881 |
| HOLD_OPEN | 684982114 | Alternating green/blue | 449476807 |
| FULL | 1420175036 | Solid red | 1899478556 |
| FAULT | 2176498190 | Flashing red | 785359124 |

OFF was verified at startup and after every pattern. The final transaction
785359124 received APPLIED/OFF on attempt 1. This establishes all seven
indicator states commanded controller-to-Node1 over LoRa, with matching ACKs
and user visual confirmation. It does not establish range, production power
performance, or controlled lost-packet recovery.

## Unpowered-node failure and recovery

The user switched OFF Node1's breadboard supply while leaving the controller
powered through DAPLink. ACTIVE token 1024202701 produced exactly three
transmit attempts, each with a P2P TX-done event and no ACK. Central reported
FAILED, ATTEMPTS=3, COMMANDED=ACTIVE, retaining last CONFIRMED=OFF rather
than falsely confirming ACTIVE. Log: build/radio-node-unpowered.log.
The host helper's failure exception was expected for this deliberate test.

The user restored breadboard power, waited for startup and confirmed Node1
remained dark. Fresh ACTIVE token 1024202702 received APPLIED/ACTIVE on
attempt 1. The user confirmed approximately 10 seconds of green flashing,
then complete darkness, with red and blue remaining dark. A fresh OFF
transaction 1024202703 received APPLIED/OFF on attempt 1. Logs:
build/radio-node-recovery.log and build/radio-node-recovery-off.log.

This verifies bounded failure reporting with an unavailable node and recovery
without restarting or reprogramming the controller. It does not simulate a
command delivered successfully followed by a lost ACK; that test is below.
Final node-confirmed state after this milestone is OFF.

## Controlled lost-ACK test: controller 0.3.1

Node1 stays on its verified 0.3.0 receiver image. Controller 0.3.1 adds a
one-shot UART-only bench control: ATC+DROP=1 arms the next accepted transaction
to ignore its first matching successful ACK. ATC+DROP=0 disables it; query
with ATC+DROP=?. It defaults disabled on startup and is not persisted.
The arm is consumed when a valid PEN command starts; the transaction latch
clears after the ignored ACK or terminal completion. Rejected ACKs are not
ignored. Ignoring an ACK leaves its retry deadline and previous confirmation
unchanged. Serial status reports both arm and transaction-latch state.
Normal packet format, RF settings and Node1 code are unchanged.

The central image compiled, uploaded and passed full flash readback:
175720 application bytes, SHA256
`a3cb2959747dc25192dca860cefd92084a128c6491b34081eefc889a878c541f`.
Private verification record:
`prototype-01/20261024T211958.770610Z-post-radio-central-upload/verification.json`.
Hardware UID matched and bootloader code/activation marker were unchanged
outside the reviewed update-record range. Full snapshot SHA256:
`1e6be56adb2800ca59b638d1b5baf4b53151a593457fa57582328dd56545b539`.
Settings differ from the factory backup; their preservation is not claimed.

Live script `scripts/lost_ack_test.py`, logged to build/radio-lost-ack-test.log:

- Established fresh OFF confirmation with token 1781629467.
- Armed one-shot dropping, sent ACTIVE token 2186859522, and observed the
  first matching APPLIED ACK received over air but intentionally ignored.
- Queried during the retry interval: transaction remained PENDING, attempt 1,
  last CONFIRMED=OFF. There was no false delivery confirmation.
- Observed exactly attempts 1 and 2. Attempt 2 received DUPLICATE/ACTIVE and
  completed ACK/CONFIRMED=ACTIVE, demonstrating retry recovery.
- Repeated the same token at 10.369 seconds after the original command write;
  EXPIRED/OFF was received by 10.623 seconds, before the approximately
  11-second cutoff that a timer restart on retry would produce. The script
  rejects a missed timing window instead of claiming timing success.
- Disabled test injection explicitly and obtained APPLIED/OFF on attempt 1
  using fresh token 1479022242. Both test flags were zero afterward.

This simulates loss at the controller's ACK-consumption boundary; the first
ACK was physically received, not jammed or corrupted over RF. It verifies
the real over-air resend/duplicate path and retention of the original bench
timeout. It is not a statistical interference or packet-loss-rate test.
The user confirmed green flashed for approximately 10 seconds and then went
completely dark during this injected-loss run, with red and blue remaining
dark. The lost-ACK recovery milestone therefore passed both automated and
visual verification. Final confirmed state remains OFF, injection disabled.
