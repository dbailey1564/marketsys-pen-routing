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

## Node3 deployed and readback-verified — October 31

Node3 UID 003400345233500d20303938 matched. Fresh two-read private backup:
proto-node3-v1/20261031T214225.659088Z, SHA256 419d83d437d8b7cf1396bcb7b31115d6bf350a9f594a15575f8d2cca9b2ac86e.
Reference bootloader/activation marker and frozen image/source checksums passed.
Official RAK UART uploader installed the 176256-byte 0.6.0 image
SHA256 2e26a13fde9660464c3b31ee6f2978b086d802bcbd1e3c0486cb5b2fe082e79d. A subsequent UART restart observed all five boot steps
in order and final OFF; runtime reports NODE3, READY=1, FW=0.6.0, address 3,
LIMIT_MS=0, normal phase 500 ms, boot test complete and all three GPIO LOW.
These are software/UART observations, not a user-confirmed optical pass.

Private full readback verification: proto-node3-v1/20261031T214447.404206Z-post-addressed-upload/verification.json.
Hardware identity, exact application bytes and preserved bootloader outside the
reviewed update-record range pass. Settings region changed; full settings
preservation is not claimed. Snapshot SHA256 8230a75869faa16cc4af6889f190d03baf96825e933dfaed9182a5b458698097.
Post-upload LoRa, persistent indication and controller-restoration checks remain
pending until the programmer returns to the controller and nodes are powered.
Logs: build/node3-pre-060-*.log and build/node3-060-*.log.

## Post-upload live polling — October 31

Programmer returned to the controller. UART reports CENTRAL READY=1, FW=0.6.0
and STORAGE=OK. All three updated nodes responded to addressed read-only POLL
on attempt 1 with reported state OFF. No SET/OFF command was used for these
reachability checks. Transactions: Node1 token 4032471649, Node2 token 4116276667, Node3 token 894501700.
Logs: build/node1-060-first-live-poll.log through node3. This establishes live
query/response and reported state for all three addresses, not physical LED
output, persistent-light duration or restart-restoration validation. Those
hardware checks remain pending.

## Node1 post-upload indication and persistence radio test — October 31

Node1 acknowledged ACTIVE, LAST, HOLD_OPEN, FULL and FAULT in sequence. Each
was held for two seconds after its command helper completed. A further ACTIVE
command was followed by sixteen seconds without refreshing SET; a live read-only
POLL then reported ACTIVE. Explicit OFF was acknowledged and another live POLL
confirmed OFF. This verifies reported state beyond the former ten-second cutoff
and final OFF, not physical illumination or restart restoration. User optical
confirmation and hardware restart/restoration tests remain pending. Logs:
build/node1-060-bench.log; machine result: build/node1-060-bench-result.json.
The test altered no firmware or wiring and left Node1's saved desired state OFF.

## Node1 optical confirmation and Node2 radio/persistence test — October 31

User reported all Node1 tests good after the five-indication/persistence run,
confirming physical colors/flashing and the tested light behavior. This does
not establish a restart-restoration test, which has not yet been performed.

At the user's request Node2 then acknowledged ACTIVE, LAST, HOLD_OPEN, FULL
and FAULT, two seconds each after command completion. A fresh ACTIVE command
was followed by sixteen seconds without refreshing SET; live POLL reported
ACTIVE. Explicit OFF and a further live POLL confirmed OFF. Node2's radio and
reported persistence checks pass; user optical confirmation is pending. Logs:
build/node2-060-bench.log and node2-060-bench-result.json. No firmware or wiring
changed. Both tested nodes have saved desired OFF at the end.

## Node2 optical pass and Node3 radio/persistence test — October 31

User confirmed Node2 passed the observed indication/persistence test. Node1
and Node2 now have both radio and user-observed visual passes for this run.

Node3 then acknowledged ACTIVE, LAST, HOLD_OPEN, FULL and FAULT. A further
ACTIVE remained reported by live POLL after sixteen seconds without refresh.
Explicit OFF and a live POLL confirmed final OFF. Node3 visual confirmation
is pending. Logs: build/node3-060-bench.log and node3-060-bench-result.json.
No restart-restoration test is established by these runs. All three nodes
have saved desired OFF after their tests. No firmware or wiring changed.

## All three post-upload visual passes confirmed — October 31

User confirmed Node3 visual result passed. Nodes1/2/3 have now passed the
observed five-indication sequences and persistence checks, with live reported
ACTIVE after sixteen seconds without SET refresh and explicit final OFF
confirmed by live poll. All three have saved desired OFF. This does not
establish controller/node restart restoration, new-auction recovery/reboot,
missing-node fault handling, cached/all-node polling or physical power-loss
acceptance. Those hardware tests remain separate and pending. No firmware or
wiring changed for this confirmation.

## Node1 power-cycle restoration confirmed — October 31

Node1 was commanded ACTIVE and live-polled ACTIVE while the controller saved
that intent. The user was asked to power-cycle only Node1 with the controller
remaining powered, then confirmed return to green. A subsequent read-only live
POLL reported ACTIVE on attempt 1. This establishes the observed Node1 return
to saved green after the manual restart; the user's reply did not separately
confirm every startup-test step during that cycle. No standalone remote reboot
was commanded. Controller remote reboot support is currently invoked only by
new-auction failed-OFF recovery.

Cleanup explicitly commanded OFF and a fresh live POLL confirmed OFF on attempt
1; the controller's active role returned to zero. Logs: node1-060-recovery-before,
node1-060-recovery-after and node1-060-recovery-cleanup under ignored build/.
Controller power-loss retention and new-auction failure/reboot/fault handling
still need hardware acceptance tests. No firmware or wiring changed.

## Node2 power-cycle restoration confirmed — October 31

Node2 was explicitly commanded ACTIVE and live-polled ACTIVE; user confirmed
green visible. After being asked to manually power-cycle only Node2 while the
controller stayed powered, user confirmed solid green returned. A fresh live
POLL confirmed ACTIVE. Cleanup OFF was acknowledged and another live POLL
confirmed OFF. This establishes Node2 restoration to saved green, without
separately claiming the user observed every startup step during this cycle.
No remote reboot was sent. Logs: build/node2-060-recovery-before.log,
node2-060-recovery-after.log and node2-060-recovery-cleanup.log. Node2 saved
desired state is now OFF. No firmware or wiring changed. Controller power-loss
and new-auction recovery/fault tests remain pending.

## New-auction clear and controller-restart check — October 31

Seed commands were acknowledged: Node1 ACTIVE, Node2 LAST, Node3 FULL.
ATC+AUCTION=CLEAR durably advanced the local auction generation to 1 and
completed with all three nodes confirmed OFF and zero faults. Controller
then underwent an authorized-test software restart via UART ATZ. After
restart it reported STORAGE=OK, AUCTION=1, ACTIVE=0, LAST=0 and recovery idle.
Fresh live polls of Nodes1/2/3 each reported OFF on attempt 1; controller fault
query showed all three desired OFF with no faults. Thus old seeded indications
were not restored across this software restart. This is not a physical
controller power-cut test, interrupted-clear test or failed-node reboot test.
Logs: build/auction-060-seed.log, auction-060-clear.log,
auction-060-controller-restart.log and auction-060-after-restart.log.
All nodes remain intended/confirmed OFF. No firmware or wiring changed.

## Missing Node3 new-auction recovery and durable fault — October 31

User intentionally switched Node3 off. New-auction clear durably advanced to
local auction generation 2. Nodes1/2 confirmed OFF; Node3 exhausted bounded
OFF delivery, the bounded remote reboot request had no acknowledgment, and
post-wait live query remained unanswered. Controller emitted Node3 UNCONFIRMED
and a per-node fault, with one fault in the completed sweep. The host helper's
nonzero exit is the intended report of an incomplete confirmed clear, not a
claim that Node3 rebooted or turned OFF. The powered-off node cannot receive
the reboot; this tests the negative/unreachable branch, not successful remote
reboot execution.

Fault query showed only Node3 faulted, desired OFF for all three. A controller
software restart then retained AUCTION=2 and the Node3 fault; Nodes1/2 still had
no faults. This verifies retained fault across reset, not a physical controller
power-cut test. Node3 remains physically off until the user restores its power.
Logs: build/auction-060-missing-node3.log, auction-060-missing-node3-faults.log,
auction-060-fault-restart.log and auction-060-fault-after-restart.log. No firmware
or wiring changed. Fault remains latched pending a later explicit new-auction
clear; communication recovery alone must not be represented as fault clearing.
