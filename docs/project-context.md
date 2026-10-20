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

## Host protocol work carried forward

- Distinguish commanded state from last node-confirmed state.
- Planned polling commands: POLL, POLL_LAST, POLL_ALL, POLL_ALL_LAST.
- Header/framing and CRC remain open pending inspection of the penning
  application's existing binary framing.
- Radio packet format, addressing, duplicate handling, timeouts, retries, and
  failure/restart behavior require implementation-level specifications.

## Current implementation boundary

RUI3 is the selected initial development path. At this milestone the repository
contains a minimal serial build check and development tooling. The official P2P
examples are maintained separately as references. No MarketSys command parser,
radio state machine, indicator driver, or power management has been implemented.
Prototype-01 has now been programmed with the minimal SiP bring-up sketch after
its original flash was backed up. AT queries and its repeating UART heartbeat
confirm execution; see first-upload.md. No indicator or radio tests have run.

Next engineering questions: confirm the exact module/oscillator variant and
programming connection; resolve node power/recharge; inspect the host framing;
then implement and test indicator behavior and the radio protocol in stages.
