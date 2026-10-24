# MarketSys prototype context

This is a project-decision record carried forward from Daniel's referenced
**Prototyping MS** conversation and the continuation in Codex on 2026-10-20.
It is not a manufacturer specification or a newly verified bench report.

## Agreed architecture

## Standing version-control workflow

User instruction, October 24, 2026: all future iterations saved to the Lin
development repository are to be committed and pushed to GitHub without
requesting push approval again. Include source, tooling, documentation and
iteration/test records. Verify the remote branch matches the local commit
after pushing. If authentication or another error prevents a push, report
the unsynced status explicitly and resolve it before claiming synchronization.
Private flash backups and credential-bearing files remain outside Git.

## Agreed architecture (continued)

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
- The penning application provides desired pen states. Cattle, buyer, and routing
  metadata are outside the device protocol.
- The host interface is intended to be binary and host-agnostic over a USB
  virtual COM connection. The physical serial/USB arrangement still needs to be
  documented; this build setup does not establish it.

## Indicator states

| State | Intended indication |
|---|---|
| OFF | Off |
| ACTIVE | Solid green |
| NEXT | Solid blue |
| HOLD_OPEN | Flashing green |
| FULL | Solid red |
| FAULT | Flashing red |

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
- Header/framing and CRC remain open pending inspection of the penning
  application's existing binary framing.
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

Next: inspect the penning application's existing framing and implement the host interface.
Resolve production power/recharge and measured OFF/sleep
current separately; inspect existing host framing before fixing its protocol.
