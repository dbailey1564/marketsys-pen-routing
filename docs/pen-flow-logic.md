# Controller-owned pen flow: decisions and open questions

October 24, 2026. This records the user's flow correction after the verified
0.4.0 six-state indicator milestone. It changes the intended application
contract; no new flow firmware or host packet format is implemented yet.

## Confirmed direction

- The penning application sends the controller the next ACTIVE pen assignment.
  It should not have to send separate ACTIVE and NEXT indicator commands.
- Central manages state advancement and determines the pen-node indications.
- A new assignment for a different pen activates that pen immediately; a
  separate advance trigger is not required. The previous ACTIVE pen becomes
  NEXT (solid blue), not automatically FULL or OFF.
- A new assignment for the same active pen produces HOLD_OPEN (flashing green).
  The controller retains that pen's active identity while showing HOLD_OPEN.
- Pen nodes continue to apply central's commands and acknowledge application.
  Nodes do not receive cattle, buyer, or routing metadata or decide the flow.
- Current indications remain OFF, ACTIVE solid green, NEXT solid blue,
  HOLD_OPEN flashing green, FULL solid red and FAULT flashing red. STANDBY
  is retired. This flow discussion does not change those indications.

This supersedes the earlier assumption that the penning app chooses all
desired indicator states. Existing ATC+PEN commands remain diagnostic
low-level state commands; they are not the future application event contract.

## Proposed event identity requirement

A new logical assignment must be distinguishable from retransmission of the
same assignment. An opaque event ID/sequence can provide this without cattle
or buyer metadata. Exact field sizes, session/reset behavior and framing
remain unspecified. This is a proposed protocol requirement, not an already
implemented host field.

For example, two distinct assignment events naming pen 12 may intentionally
produce HOLD_OPEN; retransmitting one event naming pen 12 must not accidentally
advance the flow or produce HOLD_OPEN. Radio retry tokens alone do not define
application-level event identity. Host-event deduplication is not implemented.

## Confirmed transition example

| Before | New assignment | Result |
| --- | --- | --- |
| Pen 12 ACTIVE | Pen 14 | Pen 14 ACTIVE; pen 12 NEXT |
| Pen 14 ACTIVE | New assignment to pen 14 | Pen 14 HOLD_OPEN; pen 12 remains NEXT |

NEXT therefore identifies the previous active pen in this flow; do not assume
it means a future queued assignment. The app still supplies the incoming
assignment only; central derives the related indications.

## Remaining business-rule question

With 12 NEXT and 14 ACTIVE, a new assignment to 16 makes 16 ACTIVE and
14 NEXT. The disposition of the older NEXT pen 12 is not yet specified:
OFF, FULL, or still NEXT are pending the user's answer. Do not silently
choose an older-pen state or assume at most one NEXT pen until resolved.

Then define initialization and return from HOLD_OPEN, plus cancellation/
correction, reset/resynchronization and node-ACK failure effects before
claiming production flow behavior. No automatic FULL event is established.

## Current implementation boundary

The 0.4.0 firmware has verified radio state control and bounded transport,
not controller-owned business flow. The 10-second automatic OFF remains a
bench timeout, not an advance/close event or a production persistence rule.
No node/controller has been reflashed during this discussion.
