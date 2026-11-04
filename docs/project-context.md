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

## Node3 return and explicit new-auction fault cleanup — October 31

After the user restored Node3 power and confirmed startup finished, live POLL
reported Node3 OFF. Its controller fault remained latched at 1 despite recovered
communication; Nodes1/2 remained fault-free. A subsequent explicit new-auction
clear advanced local generation to 3 and confirmed all three configured nodes
OFF with zero faults. Fault query then showed desired OFF and no fault on all
three nodes. This completes the intentionally offline-node test and cleanup.
No successful remote reboot was observed: Node3 was physically powered off
during reboot attempts. A physical controller power-loss test and interrupted
clear remain separate unperformed tests. Logs: build/auction-060-node3-return.log,
auction-060-final-clear.log and auction-060-final-faults.log. No firmware or
wiring changed. Final state: all configured nodes confirmed OFF, no controller
faults, ACTIVE/LAST empty, auction generation 3.

## Successful remote Node3 reboot verified — October 31

User requested a reboot test. Inspection of the installed RUI3 At_P2pSend
implementation established that its existing AT+PSEND can transmit a valid
MarketSys REBOOT frame without new firmware. An isolated bench helper waited
for an idle controller reconciliation boundary, verified Node3 ready/OFF, and
sent exactly one CRC-checked addressed REBOOT frame with a fresh token. It
matched the REBOOT_ACK by node/token/CRC, then observed Node3 STATUS BOOTING
and subsequent ready/OFF via the normal controller queries. User confirmed
the startup light test completed. Final live POLL confirmed OFF, and controller
fault query showed no faults and desired OFF on all three nodes.

This verifies successful on-device remote reboot execution and post-boot
readiness. It used the installed vendor raw-send interface as a bench injection;
there is still no dedicated MarketSys standalone reboot AT command. Earlier
statements that the firmware cannot remotely reboot at all were too broad:
the node handler exists and is now hardware-tested. No firmware was changed
or reflashed. This test did not force the automatic failed-OFF trigger or
exercise the coordinator's successful-reboot transaction end to end.

Logs: build/node3-060-remote-reboot.log and node3-060-remote-reboot-final.log;
structured result: build/node3-060-remote-reboot-result.json. All nodes remain
intended OFF with no controller faults. Physical controller power-cut and
interrupted-clear acceptance remain separate unperformed tests.

## Prepared controller advancement 0.6.1 — November 4

Resumed after the prior automatic approval review was interrupted by account
usage limits. Implemented controller-owned destination advancement independent
of the deferred app protocol: DEST supplies one bench destination, controller
computes ACTIVE/LAST/HOLD_OPEN/OFF. Durable progress supports restart/resume;
unconfirmed steps pause until explicit retry or new-auction clear. Existing
MSR1 records load without clearing intent; new saves use MSR2. Nodes remain on
0.6.0. Eleven host programs and the controller build pass; host controls compile
and the walkthrough now sends destinations only. Frozen package and exact scope:
indicator-revision-061.md and firmware-release-061.json. No 0.6.1 flash yet.
Lin is reachable; initial UART check did not answer, so physical connection is
being checked before any backup/upload. No wiring graphic changes.

## Controller deployed; destination walkthrough initialized — November 4

Fresh two-read controller backup: prototype-01/20261104T132721.649898Z, SHA256
d2cae0433e075ad09787aed381106100fba136e4951e1a560b66ed2988cd12c1.
SWD UID matched 0027001c4b33501220333734. Backup application matched deployed
0.6.0, and both saved MSR1 routing records were valid (sequences 41/42, local
auction 3, active/last empty, configured 1–3, desired OFF). Initial UART was
unresponsive/garbled; one controlled debugger restart restored valid replies.
A brief USB re-enumeration interrupted a readiness check before programming;
readiness was reverified before the upload.

Official RAK UART uploader installed the frozen 182952-byte controller 0.6.1
image, SHA256 4fda241965498c05cba923015a9ed3347472dbcebd3160aff00e47b0e8dc734c.
Private verification: prototype-01/20261104T133219.666041Z-post-addressed-upload/verification.json.
UID, exact application bytes and checked bootloader preservation pass. Entire
user-data partition 0x08037800..0x0803dfff matched the fresh backup byte for byte
before any new routing operation. Snapshot SHA256
c8fbfa708613193764dfbc357d3723b59d0bc74488c6cbf8dfe0414c78c67b5a.

Walkthrough start explicitly cleared the three-node setup: auction 4, desired
OFF and live OFF on all three, ACTIVE/LAST empty. A subsequent controller UART
restart loaded the new saved record successfully: FW=0.6.1, STORAGE=OK and FLOW
IDLE with auction 4. Local walkthrough is step 0, all OFF, waiting for the user
to request the first destination. No DEST advancement step has yet been sent
on hardware. Nodes remain on 0.6.0; no node flash or wiring/graphic change.
Logs: build/controller-061-*.log, controller-pre-061-*.log, flow-061-start.log;
storage preservation evidence: build/controller-061-storage-preservation.json.

## Node1 reported repeated startup near final red step — November 4

User reported Node1 appears to restart its startup sequence near the final
flashing-red step, unlike Nodes2/3. Controller status showed firmware 0.6.1,
STORAGE=OK, normal recovery idle, desired OFF and no faults on all three nodes.
A forty-second passive UART capture of existing controller traffic contained
multiple valid Node1 STATUS frames reporting OFF/APPLIED, matching Nodes2/3.
No controller reboot or fault messages appeared in that interval. No state or
reboot commands were sent for this observation. The symptom was not reproduced
during that interval; actual reset cause and repeat count remain unknown.
Logs: build/node1-startup-loop-controller.log and
node1-startup-loop-observation.log. Do not attribute this to a brownout, wiring
or firmware reset without further evidence. Advancement walkthrough remains
at step 0; all reported node states OFF.

### Startup symptom clarification

User clarified that Node1 shows the startup sequence twice only, then stops;
a continuous repeating loop is not reported. The passive capture so far has
shown normal OFF status, not a newly captured boot. A synchronized fresh
power-on observation is still required before claiming a second MCU reset
or assigning a cause.

## Captured power-on follow-up — November 4

After user reported power applied, capture showed Node1 temporarily missing
queries/SET OFF during normal RECOVERY, producing a latched controller fault
for unconfirmed delivery. It also captured Node2 BOOTING through LAST,
HOLD_OPEN, FULL, FAULT then ready OFF. Subsequently Node1 reported BOOTING
in FULL and FAULT, then ready OFF repeatedly. No MS_REBOOT_NODE transaction
was recorded. This is evidence of startup timing/availability, not a reset-cause
measurement or complete capture of both user-observed Node1 sequences.

Current desired states remain OFF; Node1's fault is latched at 1, Nodes2/3 at 0.
The sweep's current failure count can be zero while an older fault stays latched.
No new-auction clear or state/reboot command was issued for this diagnosis.
Physical power scope needs clarification because Node2 also reported startup.
Logs: build/node1-fresh-power-on.log, node1-startup-followup.log and
node1-startup-followup-status.log. Firmware and wiring unchanged; walkthrough
remains paused at step 0. Direct node startup/reset evidence is still needed
to distinguish an actual second MCU reset from other observed light behavior.

## Shared supply clarified; direct UART attempt not captured — November 4

User confirmed cycling the bench supply, explaining startup on multiple nodes.
User then moved UART leads to Node1 for direct observation. Initial capture
failed when DAPLink disappeared from USB. On reconnect, bench power was applied
before the replacement capture was armed. That capture received no UART bytes
before the user unplugged USB to return the leads to the controller; it ended
with the expected port-disconnect error. No usable direct startup log or reset
cause was obtained. User observed only one startup sequence on this attempt,
so the earlier twice-only symptom remains intermittent/unexplained. Do not
claim the direct log verified one boot or fixed the issue.

After the leads returned, controller UART was verified: FW=0.6.1, READY=1,
STORAGE=OK, FLOW IDLE, ACTIVE/LAST empty, auction 4. No destination advancement,
firmware update, reset command or light command was issued during the direct
UART attempt. Startup diagnosis remains open; walkthrough remains at step 0.

## Node1 same-image reflash verified — November 4

User reported Node1 again repeated its visible startup with UART disconnected
and explicitly requested reflashing. After user connected Node1 to the programmer
with bench power off, UART reported NODE1/0.6.0 and hardware UID matched. Fresh
two-read backup: proto-node1-v1/20261104T141332.389499Z, SHA256
2a8bf0962e52b6e3b3d2e3ee33b82f66f70875e53ce50ad2052c0e400345dcc8.
Preflight found the existing application already identical to the verified
0.6.0 Node1 image. Node source and its direct header dependencies matched the
frozen release; unrelated controller source changes were excluded from this check.

Official RAK uploader rewrote the same 176256-byte Node1 image, SHA256
a71795bf57fa44348beeb3a884111168cb4b386962f7c2863ed7fabc95ddf1af.
After a deliberate UART restart, all five startup steps and final OFF were
observed once in the fresh UART verification; NODE1 READY=1, address 1,
LIMIT_MS=0, normal phase 500 ms and all output GPIO LOW. Bench driver power
remained off, so this does not reproduce the original loaded bench cold-start.

Full readback verification: proto-node1-v1/20261104T141655.464868Z-post-addressed-upload/verification.json.
UID, exact application and checked bootloader pass; settings region unchanged.
Entire main-flash readback hash equals the fresh pre-flash backup hash above.
Thus no firmware-data difference or application corruption was found; the
intermittent twice-only visible startup remains unexplained pending bench-power
retest. No controller/Node2/Node3 firmware or wiring diagram changes.
Logs: build/node1-reflash-pre-uart.log, node1-reflash-backup.log,
node1-reflash-preflight.json, node1-reflash-upload.log, node1-reflash-runtime.log
and node1-reflash-readback.log. Programmer remains on Node1 until moved by user.

## Double visible startup persists after Node1 reflash — November 4

After returning the programmer to the controller, readiness was verified and
a passive radio capture was armed before bench power. User reported Node1
again ran its lights twice. Thus the requested same-image reflash did not
remove the observed symptom. Capture recorded no controller reboot messages.
It showed Node2 BOOTING through its sequence and ready OFF; Node1 was reported
OFF in the sampled frames. Sequential controller queries did not provide a
complete Node1 startup trace or reset-cause evidence. Unconfirmed faults were
logged while bench power was off/startup; these must not be called the cause
of a reset. All final sampled node states were OFF. Capture:
build/node1-post-reflash-bench-startup.log.

A receive-only direct UART capture under normal bench supply is the next
proposed check: programmer RX to Node1 PA2 and common ground, without wiring
programmer TX or 3V3 to Node1. Capture must be armed before applying bench
power. This has not yet been performed. No additional firmware changes or
reflash are warranted without new evidence.
