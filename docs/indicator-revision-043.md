# Node revision 0.4.3: automatic boot light test

Prepared October 28, 2026 at the user's request. Node1 is now uploaded and
verified on 0.4.3; Nodes 2/3 remain on 0.4.2 pending their uploads. Central
remains on 0.4.2 and requires no change. The addressed radio protocol is unchanged.

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

## Deployment procedure

For each node upload, turn breadboard
power off and unplug DAPLink USB, disconnect the node's external 3.3 V supply,
move all six programmer connections to the selected node, and reconnect USB. Use only
DAPLink power while programming and keep its antenna attached. Verify UID,
take a fresh two-read private backup, upload that node's explicit-address image,
read back all application bytes and verify preserved bootloader regions.
Repeat with each board's own identity and address-specific image for Nodes 2/3.

For visual verification, restore the normal bench 3.3 V node / 5 V driver
supplies with common GND after disconnecting DAPLink power, and power-cycle
with the RGB wiring connected. Expect the complete sequence above and final
OFF. Then verify a real command can cancel a running boot test and that normal
command timing remains 500 ms / 10 seconds. Host binary interface and automatic
head-entry flow remain separate, unimplemented work.

## Node1 deployment and runtime verification completed

Node1 UID matched `003300455233500d20303938`; pre-upload UART reported
0.4.2, READY=1, OFF. Two new full-flash reads matched the prior verified
0.4.2 snapshot. Private backup: `proto-node1-v1/20261028T104512.392026Z/`,
SHA256 `3cbbe04981a4176a4e49bc0255f202f708d141dcfd552302b3928d38c514a6d6`.
The official UART uploader completed the prepared address-1 0.4.3 image.
Private SWD verification record:
`proto-node1-v1/20261028T104744.121347Z-post-addressed-upload/verification.json`.
All 175928 application bytes match its prepared SHA256. UID matches, and
bootloader regions outside the reviewed update-record range are unchanged
from Node1's own pre-upload backup. Settings changed; full settings preservation
is not claimed. Full snapshot SHA256:
`874af3204735a5d6a0acc73cfdc580905e8769793e2d119c22bd5ef6a2165d75`.

Runtime observer restarted Node1 and captured ACTIVE, LAST, HOLD_OPEN, FULL,
FAULT, then OFF. Host-observed step durations were 2.0010, 2.0030, 1.9969,
2.0009 and 2.0031 seconds; total 10.0049 seconds. Ninety-nine GPIO reports
showed the expected color mappings with no simultaneous HIGH colors; both
flashing states included ON and OFF samples. The node reported boot phase
250 ms and step limit 2000 ms, followed by normal 500 ms / 10000 ms values.
This is GPIO/internal timing validation, not optical measurement or an external
scope capture. The user has not yet confirmed the physical light sequence.

A second restart plus ATC+NODE=OFF during the first test step produced
CANCELLED immediately; repeated queries stayed OFF with all three GPIO LOW
for 2.5 seconds, past the next scheduled boot step. Final UART status:
NODE1, READY=1, FW=0.4.3, NODE_ID=1, OFF, PB3/PB4/PB5=0, BOOT_TEST=0,
PHASE_MS=500 and LIMIT_MS=10000. No post-update LoRa command is claimed yet.

Logs and summary: build/node1-pre-043-backup.log, node1-043-upload.log,
node1-043-flash-verification.log, node1-043-boot-test.log,
node1-043-boot-summary.json, node1-043-boot-cancel.log and
node1-043-final-status.log. Reusable tooling: scripts/observe_boot_light_test.py
and scripts/check_boot_cancel.py. Node2 and Node3 builds remain ready; the
single DAPLink is still on Node1. Next: power off, unplug USB, move all six
connections to Node2, and reconnect with DAPLink as its only power source.
Keep the breadboard supply off and the node antenna attached.
