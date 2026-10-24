# Controller-owned pen flow: decisions and open questions

October 24, 2026. This records the user's flow correction after the verified
0.4.0 six-state indicator milestone. It changes the intended application
contract; no new flow firmware or host packet format is implemented yet.

## Confirmed direction

- The penning application sends the controller the next ACTIVE pen assignment.
  It should not have to send separate ACTIVE and NEXT indicator commands.
- Central manages state advancement and determines the pen-node indications.
- A new assignment for the same pen is to produce HOLD_OPEN. The working
  interpretation is comparison with the currently active pen; the detailed
  event/advance timing still needs to be settled.
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

## Questions requiring the user's business rules

1. With pen 12 ACTIVE, does the arrival of next-ACTIVE pen 14 activate 14
   immediately, or display 14 as NEXT and wait for a separate advance trigger?
   If separate, the trigger and its source need definition. This also decides
   how the controller learns enough look-ahead to show NEXT.
2. On advancing to a different pen, does the old ACTIVE pen become FULL,
   become OFF, or wait for an explicit close/full event?

Once these are answered, define initialization, the different-pen path,
same-pen/HOLD_OPEN path, and the return from HOLD_OPEN before implementation.
Also specify cancellation/correction, reset/resynchronization and node-ACK
failure effects before claiming production flow behavior.

## Current implementation boundary

The 0.4.0 firmware has verified radio state control and bounded transport,
not controller-owned business flow. The 10-second automatic OFF remains a
bench timeout, not an advance/close event or a production persistence rule.
No node/controller has been reflashed during this discussion.
