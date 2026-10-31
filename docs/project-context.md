# MarketSys prototype context

This is a project-decision record carried forward from Daniel's referenced
**Prototyping MS** conversation and the continuation in Codex on 2026-10-20.
It is not a manufacturer specification or a newly verified bench report.

## Standing version-control workflow

User instruction, October 24, 2026: all future iterations saved to the Lin
development repository are to be committed and pushed to GitHub without
requesting push approval again. Include source, tooling, documentation and
iteration/test records. Verify the remote branch matches the local commit
after pushing. If authentication or another error prevents a push, report
the unsynced status explicitly and resolve it before claiming synchronization.
Private flash backups and credential-bearing files remain outside Git.

## Agreed architecture

- Prototype hardware plan: one central controller and three pen nodes. Earlier
  discussions called the hardware RAK3272S/RAK3172. The photo supplied on
  2026-10-20 identifies the pictured module as **RAK3172-SiP** and breakout as
  **RAK3272-SiP Ver B**. The build target has been corrected accordingly.
  Confirm other boards individually if their markings differ. A RAKDAP1 debugger
  is available and enumerates on Lin as DAPLink/CMSIS-DAP, with UART at ttyACM0.
- Raw LoRa P2P star topology in the US915 operating context. Exact P2P frequency,
  bandwidth, spreading factor, coding rate, output power, and operating rules
  still need an explicit project configuration; a LoRaWAN region label alone
  does not supply those P2P settings.
- The central controller owns radio transport, acknowledgement, and retry logic.
  Nodes apply requested indicator states and report their state.
- The penning application supplies the next ACTIVE pen assignment. The central
  controller owns state advancement and derives the indicator commands,
  including HOLD_OPEN for a new assignment to the same active pen. The app
  should not have to issue separate ACTIVE and LAST indicator commands.
  A different pen assignment activates immediately and moves the previous
  ACTIVE pen to LAST. LAST therefore denotes the previous active pen, not
  a future queued pen. On the next different-pen advance, the older LAST pen
  becomes OFF. Same-pen assignments produce HOLD_OPEN without advancing the
  two positions. See pen-flow-logic.md. Cattle, buyer, and routing metadata
  remain outside the node protocol.
- The host interface is intended to be binary and host-agnostic over a USB
  virtual COM connection. The physical serial/USB arrangement still needs to be
  documented; this build setup does not establish it.
- The user confirmed we define the penning-application interface for the
  engineer to implement. Access to his application source or its existing
  binary communications format is not a prerequisite. See host-interface-record.md.

## Indicator states

| State | Intended indication |
|---|---|
| OFF | Off |
| ACTIVE | Solid green |
| LAST | Solid blue |
| HOLD_OPEN | Flashing green |
| FULL | Solid red |
| FAULT | Flashing red |

Naming update: LAST replaces NEXT, with solid blue and wire value 0x02
unchanged. Source and helper commands are updated in prepared 0.4.1; both
images compile and tests pass. Physical boards still run verified 0.4.0 and
retain the previous NEXT text until deployment. Historical test records keep
their original names. See pen-flow-logic.md for exact build hashes.

Flash periods and transition timing are not fixed by this table.
STANDBY was removed by the user's October 24 revision. Its wire value 0x03
is retired; surviving state IDs are unchanged. Radio firmware 0.4.0 implementing
this table is compiled and host-tested. Node1's previous iteration was freshly
backed up twice and verified before upload; Node1 now runs 0.4.0 with matching
flash readback and OFF GPIO status. DAPLink has been returned to central,
which also runs 0.4.0 with verified flash readback; Node1 is on breadboard
power. All six revised states and their fresh OFF commands received APPLIED
ACKs on the first attempt, and the user visually confirmed each revised
pattern and darkness afterward. STANDBY was rejected by the controller
without starting a transaction. Final confirmed state is OFF, token 357773534;
test-injection flags are zero. The requested revision and verification are complete.
See indicator-revision-040.md for deployment progress and exact build hashes.

## Hardware work carried forward

- 3537 RGB LED: preliminary common-driver working point around 160 mA, pending
  visibility testing; this is not a final current selection.
- Reported measurements: green 2.63 V at 152 mA; red 2.02 V at 162 mA; good blue
  sample 2.80 V at 163 mA. An earlier dim blue sample at 1.80 V is suspect.
- LD1500SB constant-current module, one driver per node, is the current plan.
- FDC6401N dual MOSFET is the bench candidate: two packages per node provide
  three color switches and one unused MOSFET.
- Power and recharge remain unresolved and precede full-node LoRa system tests.

Power requirements clarified by the user after first bring-up: solar remains
connected for recharge; target one eight-hour session per week (auction-house
confirmation pending); support inter-session sleep; allow at most five minutes
for central wake activation. The proposed initial sleep/RX cadence is five
seconds of usable RX every two minutes, pending hardware validation. This is a
power/session mode separate from indicator STANDBY (solid blue). See
power-planning.md and standby-wake-design.md. Battery, charger, solar panel, and
regulator hardware are still unselected.

Charging-source clarification: panels are intended to harvest the penning barn's
house lighting. Do not assume outdoor sunlight or outdoor-rated panel output.
Lighting schedule and panel area are pending; see indoor-light-charging.md.

October 21 deadline decision: for the October 22 presentation, prioritize a
USB-rechargeable prototype and defer indoor harvesting. The proposed build uses
an enclosed 10,000 mAh USB power bank, 5 V LED-driver rail and separate regulated
3.3 V logic rail. See thursday-usb-prototype.md for selection, wiring, runtime
assumptions and remaining hardware checks. This is not yet an assembled system.

## Host protocol work carried forward

- Distinguish commanded state from last node-confirmed state.
- Planned polling commands: POLL, POLL_LAST, POLL_ALL, POLL_ALL_LAST.
- Header/framing, field widths and CRC remain open for our specification;
  they no longer depend on inspecting the penning application's existing format.
  Cross-thread recovery found an earlier proposed AA55/version/type/length/
  transaction/count/state-records/CRC host frame (17-byte worked example).
  It is preserved in host-interface-record.md, with its proposal status and
  distinction from the new assignment-driven payload. Do not treat absence
  from an earlier repository summary as proof no packet proposal existed.
- Radio packet format, addressing, duplicate handling, timeouts, retries, and
  failure/restart behavior require implementation-level specifications.

## Current implementation boundary (October 24)

The full onsite demonstration was subsequently cancelled in favor of an
application-engineer visit. The earlier USB power-bank plan is historical.
The current bench uses the DAPLink as Node1's sole 3.3 V supply and the
HW-131 breadboard supply's 5 V rail for the LED driver, with common supply
ground. The breadboard 3.3 V rail is unused. Production power remains open.

RUI3 is the development path. The repository contains the serial build check,
finite timer/sleep harness, and Node1 indicator bench harness. The controller
and Node1 have separate flash backup directories. The one DAPLink has been
moved from the controller to PROTO - Node1 V1; do not assume both are powered
or accessible simultaneously.

Node1's physical blue/red/green ON tests passed, including automatic OFF
and darkness of the unselected dies. A green source wiring error was corrected
from IC2 P2 to P5. Firmware 0.2.0 implements all seven indicator states with
provisional 500 ms phases, OFF at startup, a 10-second bench timeout, and
single-color switching. Host pattern tests and on-device GPIO pattern checks
passed. The user then visually confirmed all seven states, with expected
colors/patterns, unselected dies dark, and automatic OFF after each test.
Final state was OFF. See node1-indicator-test.md for exact behavior and verification.

Earlier radio milestones used Node1 0.3.0 and controller 0.3.1 with
a default-disabled one-shot lost-ACK test control. Both are now on 0.4.0
as recorded above; the earlier test results below remain historical. The DAPLink is
back on the controller; Node1 is powered from breadboard 3.3 V, with its driver
on 5 V. Both uploads have matching flash readbacks and preserved bootloader
code. The prototype transport has explicit frames/CRC, acknowledgements and
bounded retries. After the first mode-change restart, wireless OFF received
an APPLIED ACK on the first attempt. Duplicate OFF and conflicting-token
rejection and expired-duplicate handling also passed over air. All seven
radio-commanded states now have matching ACKs and user visual confirmation;
each command and fresh OFF received an APPLIED ACK on the first attempt.
An unpowered-node test then produced exactly three completed transmissions,
no ACK, and FAILED while retaining the prior confirmed state. After power
was restored, fresh ACTIVE and OFF commands each succeeded on attempt 1,
with user confirmation of flashing green then darkness. No controller restart
or firmware change was needed for that power-recovery test.
The subsequent controlled lost-ACK test ignored the first received APPLIED
ACK, retained the prior confirmed state while pending, and succeeded on
attempt 2 with a DUPLICATE ACK. An expired-duplicate probe by 10.623 seconds
verified that the retry did not extend Node1's original 10-second cutoff.
Test injection is disabled and final confirmed state is OFF (token 1479022242).
The user also visually confirmed this injected-loss run: approximately
10 seconds of green flashing, then complete darkness, other dies dark.
See radio-bench-link.md for backups, SDK restart requirement and limitations.

The UART AT commands are bench controls, not the planned binary host protocol.
Host integration and production power management remain unimplemented.
Official P2P examples remain separate references.

Next: settle controller-owned pen flow (pen-flow-logic.md), then define our
host interface for the application engineer and implement it.
Resolve production power/recharge and measured OFF/sleep
current separately. Existing application source/framing is not required.

## Current host CRC documentation revision

The consolidated review draft is docs/interface-protocol-specification.md.
The current outgoing HEAD_DESTINATION example is fixed at 16 bytes, removes
payload length, and uses numeric/alpha destination bytes. Session-versus-
persistent-event identity remains open. Its detailed CRC section includes
COBOL arithmetic XOR, ORD/CHAR preflight, raw-byte serialization, intermediate
register results and five conformance vectors. The standalone GnuCOBOL 3.2
reference passed all vectors and full-frame serialization on Lin; the
engineer's actual compiler/runtime has not been provided or tested. No host
firmware implementation or upload is implied by this documentation revision.

## Latest physical firmware status: 0.4.2 deployment

The user requested addressing for 50 nodes. Revision 0.4.2 adds controller
selection of addresses 1..50, per-node last-confirmed state, and explicit
compile-time node identity. Controller is now uploaded and readback-verified
on 0.4.2. Node1 is also uploaded and readback-verified on 0.4.2 with
NODE_ID=1, OFF and all GPIO LOW. Node2 is also uploaded and readback-verified
on 0.4.2 with NODE_ID=2, OFF and all GPIO LOW. Node3 is now uploaded and
readback-verified on 0.4.2 with NODE_ID=3, OFF and all GPIO LOW. All four
requested uploads are complete. Over-air validation of 0.4.2 is still pending;
the programmer remains on Node3 until returned to central. This supersedes earlier current-
revision statements above; those paragraphs describe prior milestones.
See indicator-revision-042.md for hashes, backup and deployment evidence.
Node builds now require --node-id N and have separate build/radio-node-N
output directories. The automatic 10-second bench cutoff remains. Host binary
protocol and head-entry business flow remain unimplemented.

## Prepared node revision 0.4.3 (October 28)

The user requested a light test at the end of node startup with 250 ms flash
phases and two seconds per indication. Node1/2/3 0.4.3 images compile and
host tests pass. Sequence: ACTIVE, LAST, HOLD_OPEN, FULL, FAULT, then OFF.
It starts after successful setup, runs nonblocking and yields to a real
command. Normal commands retain 500 ms phases and a ten-second cutoff.
Nodes1/2/3 now run readback-verified 0.4.3 with passed on-device GPIO boot
sequences and UART OFF cancellation checks. Central remains on compatible
0.4.2; central needs no update. Physical LED observation and post-update
LoRa checks remain pending. See indicator-revision-043.md for hashes, behavior and test plan.
On October 28, the 0.4.2 controller UART test passed and Node1 acknowledged
addressed OFF token 1707266160 on attempt 1 (RSSI -26 dBm, SNR 12 dB).
This is one 0.4.2 reachability check, not all-state or three-node validation.

## Prepared node revision 0.4.3 (October 28)

The user requested a light test at the end of node startup with 250 ms flash
phases and two seconds per indication. Node1/2/3 0.4.3 images compile and
host tests pass. Sequence: ACTIVE, LAST, HOLD_OPEN, FULL, FAULT, then OFF.
It starts after successful setup, runs nonblocking and yields to a real
command. Normal commands retain 500 ms phases and a ten-second cutoff.
Nodes1/2/3 now run readback-verified 0.4.3 with passed on-device GPIO boot
sequences and UART OFF cancellation checks. Central remains on compatible
0.4.2; central needs no update. Physical LED observation and post-update
LoRa checks remain pending. See indicator-revision-043.md for hashes, behavior and test plan.
On October 28, the 0.4.2 controller UART test passed and Node1 acknowledged
addressed OFF token 1707266160 on attempt 1 (RSSI -26 dBm, SNR 12 dB).
This is one 0.4.2 reachability check, not all-state or three-node validation.

## Withdrawn node revision 0.4.4 (October 30)

The user approved an extra ten-second dark delay after successful node/radio
initialization, before the unchanged ten-second light sequence. Roughly five
seconds initial startup plus setup plus ten seconds wait precede first green.
The delay is nonblocking and cancelled by an indicator command, as is the
active test. Host tests and builds for Nodes1/2/3 pass. All nodes still run
0.4.3 pending uploads; central remains 0.4.2. See indicator-revision-044.md
for exact behavior, prepared hashes and runtime validation tools. The reported
unreliable optical behavior/EN sensitivity remains a separate hardware issue;
the delay is not a proven repair. No EN GPIO wiring or firmware was added.

## Current baseline restored to 0.4.3 (October 30)

The user reported repaired connections and explicitly requested rollback of
Lin and GitHub to 0.4.3, confirming the nodes were never flashed with 0.4.4.
The exact pre-0.4.4 node sketch, BootLightTest helper, boot-test unit tests and
runtime verification scripts have been restored. All three node images are
rebuilt from 0.4.3. The additional ten-second wait is removed; startup retains
its original five-second window plus setup, followed by the ten-second light
sequence. Quarter-second flashing and two-second steps remain unchanged.
The controller remains 0.4.2. No hardware flashing/reset is part of this rollback.
Later EN wiring and test observations are retained. Revision 0.4.4 is marked
withdrawn/never flashed, not deleted from Git history. Current source and deployed
nodes are 0.4.3; do not treat the earlier prepared-revision section as active.

## Node1 confirmed full visual pass (October 31)

The user confirmed all five commanded indications and final darkness during
a fresh complete Node1 radio bench test. All six commands received APPLIED
on attempt 1; final OFF token 1290912394. See indicator-revision-043.md for
the exact record. Central remains 0.4.2; all nodes remain 0.4.3. No hold is
running and no firmware change was made. This completes Node1's full post-
update visual/radio check; earlier optical failures remain historical and
Nodes2/3 validation is separate. Read-only polling remains unimplemented.

## Node2 red-channel FET replacement (October 31)

The user reported a successful test after moving red to IC2's formerly unused
FET and confirmed G=IC2 P3, S=IC2 P2, D=IC2 P4. Red remains controlled by PB4
through 1 kohm; its 10 kohm pull-down is gate-to-source at the new FET. IC1's
old red FET/joints remain suspect, not conclusively diagnosed. The diagram and
wiring table are updated for the replacement assembly; this does not imply
other nodes were rewired. EN is tied to IN+ per the earlier user confirmation.
No firmware change or flash was made. Node2 red hold was stopped; its shutdown
OFF and a retry were unacknowledged, so no fresh radio OFF confirmation is
claimed. The user subsequently reported the replacement test good, without
specifying a new radio transaction or complete five-state visual pass.

## Prepared polling firmware 0.5.0 — October 31

User requested controller and node flash preparation for ratified commands,
leaving TBD commands out while nodes are shut down for Node3 wiring. Prepared
controller plus addressed Node1/2/3 images add live and cached one/all-node
polling; six state commands and bench/boot timings remain unchanged. Seven
C++ test programs pass and all four pinned RAK builds pass. No hardware was
accessed, reset or flashed. Deployed versions remain controller 0.4.2 and nodes
0.4.3. Binary app protocol and flow integration remain excluded pending the
recorded identity/reply/recovery decisions. Exact scope, tests and flash steps:
[indicator-revision-050.md](indicator-revision-050.md). Frozen image checksums,
UIDs and source hashes: [firmware-release-050.json](firmware-release-050.json).

## Ratified indication persistence — October 31

User decision: "Indications stay until changed." Commanded indications are
to remain in effect until superseded by a subsequent command; the ten-second
bench automatic OFF is not the intended operating behavior. This settles the
normal indication-duration question. Power-cycle recovery, startup testing,
communication-failure policy and any explicit sleep/session-close operation
remain separate decisions. This record changes no firmware: deployed nodes
0.4.3 and prepared 0.5.0 images still contain the ten-second cutoff and must
be updated before claiming persistent operation.

## Ratified FULL authority — October 31

User decision: the penning application will set the condition for FULL.
Do not infer FULL automatically from ordinary ACTIVE/LAST advancement or
add an independent controller occupancy rule. This establishes authority,
not a finalized application command frame. FULL clearing, assignment to a
FULL pen, interaction with active/last roles, and reply semantics still
require decisions. Polling remains requested and is prepared in 0.5.0,
not installed. No firmware change or flash accompanies this record.

## Ratified controller routing retention — October 31

User answered yes to retaining the controller routing record across power
loss rather than depending on the application to rebuild it. The controller
is therefore to retain routing intent across restart. Proposed record
contents discussed: ACTIVE/LAST identities, other commanded states, and
unfinished event information. Exact durable layout, atomic commit points,
event deduplication retention and recovery handshake are still to be designed.
Polling/reconciliation without generating a new head event is the proposed
recovery approach. The surrounding proposals for application recovery and
node-dark startup were not separately ratified by this yes. No firmware
implementation, build change or flash accompanies this decision record.

## Ratified node startup and restoration — October 31

User retained the automatic node startup light test and directed restoration
to the controller's intended state after the startup sequence, referring to
the existing approximately fifteen-second startup. Preserve the initial
five-second window plus initialization and the ten-second light test. Restore
the retained intended indication after test completion, without generating a
new head event. Routine recovery must not interrupt the full startup light
test. The current firmware can cancel the test on a SET command; recovery
coordination must account for this before claiming the new behavior works.
Fifteen seconds is the nominal sequence duration, not a new exact wall-clock
deadline independent of radio/setup time. The readiness/restoration handshake
and exceptional OFF/other commands during startup remain implementation or
policy details to settle. No firmware change or flash accompanies this record.

## New-auction clear and failed-OFF reboot requirement — October 31

User requested an explicit operation to clear all states and records at the
beginning of a new auction. This is distinct from ordinary reconnect/restart
recovery, which preserves routing. User explicitly confirmed that the
controller should attempt to reboot a node that fails to acknowledge OFF
during this new-auction operation. Remote reboot is now requested behavior,
not an implemented command. No node was rebooted or flashed in this exchange.

Proposed reset scope discussed: all configured indicators OFF, clear active/last
roles, FULL/FAULT intent and pending routing, and durably store the clean state.
Preserving node configuration and a boundary rejecting prior-auction messages
was recommended; exact record scope and old-event handling still require design.
An unanswered OFF does not prove it was unapplied; an unanswered reboot does not
prove reboot. An unreachable node may receive neither. Retry limits, reboot
acknowledgment/identity, startup-test handling, reset-completion criteria and
operator exclusion remain unresolved. Preserve the separately ratified startup
light test; after recovery the intended new-auction state must be OFF, not a
previous-auction indication. No permanent restart loop should be inferred.

## Ratified controller fault for unresolved reset node — October 31

User directed the controller to log a fault on a node when the new-auction
OFF/reboot recovery cannot confirm it. Record a per-node controller fault
rather than claiming successful clearing or reboot. This is a controller
record; it does not establish that the unreachable node displays FAULT/red.
It does not yet decide whether other nodes may proceed, operator exclusion,
fault clearing, durable log retention, timestamps, or the application reporting
frame. Those remain open. No firmware change or flash accompanies this record.

## Ratified two-stage replies — October 31

User approved immediate controller acceptance followed by a detailed
completion result after radio delivery attempts. Acceptance does not mean
node-confirmed application or physical illumination. Completion should give
an overall outcome and affected-pen delivery detail, as discussed: commanded
state, last confirmed state, and whether the node responded. Exact status
codes, binary serialization, event-correlation field and durable acceptance
commit point still require design. The event identity decision is deferred
for discussion with the application engineer. Specific rejection taxonomy
and lost-reply retrieval were recommended in the discussion but not separately
ratified by this yes. No firmware integration or flash accompanies this record.

## Ratified durable acceptance — October 31

User approved that application ACCEPTED means the controller has durably
saved the event for recovery across power loss. Emit acceptance only after
successful durable storage; failed storage must not produce acceptance.
Acceptance is still distinct from completed radio delivery. Exact storage
mechanism, atomicity/recovery implementation and encoded storage-error reply
remain to be designed. Event identity remains deferred to the application
engineer. No firmware implementation or flash accompanies this record.

## Prepared recovery release 0.6.0 — October 31

User authorized removal of the ten-second cutoff, deferred application replies
and asked to commit the remaining agreed recovery changes for upload. Controller
and Node1/2/3 images are now built and frozen. Implemented: persistent node
indications, complete startup test, BOOTING-aware polling, saved controller
state intent/role fields, periodic reconciliation, durable new-auction OFF
intent, bounded failed-OFF reboot recovery and persistent controller fault
latches. Nine strict host test programs pass; all four pinned RAK builds pass.
No wiring graphic changes. Application event input/identity, binary integration,
automatic head-entry advancement and application replies remain deferred; no
placeholder business command or application event field was invented. Exact
implementation/limits: [indicator-revision-060.md](indicator-revision-060.md).
Image/UID/source manifest: [firmware-release-060.json](firmware-release-060.json).
At this preparation milestone no 0.6.0 board has been flashed; deployed baseline
is still controller 0.4.2 and nodes 0.4.3.

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
