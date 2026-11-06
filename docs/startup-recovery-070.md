# Startup assessment — firmware 0.7.0

Prepared November 6, 2026. Not uploaded or hardware-validated.

## Decision approved by Daniel

Before restoring saved indications, poll the configured nodes without changing
them. Bias an ambiguous whole-system restart toward a new auction. Battery-backed
nodes that continued operating provide evidence for controller-only recovery.
A deliberate system startup and simultaneous failure cannot be distinguished
from these reports alone; the new-auction bias is a policy, not proof of intent.

## Implemented sequence

The controller loads its existing MSR1/MSR2 two-page routing journal without a
format migration. After its five-second initialization delay it allows another
16 seconds before the first startup query. It polls configured addresses only,
with the existing three-attempt transaction bound. BOOTING replies are retried
at one-second intervals, bounded by 60 seconds from controller initialization.
No SET, REBOOT, destination advancement, roster edit, or flow retry can run
through this startup barrier. Status queries remain available after a blocked
assessment. Normal node startup lights still run in full.

Each node replies with current state, last durably recorded SET state/token,
auction number, monotonically increasing persisted boot number, uptime, storage
health, and whether it has applied a SET during this boot. The last-command
history survives reboot; the applied-this-boot flag does not. During startup,
current state describes the test, not a restored historical indication.

| Evidence after collection | Action |
|---|---|
| At least one node applied a command this boot, with matching auction, historical state, and current desired state; no conflicting reports | Resume saved controller routing and reconcile restarted/missing nodes. |
| At least one reply, but every responding node is fresh this boot and currently OFF | Save a new auction and OFF intent before clearing configured nodes. |
| Surviving node reports a different auction/state, boot does not finish within the window, or node history storage is unhealthy | Save controller faults, block automatic writes, retain routing record. |
| No configured node replies | Save controller faults, retain routing record, block automatic writes. |

One surviving node is sufficient evidence to recover, but collection continues
through the roster so another conflicting survivor cannot be silently ignored.
Missing nodes do not veto evidence from responders; reconciliation/clear marks
them unconfirmed using the existing bounded retry/reboot/fault procedure.

A previously saved pending new-auction clear resumes the SAME auction after
responses arrive, even if indicators still reflect the interrupted old state.
It remains blocked on no replies or unhealthy node storage. An explicit
ATC+AUCTION=CLEAR can resolve a blocked assessment after investigation. The new
number exceeds both the controller number and every observed node auction;
UINT32_MAX exhaustion blocks rather than wrapping to an old auction.

The last-command token is reported for diagnosis and same-auction duplicate
detection. Startup compares auction and desired/current/last-command states;
the controller does not persist a per-node last-token table. Boot number and
uptime are diagnostic evidence; APPLIED_THIS_BOOT distinguishes a live auction
participant from a restarted node without relying on clock synchronization.

## Node journal

Each node uses two separate 2-KiB RUI3 user-data erase pages at offsets 0 and
2048. A 32-byte MSN1 record includes CRC, sequence, node address, boot number,
auction, last token, last state and history-valid flag. Each boot increments
and verifies its boot number. Each newly accepted SET journals and verifies the
command before applying it and returning success. A torn write leaves the prior
valid page available. Corrupt/unknown data and a different node identity are
never silently erased. Failure is reported as STORAGE_ERROR and blocks SETs.

The journal records durable command intent; a power cut between saving and GPIO
application may prevent that indication from appearing. Following reboot, the
node is marked fresh and never claims that historical indication is currently
applied. Polls and exact retransmissions do not write flash. New SETs and boots
do write flash; endurance/wear-leveling remains a production design requirement.
This release is not an unlimited-endurance storage implementation.

Older-auction SET/REBOOT packets are ignored. This remains an unauthenticated
bench radio protocol, not a security boundary or complete replay window.

## Radio wire version 2 (not the penning-application interface)

All fields are explicitly serialized; multi-byte fields are little-endian.

| Bytes | Meaning |
|---|---|
| 0–1 | ASCII MS |
| 2 | Radio version 2 |
| 3 | SET=1, ACK=2, QUERY=3, STATUS=4, REBOOT=5, REBOOT_ACK=6 |
| 4 | Node 1–50 |
| 5 | Current/requested indication; QUERY/REBOOT/REBOOT_ACK use OFF |
| 6 | APPLIED=0, DUPLICATE=1, EXPIRED=2, CONFLICT=3, BOOTING=4, STORAGE_ERROR=5 |
| 7 | Zero |
| 8–11 | Nonzero request/reply transaction token |
| 12–15 | Auction number (SET intent, or node history in replies) |
| 16–19 | Node's last saved SET token |
| 20–23 | Node boot number |
| 24–27 | Node uptime in milliseconds, wrapping at 32 bits |
| 28 | Last saved SET state |
| 29 | Flags: history valid=1, applied this boot=2, history storage healthy=4 |
| 30–31 | Zero |
| 32–33 | CRC-16/CCITT-FALSE over bytes 0–31; low byte first |

Old 14-byte/version-1 radio packets are rejected. Controller and all nodes need
0.7.0; a mixed-version system cannot confirm all configured nodes. This does
not implement application event identification or two-stage application replies.

## Verification and deployment

Fourteen host-test executables pass with C++11, including actual controller/node
handlers, read-only startup collection, new-auction bias, survivor recovery,
conflict/no-response blocking, interrupted clear, history across reset, torn
writes, failed-save output blocking, duplicate behavior, stale epochs and CRC.
Controller and Node1/2/3 images build with pinned RUI3 4.2.4.

Frozen images and source hashes: firmware-release-070.json. No live commands,
resets or uploads were issued to validate this feature. Existing installed
versions remain unchanged. Physical startup and power-cut tests are pending.

Deployment: take matching full backups and verify each board identity/user-data
pages first. Program and read back the addressed nodes, then the controller;
do not operate the mixed-version system. Keep existing backups and bootloaders.
Test whole-system startup, controller-only restart, one-node restart with the
controller running, conflicting history, absent nodes, and interrupted clear.
Confirm the complete startup light test and each final indication visually.

Implementation: firmware/radio_central/radio_central.ino,
firmware/radio_node/radio_node.ino, libraries/MarketSysBench/src/NodeHistory.h,
BenchProtocol.h and BenchAddress.h. Host tests are in tests/startup_*_test.cpp.
