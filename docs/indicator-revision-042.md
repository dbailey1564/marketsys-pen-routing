# Indicator revision 0.4.2: addressed nodes 1-50

User-authorized October 24, 2026: add addressing for 50 nodes, then deploy
controller followed by Node1, Node2 and Node3. This revision includes the
prepared 0.4.1 LAST rename and adds explicit node addressing.

## Firmware behavior

- Valid addresses are 1 through 50; address 0 and addresses above 50 are rejected
  by the controller command parser. The existing 14-byte radio frame is unchanged.
- Each node accepts SET requests only for its compiled address and returns that
  address in its ACK. Other nodes ignore the request without an acknowledgement.
- Central retains a separate last-node-confirmed state and timestamp for each
  address in RAM. A reply must match the pending node and token. Only one radio
  transaction is active at a time; this is not a 50-command concurrent queue.
- Diagnostic command: `ATC+PEN=<token>:<node>:<state>`. The existing two-field
  `<token>:<state>` form remains an explicit Node1 compatibility shortcut.
- `scripts/radio_link.py last --node 2` requests LAST from Node2. `--node` is
  also required to identify the expected node for node-status (default 1).
- `ATC+PEN=?` reports the most recent transaction, `MS_TARGET_NODE` and the
  target's last-node-confirmed state. It does not poll all 50 nodes.
- Builds require `python3 scripts/dev.py radio-node --node-id N`, with N=1..50.
  Each image is kept in its own `build/radio-node-N/` directory. Firmware source
  has no silent default address and reports its compiled NODE identity.
- States: OFF=0, ACTIVE=1 solid green, LAST=2 solid blue, HOLD_OPEN=4 flashing
  green, FULL=5 solid red, FAULT=6 flashing red. State 3 remains retired.
- 500 ms flash phases, startup OFF, 10-second automatic OFF, bounded retries
  and one-transaction lost-ACK test control are retained.

This is addressed bench firmware. It does not implement the proposed binary
application protocol, controller-owned head-entry flow, persistent confirmation
storage, production illumination persistence, or production power management.
A historical confirmed state is not a live poll after a node's bench cutoff.
The app's numeric/alpha destination is separate from these radio device IDs.

## Validation and deployment

Host addressing tests cover every address 1..50 and every addressed-node /
receiver combination, token overflow, invalid addresses and states, legacy
Node1 parsing, wrong-node/wrong-token ACK rejection and independent state
records. Addressing tests passed with AddressSanitizer and UndefinedBehaviorSanitizer;
existing codec/CRC/duplicate/state-pattern checks passed. The prepared targets
use the pinned RUI3 4.2.4 SiP toolchain.

Before any upload, identify the physical board and retain two matching
262144-byte main-flash reads privately outside Git. Verify each uploaded
application by SWD readback and compare its preserved bootloader regions with
that board's own pre-upload backup. Record actual results below. Never reuse
Node1's hardware identity or backup as Node2/Node3's identity or backup.

Controller pre-upload identity: `0027001c4b33501220333734`.
Verified private backup:
`prototype-01/20261025T004504.627654Z/` under Development/Backups/marketsys.
Both full-flash copies match SHA256:
`4a52fdcebfd1c058e051d2c8d88b15772275c757f7877bc0f520524608216b48`.
The snapshot matches the prior verified 0.4.0 controller image.

Deployment is in progress. No Node2/Node3 runtime or radio result is claimed
until those boards are individually connected, backed up, flashed and tested.

## Prepared image hashes (SHA256)

| Target | SHA256 |
| --- | --- |
| Central | `0a109714106726e6ddea80b351abf82f8f9b8ebafc5baef48bb71cb8c110cce7` |
| Node1 | `9d6ceab6f454f0b6b9a8761ebf00fe18f4cdfdfa57e8daba3ce39480676263bc` |
| Node2 | `d5446d47cb32186c523207a300530a0837b6fa9135c2594c91f7edaec4bfb4a7` |
| Node3 | `69493ae4e3b5142c63ab4b37dfc0223f75b2b66010b7082a74902900d0d5677c` |
| Node50 (compile boundary check) | `0caf4585a4462c62b0b4e849dddcdad830c1dae828665b8e9271e03d31b5e5d0` |

All five targets compiled. Central uses 160792 bytes program storage and
34312 bytes static RAM; node builds use at most 160664 bytes program storage
and 33912 bytes static RAM, within the pinned board's reported allocations.

## Controller deployment completed

Official RAK UART uploader reported Upgrade Complete. The first readback
attempt failed on an SWD memory read at 0x08005fc4; no reflash or unlock was
performed. A new read-only attempt succeeded after the controller was running.
Private verification record:
`prototype-01/20261025T005539.584601Z-post-addressed-upload/verification.json`.
All 175928 application bytes match the prepared central image. Hardware UID
matches. Bootloader regions outside the reviewed 0x4004..0x400F update record
remain identical to this board's pre-upload snapshot. Settings-region bytes
changed; preservation of all settings is not claimed. Full snapshot SHA256:
`02f63d8f13a1411b2703c46bc4d25e43f654ae0c89c0ac9783b22f29d062b484`.
Runtime reports CENTRAL, READY=1, FW=0.4.2, MAX_NODES=50; test-injection flags
are zero. Logs: build/controller-042-upload.log, controller-042-status.log,
controller-042-flash-verification.log.

Live scripts/check_addressing.py rejected address 0, 51, 255 and -1; zero and
overflow tokens; and retired/obsolete STANDBY/NEXT names. Each invalid command
left the prior transaction unchanged. Address 50 OFF token 4213089096 was
accepted, transmitted exactly three times and ended FAILED with confirmation
UNKNOWN, as expected with no address-50 node on the bench. No node confirmation
was invented. Log: build/controller-042-addressing-check.log. This is a parser /
bounded-transport check, not an over-air success test with 50 physical nodes.

Node1 remains on its previously verified 0.4.0 until the programmer is moved.
Node2 and Node3 have prepared 0.4.2 images, but no prior identity, backup,
upload or radio verification has been recorded for them in this repository.
Next physical step: all power off, unplug DAPLink USB, disconnect Node1's
breadboard 3.3 V, move the six programmer connections to Node1, and power it
only from DAPLink. Keep the indicator breadboard supply off during programming.
Verify Node1 UID 003300455233500d20303938 and take a fresh two-read backup
before uploading its explicit address-1 image. Repeat individual identification
and backup for Node2 and Node3; do not connect two 3.3 V supply outputs together.

## Node1 pre-upload backup verified

After the user connected Node1, SWD confirmed UID
`003300455233500d20303938`. Two new 262144-byte reads match each other and
the prior verified 0.4.0 snapshot, SHA256:
`0848ade0c31d89d93ce3e9252a4e7542977c1637fd3c281b9ec17ba6d126b98d`.
Private backup: `proto-node1-v1/20261025T010225.630459Z/`.
Log: build/node1-pre-042-backup.log. Initial UART AT queries returned no
response; serial wiring TX->PA3 / RX->PA2 is being checked before upload.
Node1 has not been reflashed at this checkpoint.

Node1 UART follow-up: after the user's proceed response, both the normal
status helper and longer AT/AT+VER queries still received no UART bytes.
A debugger SYSRESETREQ reset and subsequent post-startup queries did not
restore communication. Brief halt/resume diagnostics found the CPU executing
with no CFSR fault bits set; no other process held /dev/ttyACM0. This does
not establish the cause of the UART silence. No bootloader-entry or upload
command executed because the status precondition failed. User must reseat
DAPLink TX->PA3 and RX->PA2 with USB power disconnected before retrying.
Logs: build/node1-042-uart-diagnostic.log, node1-042-debug-reset.log,
node1-042-uart-after-reset.log and node1-042-runtime-diagnostic.log.
