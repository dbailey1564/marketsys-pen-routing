# MarketSys prototype context

This is a project-decision record carried forward from Daniel's referenced
**Prototyping MS** conversation and the continuation in Codex on 2026-10-20.
It is not a manufacturer specification or a newly verified bench report.

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
- The penning application provides desired pen states. Cattle, buyer, and routing
  metadata are outside the device protocol.
- The host interface is intended to be binary and host-agnostic over a USB
  virtual COM connection. The physical serial/USB arrangement still needs to be
  documented; this build setup does not establish it.

## Indicator states

| State | Intended indication |
|---|---|
| OFF | Off |
| ACTIVE | Flashing green |
| NEXT | Flashing blue |
| STANDBY | Solid blue |
| HOLD_OPEN | Alternating green/blue |
| FULL | Solid red |
| FAULT | Flashing red |

Flash periods and transition timing are not fixed by this table.

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

Node1 now runs radio receiver/indicator firmware 0.3.0. Matching controller
firmware is compiled but not installed yet; the DAPLink must be moved back to
the controller, with Node1 transferred to breadboard 3.3 V power. The prototype
transport has explicit frames/CRC, acknowledgements and bounded retries. Node1
radio initialization and OFF GPIO status passed; end-to-end radio operation
awaits the physical transfer and controller upload. See radio-bench-link.md.

The UART AT commands are bench controls, not the planned binary host protocol.
Host integration and production power management remain unimplemented.
Official P2P examples remain separate references.

Next: transfer programmer/power and verify staged radio transport, then the host interface.
Resolve production power/recharge and measured OFF/sleep
current separately; inspect existing host framing before fixing its protocol.
