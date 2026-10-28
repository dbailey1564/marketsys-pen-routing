# Node revision 0.4.3: automatic boot light test

Prepared October 28, 2026 at the user's request. All physical boards remain
on verified 0.4.2 until individually uploaded. Central requires no firmware
change; 0.4.3 is a node-only revision with the same addressed radio protocol.

## Startup indication

After the existing five-second startup window and successful node/radio setup,
run the current indicator patterns once with 250 ms flashing phases and
2000 ms per state. The sequence takes 10 seconds after setup:

| Time after test start | State | Indication |
| --- | --- | --- |
| 0 to <2 s | ACTIVE | Solid green |
| 2 to <4 s | LAST | Solid blue |
| 4 to <6 s | HOLD_OPEN | Green 250 ms on / 250 ms off |
| 6 to <8 s | FULL | Solid red |
| 8 to <10 s | FAULT | Red 250 ms on / 250 ms off |
| 10 s onward | OFF | All GPIO LOW |

The node stays dark during its initial startup window. If node/radio setup
fails, the light sequence is not started and outputs remain dark. Startup
printing identifies the test and each step; DONE reports final OFF. It is
an output exercise, not optical feedback or detection of failed LED wiring.

The test uses the same color-pattern implementation as normal commands with
separate timing parameters. Normal commanded patterns retain 500 ms phases
and the 10-second bench cutoff. No indicator codes, pins, gate sequencing,
current setting, node-address range, radio timing or retry rules change.

The test is nonblocking: loop/radio handling continues. A new valid addressed
SET command, including OFF, cancels the boot test immediately and uses normal
command timing. UART ATC+NODE=OFF also cancels it. Status queries do not restart
or cancel it. Invalid frames, another node's command and retired state 3 do
not interrupt it. The boot sequence does not create application/radio tokens
or deduplication entries. Cancelled/completed tests do not restart until reboot.
UART status includes actual PHASE_MS, LIMIT_MS and BOOT_TEST during the test.

## Validation

Host tests passed with AddressSanitizer and UndefinedBehaviorSanitizer for
all five states, 249/250 ms and 1999/2000 ms boundaries, final OFF, millis()
rollover, skipped/late loop updates, cancellation and unchanged normal timings.
Existing codec/CRC/duplicate/state-pattern tests and all-50-address isolation
checks passed. Node1/2/3 images compiled with pinned RUI3 4.2.4 SiP toolchain.
Program storage is at most 160872 bytes; static RAM is 33920 bytes, leaving
14720 bytes in the compiler's reported static allocation. This is not a runtime
stack/heap measurement. No on-device/visual result is claimed before upload.

| Prepared target | SHA256 |
| --- | --- |
| Node1 | `9d62f2512c5e208bf7f9a11ddf6f4bd76f67c4ee9ee97f08d5cd85b8a225f856` |
| Node2 | `a7e67422095e702b26b961b9d2938540acda56c661e20086d0b0a7be6ce1c763` |
| Node3 | `24b7e627409890ce7ce87f2ab20b76ec6ec41d8dc4a10bb1b132122eaa60f834` |

Build: python3 scripts/dev.py radio-node --node-id N. Images remain separated
under build/radio-node-N. New host test: tests/boot_light_test.cpp.

## Deployment pending

The programmer was last confirmed on central. To deploy, turn breadboard
power off and unplug DAPLink USB, disconnect the node's external 3.3 V supply,
move all six programmer connections to Node1, and reconnect USB. Use only
DAPLink power while programming and keep its antenna attached. Verify UID,
take a fresh two-read private backup, upload the explicit address-1 image,
read back all application bytes and verify preserved bootloader regions.
Repeat with each board's own identity and address-specific image for Nodes 2/3.

For visual verification, restore the normal bench 3.3 V node / 5 V driver
supplies with common GND after disconnecting DAPLink power, and power-cycle
with the RGB wiring connected. Expect the complete sequence above and final
OFF. Then verify a real command can cancel a running boot test and that normal
command timing remains 500 ms / 10 seconds. Host binary interface and automatic
head-entry flow remain separate, unimplemented work.
