# Controller-owned pen flow: decisions and open questions

October 24, 2026. This records the user's flow correction after the verified
0.4.0 six-state indicator milestone. It changes the intended application
contract; no new flow firmware or host packet format is implemented yet.

## Terminology update: LAST replaces NEXT

The user renamed NEXT to LAST. LAST still means the previous active pen and
still displays solid blue. Wire value 0x02 is unchanged. Current radio source
and the host helper use LAST/last; the former NEXT command name is not retained
as an alias. Historical firmware/test records retain their original names.

Both 0.4.1 images compile and host tests pass, including the preserved wire
value and solid-blue pattern. Prepared Node1 SHA256:
`2b28ec905e365f4f2e52be58bcfdd301fbf9f33cf128b4ba1031c026f3bf4a6e`.
Prepared central SHA256:
`2d19bf10ced48b13c3e5678cb0c9299e5c9efbd96df9c7a9e9cb75ace1d9c822`.
At that milestone neither board had been reflashed. The subsequent 0.4.2
50-node revision is now deployed and verified on central and Node1/Node2/Node3.
See indicator-revision-042.md for current hardware status.

## Confirmed direction

- The penning application sends the controller the next ACTIVE pen assignment.
  Specifically, it reports one destination per head after sale as that head
  enters the penning alley. Do not advance solely from an earlier sale-recording
  event if alley entry has not occurred. Consecutive heads are separate events.
  It should not have to send separate ACTIVE and LAST indicator commands.
- Central manages state advancement and determines the pen-node indications.
- A new assignment for a different pen activates that pen immediately; a
  separate advance trigger is not required. The previous ACTIVE pen becomes
  LAST (solid blue), not automatically FULL or OFF.
- A new assignment for the same active pen produces HOLD_OPEN (flashing green).
  The controller retains that pen's active identity while showing HOLD_OPEN.
- When a different-pen assignment advances the positions, the older LAST pen
  becomes OFF. Thus there is one active identity and at most one previous/LAST
  identity in this basic flow.
- Pen nodes continue to apply central's commands and acknowledge application.
  Nodes do not receive cattle, buyer, or routing metadata or decide the flow.
- Current indications remain OFF, ACTIVE solid green, LAST solid blue,
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
| Pen 12 ACTIVE | Pen 14 | Pen 14 ACTIVE; pen 12 LAST |
| Pen 14 ACTIVE | New assignment to pen 14 | Pen 14 HOLD_OPEN; pen 12 remains LAST |
| Pen 12 LAST; pen 14 ACTIVE | Pen 16 | Pen 12 OFF; pen 14 LAST; pen 16 ACTIVE |
| Pen 12 LAST; pen 14 HOLD_OPEN | Pen 16 | Pen 12 OFF; pen 14 LAST; pen 16 ACTIVE |

LAST therefore identifies the previous active pen in this flow; do not assume
it means a future queued assignment. The app still supplies the incoming
assignment only; central derives the related indications.

## Derived controller update rules

For each genuinely new assignment to pen P, let A be the current active pen
identity (whether displaying ACTIVE or HOLD_OPEN) and N the previous/LAST pen.

1. If P equals A, display HOLD_OPEN on A and leave N and the positions alone.
2. Otherwise, set the older N to OFF unless N is also the newly selected P.
   Set the old A to LAST, and set P to ACTIVE. Store N=old A and A=P.
3. If no active pen exists yet, the natural initialization is P ACTIVE with
   no LAST pen. This is a derived initialization proposal, not a separately
   confirmed restart/resynchronization policy.

When a previously LAST pen is selected again, compute the final desired
states by identity: do not issue a stale OFF after making that same pen ACTIVE.
For example 12 LAST/14 ACTIVE plus assignment 12 yields 12 ACTIVE/14 LAST.
These edge cases are derived from the confirmed rules and are not yet coded.
Repeated delivery of an existing event does not execute these rules again.

## Remaining flow details

Cancellation/correction, reset/resynchronization, node-ACK failure effects,
and persistence beyond the bench timeout still need decisions before claiming
production flow behavior. No automatic FULL event is established by the
confirmed advancement rules; FULL and FAULT remain separately available
indicator states with their triggering events not yet defined here.

## Current implementation boundary

The 0.4.0 firmware has verified radio state control and bounded transport,
not controller-owned business flow. The 10-second automatic OFF remains a
bench timeout, not an advance/close event or a production persistence rule.
The subsequent controller 0.4.2 upload adds addressed diagnostic control,
not business flow; see indicator-revision-042.md. Node1 is also verified
on 0.4.2, as are Node2 and Node3. All requested uploads are complete;
0.4.2 over-air validation is pending.

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
