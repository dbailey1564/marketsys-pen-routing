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
