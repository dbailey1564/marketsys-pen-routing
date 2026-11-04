# Controller revision 0.6.1 — automatic destination advancement

Prepared November 4, 2026. Controller-only update; nodes remain on verified
0.6.0. Implements the normal flow ratified in pen-flow-logic.md independently
of the still-deferred binary application event interface and replies.

## Input and behavior

`ATC+DEST=node` submits one new bench head destination, numeric node 1–50.
The controller computes the related states:

- First destination: ACTIVE, no LAST.
- Different destination: new destination ACTIVE; previous ACTIVE becomes
  LAST; older LAST becomes OFF.
- Same active destination: HOLD_OPEN; LAST stays unchanged.
- Returning to LAST swaps identities without sending OFF to the new ACTIVE.

ACTIVE is solid green; LAST is solid blue; HOLD_OPEN flashes green. Indications
remain until changed. Repeated deliberate DEST invocations are separate new
heads. Do not resend a destination after an ambiguous/lost acceptance reply:
query `ATC+FLOW=?`. This prototype bench input does not define the application's
future event identity, duplicate/replay rules or binary frame.

The destination must be configured. FULL/FAULT destinations and inconsistent
manually-created ACTIVE/LAST baselines are rejected as unsupported rather than
silently choosing an unresolved override policy. Unrelated FULL/FAULT pen states
are preserved. Use the explicit new-auction clear to initialize the walkthrough.
Numeric node identity is the bench destination; alpha mapping remains deferred.

## Delivery and durable progress

The complete desired-state/role change is saved before radio dispatch. A durable
progress index tracks the affected nodes. New ACTIVE is checked/applied first,
then the previous ACTIVE, then any older LAST requiring OFF. Each node is
live-queried before applying a necessary SET. Booting nodes complete their
startup test before restoration. Successful steps save progress before the
next step. A controller restart resumes an unfinished, unpaused operation
without treating it as another head.

An unresolved step pauses advancement and latches a per-node controller fault.
Ordinary background reconciliation does not bypass a paused flow and apply its
remaining changes. `ATC+FLOW=RETRY` explicitly resumes the same saved step;
`ATC+AUCTION=CLEAR` can instead cancel paused work and initialize a new auction.
A controller restart preserves a paused operation. This is the bounded stop/retry
behavior for the bench walkthrough, not a finalized production continuation
policy. Partial physical states can exist; the operation is not radio-atomic.
A save failure blocks further state-changing work and is never reported DONE.

During active advancement other state-changing operations are busy/rejected;
cached/status reads remain available. While paused, live polling is available,
but new destinations, manual PEN overrides and roster changes are blocked until
retry or explicit new-auction clear. Existing polling, persistent indications,
new-auction OFF/reboot/fault recovery and node startup behavior are retained.

## Commands and host tools

| Command | Meaning |
| --- | --- |
| `ATC+DEST=node` | One new bench head destination |
| `ATC+DEST=?` / `ATC+FLOW=?` | Roles, progress and pending/paused state |
| `ATC+FLOW=RETRY` | Resume paused operation; does not create another head |
| `ATC+AUCTION=CLEAR` | Existing explicit new-auction reset; cancels paused flow |

Flow emits PENDING, DONE or PAUSED diagnostics. These are bench diagnostics,
not the finalized two-stage application reply protocol. The existing host helper
adds `destination --node N`, `flow-status`, and `flow-retry`. A destination is
never automatically resent after an ambiguous reply; a busy rejection may be
retried because it did not accept an event.

`flow_walkthrough.py` now sends only destinations. The earlier manual-state
simulation is replaced; it does not send ACTIVE/LAST/HOLD_OPEN itself. Each step
live-polls all three nodes against the expected table, then holds until `next`.
`start`/`stop` use the existing explicit new-auction clear. See
[three-node walkthrough](three-node-flow-walkthrough.md).

## Storage compatibility

The two 176-byte slots and their erase-page offsets are unchanged. MSR2 uses
previously reserved bytes for pending/paused flow, previous role identities and
progress. The loader accepts existing MSR1 records without clearing routing
intent; subsequent saves use MSR2. Tests cover legacy load and subsequent save.
Firmware 0.6.0 cannot interpret new MSR2 records, so a rollback must account for
the fresh pre-upload backup rather than merely flashing the older binary.

## Validation and upload plan

Eleven strict C++ host test programs pass; Python host tools compile. Tests
exercise the actual controller sketch, persistence before dispatch, failure
pause, prevention of maintenance bypass, explicit retry, paused restart and a
restart midway through a successful advancement. Pure transition tests cover
all 2450 distinct ACTIVE/LAST return pairs, repeated destinations, older-LAST
clearing, invalid destinations and preserving unrelated FULL states. Existing
radio, addressing, boot, polling and storage regressions pass.

Pinned RAK build passes: controller program 166344 bytes; static RAM 34936 bytes.
These are compile reports; on-device flow validation remains pending.
[Image/source manifest](firmware-release-061.json) records the frozen controller
image, expected UID and hashes. No node upload is required.

Before programming: verify the attached controller, take two matching fresh
full-flash backup reads, validate existing saved slots and the frozen image.
After programming: verify exact application/UID/bootloader readback, confirm
old routing storage is retained, and verify radio/storage/version readiness.
Then run the destination-only walkthrough one human-observed step at a time.
App frame/event identity, application replies, correction and production
exception policies remain deferred. No wiring or diagram changes are included.
