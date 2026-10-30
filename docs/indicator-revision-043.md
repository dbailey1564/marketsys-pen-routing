# Node revision 0.4.3: automatic boot light test

Prepared and deployed October 28, 2026 at the user's request. All three
nodes are uploaded and verified on 0.4.3. Central remains on 0.4.2 and
requires no change. The addressed radio protocol is unchanged.

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

## Node2 deployment and runtime verification completed

Node2 UID matched `003300265233500d20303938`; UART reported address 2,
FW=0.4.2, READY=1 and OFF. Two fresh full-flash reads matched its prior
verified snapshot. Private backup: `proto-node2-v1/20261028T105434.681825Z/`,
SHA256 `556b05c7f36e5f83a716ed07ebc72196af69c7c1430bfb5d5f095a49807a128c`.
The official UART uploader completed the prepared address-2 0.4.3 image.
Private readback verification:
`proto-node2-v1/20261028T105656.852729Z-post-addressed-upload/verification.json`.
All 175920 application bytes match SHA256
`a7e67422095e702b26b961b9d2938540acda56c661e20086d0b0a7be6ce1c763`.
UID matches and bootloader regions outside the reviewed upload-record range
are unchanged from its own pre-upload backup. Settings changed. Snapshot SHA256:
`2b6eb74973e5a480088ac6497104e1b43a7f927d36c9242ffd3706d34d92fd61`.

Runtime observer captured all five boot states and final OFF. Host-observed
step durations were 2.0000, 2.0027, 1.9960, 2.0023 and 1.9988 seconds;
total 9.9998 seconds. Ninety-nine GPIO reports passed color mapping, one-color
output, flashing ON/OFF observations and final LOW checks. Boot timing reports
were 250/2000 ms, reverting to 500/10000 ms after completion. Optical
verification and external waveform timing were not performed.

UART OFF during the next boot immediately cancelled the sequence. Outputs
remained LOW beyond the next scheduled step. Final status: NODE2, READY=1,
FW=0.4.3, NODE_ID=2, OFF, all GPIO LOW, BOOT_TEST=0 and normal timings.
Logs: build/node2-pre-043-backup.log, node2-043-upload.log,
node2-043-flash-verification.log, node2-043-boot-test.log,
node2-043-boot-summary.json, node2-043-boot-cancel.log and
node2-043-final-status.log.

Current deployment: Nodes1/2 are verified on 0.4.3; central and Node3 remain
on 0.4.2. The programmer is on Node2. Next: with breadboard power off, unplug
DAPLink USB, move all six connections to Node3, then reconnect USB. Keep its
antenna attached and use DAPLink as the only node power source. No post-update
LoRa or physical LED observation is claimed for either updated node yet.

## Node3 deployment completed; all three nodes verified

Node3 UID matched `003400345233500d20303938`; pre-upload UART reported
NODE_ID=3, FW=0.4.2, READY=1 and OFF. Two new full-flash reads matched its
prior verified snapshot. Private backup:
`proto-node3-v1/20261028T110123.097636Z/`, SHA256
`f5617334be673f65b76aaa615ac94ece22d802a8cf8d8e3d31fab253ba0d2b5f`.
The official RAK UART uploader completed its prepared address-3 0.4.3 image.
Private readback record:
`proto-node3-v1/20261028T110337.993332Z-post-addressed-upload/verification.json`.
All 175920 application bytes match SHA256
`24b7e627409890ce7ce87f2ab20b76ec6ec41d8dc4a10bb1b132122eaa60f834`.
UID matches and bootloader regions outside the reviewed upload-record range
are unchanged from its own pre-upload backup. Settings changed. Snapshot SHA256:
`419d83d437d8b7cf1396bcb7b31115d6bf350a9f594a15575f8d2cca9b2ac86e`.

The observer captured all five boot states and final OFF. Host-observed step
durations: 2.0011, 2.0040, 1.9970, 1.9970 and 2.0068 seconds; total 10.0059 s.
Ninety-nine GPIO reports passed expected colors, one-color output, flashing
ON/OFF observations and final LOW. Boot timing reported 250/2000 ms and
normal timing returned to 500/10000 ms. UART OFF during another boot cancelled
the sequence immediately; outputs stayed LOW beyond the next scheduled step.
Final status: NODE3, READY=1, FW=0.4.3, NODE_ID=3, OFF, all GPIO LOW and
BOOT_TEST=0. No optical measurement or external waveform capture was performed.
Logs: build/node3-pre-043-backup.log, node3-043-upload.log,
node3-043-flash-verification.log, node3-043-boot-test.log,
node3-043-boot-summary.json, node3-043-boot-cancel.log and
node3-043-final-status.log.

| Board | Deployed firmware | Address | Upload / readback | Boot GPIO / OFF cancellation |
| --- | --- | --- | --- | --- |
| Controller | 0.4.2 | Central, addresses 1-50 | Previously verified | Not a node boot test |
| Node1 | 0.4.3 | 1 | Passed | Passed |
| Node2 | 0.4.3 | 2 | Passed | Passed |
| Node3 | 0.4.3 | 3 | Passed | Passed |

All requested node boot-test uploads and GPIO/runtime checks are complete.
Physical LED observation and post-update addressed LoRa checks remain pending.
The single programmer is still on Node3. Next: with all power off, unplug
DAPLink USB and return its six connections to central. Power nodes separately
from regulated 3.3 V with antennas attached before radio checks. Do not parallel
DAPLink and breadboard 3.3 V outputs. The controller needs no firmware update.

## Node1 addressed radio indication test after 0.4.3 update

With central connected, Node1 acknowledged OFF token 485294525 on attempt 1.
The user then requested the complete indication test. Each normal command
below received APPLIED with its requested state on the first transmission.
Each illuminated state was given its full normal ten-second window before
advancing; flashing uses normal 500 ms phases rather than the shorter boot
sequence timings. The final fresh OFF received APPLIED/OFF on attempt 1.

| State | Token | Result | Attempt |
| --- | --- | --- | --- |
| ACTIVE | 2070760980 | APPLIED / ACTIVE | 1 |
| LAST | 3724716211 | APPLIED / LAST | 1 |
| HOLD_OPEN | 2242584524 | APPLIED / HOLD_OPEN | 1 |
| FULL | 3948410128 | APPLIED / FULL | 1 |
| FAULT | 332631230 | APPLIED / FAULT | 1 |
| OFF | 3558796681 | APPLIED / OFF | 1 |

Received RSSI ranged -25..-24 dBm and SNR 12..13 dB. Central reported FW=0.4.2
and test-injection flags zero. Node1's last physically verified firmware is
0.4.3; the ACK protocol does not itself transmit a firmware-version field.
These results establish addressed transport and node-reported state application,
not optical correctness. User visual confirmation of the five patterns and
final darkness is pending. Nodes2/3 still need post-update radio tests.
Logs: build/node1-043-visual-active.log, node1-043-visual-last.log,
node1-043-visual-hold-open.log, node1-043-visual-full.log,
node1-043-visual-fault.log, node1-043-visual-final-off.log.

## Source rollback to 0.4.3 after repaired connections (October 30)

The user reported repaired connections and requested Lin/GitHub rollback to
0.4.3, explicitly confirming the nodes were not flashed and remain at 0.4.3.
The node sketch, BootLightTest helper, boot-test unit test and both runtime
verification scripts were restored exactly from the parent of the 0.4.4
change (af84707). No additional ten-second delay remains. The original
five-second startup window plus setup and ten-second indication sequence
remain. Central stays on 0.4.2. No hardware was flashed or reset.

Host boot-test checks (with address/undefined-behavior sanitizers), normal
pattern/codec/CRC tests and all-50-address isolation tests passed. All three
node images were rebuilt successfully; program/RAM usage matches 0.4.3.
Each rebuilt application was compared with the application slice of its own
verified 0.4.3 flash snapshot. The only differences are eight bytes within
the automatically generated build time/date strings. RUI3 platform.txt's
prebuild hook invokes buildtime.py to regenerate the variant version.c file.
Every application byte outside that timestamp field is identical. The nodes'
original deployed image hashes above remain correct; these are rebuilt images:

| Target | Rebuilt 0.4.3 SHA256 |
| --- | --- |
| Node1 | `09bc72fec93945f11c80726f6944f6f9b76bfd0a4b80c59c5ab42aca07b62bcd` |
| Node2 | `a75a4cad4d9ed20e3f2e36f72a17b2974826b4fbdbc2db040a70e28487bd5d1f` |
| Node3 | `8466da5ffa9fdc1e39558cf29432b8529c3ba45538f21716d635ce83f87e4854` |

Logs: build/radio-node1-043-rollback-build.log,
radio-node2-043-rollback-build.log, radio-node3-043-rollback-build.log,
and 043-rollback-comparison.log. The abandoned 0.4.4 proposal and later hardware
observations are retained as historical records; 0.4.4 is marked withdrawn and
never flashed. This rollback makes Lin/GitHub source match the installed
firmware revision without rewriting history or discarding wiring/test records.
The user's repaired-connection report does not itself add an optical pass
for any earlier failed test; subsequent visual results should be recorded
as they are supplied.
