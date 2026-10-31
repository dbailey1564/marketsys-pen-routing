# Indicator revision 0.5.0 — prepared, not flashed

October 31, 2026. Controller and Node1/Node2/Node3 builds are prepared.
This revision adds the four agreed polling operations through the existing
bench AT interface. It does not freeze or implement the draft application
binary protocol. No board was connected, reset, backed up or flashed for this preparation.

## Available commands

| Command | Operation |
| --- | --- |
| `ATC+PEN=token:node:OFF` (or ACTIVE, LAST, HOLD_OPEN, FULL, FAULT) | Existing addressed state control; node 1–50 |
| `ATC+POLL=token:node` | Read live state over LoRa without applying a state |
| `ATC+POLL_LAST=node` | Read last confirmed state from controller RAM; no radio transmission |
| `ATC+POLL_ALL=token` | Sequential live queries of all 50 addresses, including currently absent nodes |
| `ATC+POLL_ALL_LAST=?` | All 50 cached records, including UNKNOWN entries; no radio transmission |
| `ATC+PEN=?` | Existing controller link/last transaction diagnostic |
| `ATC+DROP=0` or `1`, `ATC+DROP=?` | Existing bench ACK-loss diagnostic; applies to SET only |
| `ATC+NODE=?`, `ATC+NODE=OFF` | Existing direct node UART status / local OFF |

Tokens are nonzero decimal uint32 values; nodes are decimal 1–50. For an all-node
scan, each subsequent query increments the initial token and skips zero on wrap.
The controller permits one active radio transaction/scan; a new SET or live poll
while busy is rejected. Cached reports remain available. All-node polling checks
all 50 addresses because no installed-node roster has been ratified; an absent
node times out after three attempts and the scan continues. Allow up to eight
minutes in the host helper for a complete scan with missing nodes.

`MS_POLL_NODE` output identifies node, token, OK/TIMEOUT, reported state,
attempts and SOURCE=LIVE. TIMEOUT reports UNKNOWN for that query and preserves
the preceding cache. `MS_CACHE_NODE` identifies the last commanded state,
last confirmed state, KNOWN, elapsed milliseconds since confirmation and
SOURCE=CACHE. UNKNOWN/KNOWN=0 means no confirmation since controller startup;
AGE_MS=0 in that case is not evidence of a fresh report. Cached state is a
historical observation, not proof the node is currently powered or still lit.
The cache is volatile and is refreshed by matched SET ACKs or live STATUS replies.
A live response confirms the node's software state, not optical output or LED current.

## Node behavior and transport

The 14-byte version-1 LoRa envelope retains existing SET=1 and ACK=2. New internal
QUERY=3 and STATUS=4 carry the same node/token/CRC layout. QUERY requires OFF in
its otherwise unused state field and APPLIED in the result field; it never
applies OFF. STATUS returns the actual current indication and APPLIED. Node
state can change while the reply is queued; the response is refreshed before
transmission. A poll does not cancel the boot sequence, alter output pins,
restart the ten-second state timer, or replace the last SET deduplication token.
SET accepts ACK only; QUERY accepts STATUS only, matching both node and token.
These radio types are implementation allocations, not the undecided app message IDs.

Six states, PB3/PB4/PB5 assignments, 500 ms normal flashing, ten-second automatic
OFF, and the 0.4.3 boot test are unchanged. Nodes support compiled identities 1–50;
separate binaries are prepared for the three physical prototype nodes.
Old 0.4.x nodes ignore QUERY. Flash all requested targets before claiming live
poll support; existing SET/ACK formats remain unchanged.

## Deliberately excluded pending decisions

- Binary app frame integration, session versus persistent event identity,
  alpha encoding/valid range, and reserved destination numbers.
- App reply framing, acceptance versus application reporting, deduplication,
  reset recovery and resynchronization.
- Controller-owned HEAD_DESTINATION flow integration: the normal transition
  rules are recorded, but its event identity, failure handling and recovery
  contract remain unresolved. No placeholder assignment command is added.
- Automatic FULL/FAULT triggers, cancellation/correction and production state
  persistence. The ten-second cutoff remains a bench feature.
- Remote reboot, remote boot-test trigger and a permanent hold command.

See [flow decisions](pen-flow-logic.md), [interface draft](interface-protocol-specification.md),
and [engineer questions](engineer-questions.md). This release does not turn
an open proposal into an approved command.

## Validation and flash preparation

All seven C++ host test programs pass with `-std=c++11 -Wall -Wextra -Werror`.
Tests include the actual controller and node sketches with test-only hardware
stubs: live/cache separation, wrong node/token/type replies, three-attempt
timeouts, continuation across missing nodes, a full 50-address scan and token
wrap; preservation of boot, timer and SET deduplication; and query replies
straddling automatic OFF. Existing codec, pattern, addressing and boot regressions pass.
`radio_link.py` also passes Python syntax compilation.

Pinned Arduino CLI 1.5.1 / RUI3 4.2.4 builds pass for controller and nodes 1–3.
Controller: 161984 bytes program / 34728 bytes static RAM. Nodes 1 and 3:
160952 / 33920 bytes; Node2: 160944 / 33920 bytes. These build reports do not
establish runtime stack headroom or hardware operation.

Frozen application images are on Lin under `build/releases/0.5.0/`.
[Release manifest](firmware-release-050.json) records exact checksums, sizes,
expected UIDs and source hashes. Binaries remain in ignored build storage;
source, tests, preparation script and manifest are versioned on GitHub.

Flash order: controller, Node1, Node2, Node3 when the user reconnects each board.
For each target, verify its expected UID, take a new two-read flash backup using
`scripts/backup_flash.py`, check the frozen image checksum, upload the application,
then run `scripts/verify_addressed_upload.py` against that fresh backup and image.
Verify UART role/version/identity/radio readiness after restart. Keep private
flash backups outside Git. No automatic flashing is performed by the release script.

After the nodes are reconnected and powered: verify each live POLL, compare
POLL_LAST, check an unreachable address and all-node scan completion, then run
the six-state optical test per node. Verify that live polling during an indication
does not extend its ten-second cutoff. Hardware and optical validation are pending.
