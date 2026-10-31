# Indicator revision 0.6.0 — persistent indications and controller recovery

October 31, 2026. Implemented for the user's requested next upload. Prepared
and host-tested; hardware deployment and verification are recorded separately below.
The unflashed 0.5.0 polling package is superseded for this upload.

## Implemented

- All six indications persist until changed. No automatic ten-second OFF in
  radio-node firmware. Normal flashing remains 500 ms on/off. Standalone older
  bench sketches retain their own bounded tests; they are not this release.
- Full startup sequence retained: initial five-second window plus setup,
  then five two-second indications with 250 ms flashing, then OFF. Radio SETs
  during the startup test are ignored so recovery cannot cut it short. Direct
  UART OFF remains a local maintenance override. Live queries identify BOOTING.
- Controller saves desired state per node and active/last role fields before
  accepting a diagnostic state change. Save failure rejects the change.
- Controller startup and periodic reconciliation query configured nodes, wait
  through their startup tests, and reapply saved intent when different. A
  restoration is a state command, not a new head-entry event.
- Explicit new-auction CLEAR saves all desired states OFF, clears active/last
  roles and old fault latches, advances a local auction generation, and marks
  clearing pending before radio traffic. Configuration is retained. An interrupted
  clear resumes from saved intent on controller restart.
- Each configured node gets bounded OFF delivery attempts. If unconfirmed,
  controller makes one bounded reboot transaction (up to three transmissions),
  waits twenty seconds for startup, polls readiness, and confirms/restores OFF.
  Reboot acknowledgment means queued, not evidence of completed reboot or OFF.
  If unresolved, controller records a durable per-node fault and reports the
  node unconfirmed. There is no endless reboot loop.
- Later successful reconciliation does not silently erase a logged fault.
  Fault latches are cleared by a new-auction clear. No unratified explicit
  fault-acknowledgment behavior or automatic red display is inferred.
- All four polling controls retained. Boot-test colors are not recorded as
  confirmed operational states. Cached replies include controller fault flags.

## Commands through the existing bench AT interface

| Command | Behavior |
| --- | --- |
| `ATC+PEN=token:node:STATE` | Durable desired state then SET; STATE is OFF/ACTIVE/LAST/HOLD_OPEN/FULL/FAULT |
| `ATC+PEN=?` | Last radio transaction, storage status, auction generation, recovery status and role fields |
| `ATC+POLL=token:node` | Live read-only query; OK, BOOTING or TIMEOUT |
| `ATC+POLL_LAST=node` | Cached confirmation, saved desired state, age and controller fault |
| `ATC+POLL_ALL=token` | Live scan of all 50 supported addresses |
| `ATC+POLL_ALL_LAST=?` | Cached records for all 50 addresses |
| `ATC+AUCTION=CLEAR` | Durable new-auction clear with bounded OFF/reboot recovery |
| `ATC+AUCTION=?` | Controller status; saved clearing is not proof every node is OFF |
| `ATC+FAULTS=?` | Configured nodes' latched controller faults and desired states |
| `ATC+ROSTER=?` | Configured nodes |
| `ATC+ROSTER=node:1` / `node:0` | Explicitly add/remove a node from automatic reconciliation/new-auction work |
| `ATC+DROP=0/1`, `ATC+DROP=?` | Preserved bench SET ACK-loss test |
| `ATC+NODE=?`, `ATC+NODE=OFF` | Node direct-UART diagnostics and local OFF |

Initial blank controller storage configures the three actual prototypes,
addresses 1, 2 and 3, with desired OFF. Addresses 1–50 remain supported.
Manual PEN to a valid address adds it to the saved roster. Removing a node
from the roster is an explicit configuration operation, not automatic exclusion
of an unreachable node. It does not turn that node OFF or erase its intent.
All-node polls still check 1–50 regardless of roster.

Automatic reconciliation runs at startup and about ten seconds after each
completed reconciliation sweep. Only one radio transaction/operation is active;
new live controls are rejected as busy while recovery/clear owns it. Read-only
cached/status queries remain available. The host helper retries busy commands
for up to ninety seconds. A new-auction sweep can take longer with many missing
nodes; the dedicated helper waits for its result. A sweep reporting faults is
not reported as a completely confirmed clear.

## Persistence and wire details

The installed pinned STM32WL RUI3 implementation exposes user flash starting
at 0x08037800 and uses 2048-byte erase pages. Two alternating record slots use
user-data offsets 0 and 2048 (absolute 0x08037800 and 0x08038000). Each 176-byte
explicitly serialized record has format magic, sequence, local auction generation,
role fields, fifty-node desired/configured/fault arrays, pending-clear flag and
CRC. Save writes the alternate page and reads it back before succeeding. Load
chooses the newest valid copy; an interrupted alternate write leaves the prior
valid copy available. Nonempty unrecognized data with no valid copy blocks
state-changing controls instead of being silently erased. This is a bounded
prototype record, not a historical audit database or durable application event queue.

New radio types REBOOT=5 and REBOOT_ACK=6 complement SET=1/ACK=2/QUERY=3/STATUS=4
in the existing 14-byte envelope. STATUS can report BOOTING=4 in its result field.
A query remains observational, preserving SET deduplication and output timing.
A reboot request schedules reset after a one-second ACK opportunity; duplicates
before reset do not postpone it. All image roles report version 0.6.0.

## Deliberately deferred

Application event identity, binary HEAD_DESTINATION integration, application
acceptance/completion replies, durable event replay/outcome storage, and a boundary
rejecting old application events across auctions remain deferred. No proposed
session/event field is silently frozen. The local auction counter is not the
application's event identifier. Automatic head-entry flow integration remains
outside this release until event input/replay semantics are finalized; existing
state commands do not themselves advance ACTIVE/LAST for a new head.

FULL can be commanded and retained, but its automatic controller trigger is
not added: the user assigned FULL authority to the penning application. Its
binary command, clearing/override rules remain undecided. Likewise no unratified
correction, sleep/session-close, radio authentication or production deployment
policy is introduced. Node software confirmation does not prove physical light.

The controller's saved role fields currently reflect explicit diagnostic ACTIVE,
HOLD_OPEN and LAST commands; they are not a claim of implemented business flow.
The standalone boot cancellation test scripts for 0.4.3 describe that historical
revision and must not be used to claim radio SET cancellation on 0.6.0.

## Validation

Nine C++ host programs pass using C++11, Wall/Wextra/Werror. New tests execute
actual controller/node sketches with simulated flash/radio and cover:

- Full boot retention, BOOTING status, persistent light beyond ten seconds and
  one day, millis rollover, duplicate SET and delayed bounded reboot.
- Controller desired-state recovery, readiness wait, clearing saved before RF,
  unanswered OFF/reboot/query, latched durable node fault and failed-save rejection.
- Two-page load/save, torn alternate write, pending-clear and fault retention,
  refusal of unknown data, and every single-bit record corruption.
- Existing state codec, all fifty addresses, polling, boot patterns and CRC tests.

All four pinned RUI3 builds pass. Controller uses 165032 bytes program / 34928
bytes static RAM; Node1/3 use 161176 / 33928; Node2 uses 161168 / 33928. These are
compile/static reports, not peak-runtime memory or hardware acceptance evidence.
Python host helper syntax compilation passes. Hardware flash/readback, real
power-cycle storage recovery, boot-aware LoRa restoration and visual checks
remain required before claiming on-device validation.

## Flash package and plan

Frozen images on Lin: `build/releases/0.6.0/`. Exact sizes, SHA256 values,
expected UIDs and source hashes: [firmware-release-060.json](firmware-release-060.json).
Source and manifest are committed; private backups and generated images remain
outside Git. Flash order is controller, Node1, Node2, Node3, with a fresh two-read
backup, UID check, checksum check and full readback verification for each.
Expect mixed old/new nodes to lack polling/reboot support until all are updated.
For subsequent uploads, preserve and verify the controller routing record in
addition to application/bootloader checks; the initial update starts from unused
user-storage only after verifying its contents.

## Controller deployed and readback-verified — October 31

Controller UID 0027001c4b33501220333734 was verified before upload. Fresh
two matching 262144-byte private reads at prototype-01/20261031T212444.365038Z
produced backup SHA256 02f63d8f13a1411b2703c46bc4d25e43f654ae0c89c0ac9783b22f29d062b484.
Packaged reference bootloader code and activation marker matched; both new
routing storage pages were blank. Frozen 181312-byte controller image
SHA256 dd8210f5df4bbd341ec2dd913be577cdb0ee644b965ea9f5b3185dfb46d5b310
was uploaded with the official RAK UART uploader. After restart, UART reports
CENTRAL READY=1, FW=0.6.0, STORAGE=OK and automatic recovery active.

Private readback verification record:
prototype-01/20261031T212742.507941Z-post-addressed-upload/verification.json.
UID matches, all application bytes match, and bootloader outside the reviewed
update record is unchanged. Settings region changed as expected; settings
preservation is not claimed. Readback SHA256:
a5fafb7abadf1b165b333054357ffc1f7197df565046da53241e27a90f5377b1.
Controller fault-query interface responds. No new-node optical, reboot-recovery
or power-cycle recovery test is claimed yet. Nodes1/2/3 remain on 0.4.3 pending
connection and individual backup/upload/readback; mixed old nodes do not support
new polling/reboot types. Logs are in ignored build/controller-060-*.log.

## Node1 deployed and readback-verified — October 31

Node1 UID 003300455233500d20303938 matched. Fresh two-read private backup:
proto-node1-v1/20261031T213120.554329Z, SHA256 874af3204735a5d6a0acc73cfdc580905e8769793e2d119c22bd5ef6a2165d75.
Reference bootloader/activation marker and frozen image/source checksums passed.
Official RAK UART uploader installed the 176256-byte 0.6.0 image
SHA256 a71795bf57fa44348beeb3a884111168cb4b386962f7c2863ed7fabc95ddf1af. A subsequent UART restart observed all five boot steps
in order and final OFF; runtime reports NODE1, READY=1, FW=0.6.0, address 1,
LIMIT_MS=0, normal phase 500 ms, boot test complete and all three GPIO LOW.
These are software/UART observations, not a user-confirmed optical pass.

Private full readback verification: proto-node1-v1/20261031T213514.270218Z-post-addressed-upload/verification.json.
Hardware identity, exact application bytes and preserved bootloader outside the
reviewed update-record range pass. Settings region changed; full settings
preservation is not claimed. Snapshot SHA256 2a8bf0962e52b6e3b3d2e3ee33b82f66f70875e53ce50ad2052c0e400345dcc8.
Post-upload LoRa, persistent indication and controller-restoration checks remain
pending until the programmer returns to the controller and nodes are powered.
Logs: build/node1-pre-060-*.log and build/node1-060-*.log.

## Node2 deployed and readback-verified — October 31

Node2 UID 003300265233500d20303938 matched. Fresh two-read private backup:
proto-node2-v1/20261031T213717.342371Z, SHA256 2b6eb74973e5a480088ac6497104e1b43a7f927d36c9242ffd3706d34d92fd61.
Reference bootloader/activation marker and frozen image/source checksums passed.
Official RAK UART uploader installed the 176248-byte 0.6.0 image
SHA256 921523de28a90229121b02c8cdcd67c1a4dc33628e388add16e8be456ddf2255. A subsequent UART restart observed all five boot steps
in order and final OFF; runtime reports NODE2, READY=1, FW=0.6.0, address 2,
LIMIT_MS=0, normal phase 500 ms, boot test complete and all three GPIO LOW.
These are software/UART observations, not a user-confirmed optical pass.

Private full readback verification: proto-node2-v1/20261031T213939.814880Z-post-addressed-upload/verification.json.
Hardware identity, exact application bytes and preserved bootloader outside the
reviewed update-record range pass. Settings region changed; full settings
preservation is not claimed. Snapshot SHA256 b353c5f68cadb50332e5513531ab3bb486ce8fc0cfe82fdf100fca273404c93d.
Post-upload LoRa, persistent indication and controller-restoration checks remain
pending until the programmer returns to the controller and nodes are powered.
Logs: build/node2-pre-060-*.log and build/node2-060-*.log.
