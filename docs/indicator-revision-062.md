# Controller revision 0.6.2 — HOLD_OPEN clears previous LAST

November 4, 2026. User corrected the expected flow: every new head advances
the indication sequence, including a repeated destination. A repeated active
destination becomes HOLD_OPEN and the previous LAST becomes OFF; the LAST
identity is cleared to zero. Example: Node1 LAST / Node2 ACTIVE plus another
head to Node2 produces Node1 OFF / Node2 HOLD_OPEN / LAST=0. A further head to
Node2 remains HOLD_OPEN with no LAST. A subsequent different destination makes
the old active identity LAST normally. This supersedes the earlier rule that
reserved blue throughout HOLD_OPEN.

The planner now saves the old-LAST OFF intent and cleared role together with
HOLD_OPEN. The radio work list includes that old node even when the active
identity is unchanged. Thus clearing a role alone is never mistaken for
confirmed physical delivery. An unresolved OFF pauses flow at that step;
explicit retry and restart preserve its pending OFF intent and LAST=0.

Only the controller changes; nodes stay on 0.6.0. Radio frames, storage format,
startup behavior, persistence and deferred application interface are unchanged.
No wiring or graphic changes. The destination-only walkthrough expectations
now clear blue on repeated destinations:

| Step | Input | Node1 | Node2 | Node3 |
| --- | --- | --- | --- | --- |
| 0 | Clear | OFF | OFF | OFF |
| 1 | 1 | ACTIVE | OFF | OFF |
| 2 | 1 | HOLD_OPEN | OFF | OFF |
| 3 | 2 | LAST | ACTIVE | OFF |
| 4 | 2 | OFF | HOLD_OPEN | OFF |
| 5 | 3 | OFF | LAST | ACTIVE |
| 6 | 3 | OFF | OFF | HOLD_OPEN |
| 7 | 2 | OFF | ACTIVE | LAST |
| 8 | 1 | ACTIVE | LAST | OFF |
| 9 | Clear | OFF | OFF | OFF |

All eleven strict host programs pass. Pure flow tests cover all 2450 distinct
active/last pairs both for same-active clearing and return-to-LAST selection,
plus further repeated HOLD_OPEN. Actual-sketch tests cover unresponsive old
LAST during clearing, durable pause, restart and explicit retry. Existing
polling, addressing, storage, boot and recovery regressions pass. Controller
build: 166352 bytes program, 34936 bytes static RAM. Frozen image and source
hashes: firmware-release-062.json. Hardware upload/readback and corrected
advancement walkthrough remain pending until recorded below.

## Installed and hardware-verified correction — November 4

Fresh controller backup: prototype-01/20261104T152630.976714Z; two matching
full reads, SHA256 3694e5895afbeecb41a48a2e0ee2be5b801f5ebc44f3c2116192b32732f780b6.
Controller UID matched; old application matched verified 0.6.1, valid saved
MSR2 records and reference bootloader passed. Official RAK uploader installed
182960-byte 0.6.2 image SHA256
d0cb198976d681074efeedf0c6ecd3b78da11aa11d046dd1ea180544f8930441.
UART reports CENTRAL READY=1, FW=0.6.2, STORAGE=OK and FLOW IDLE.

Private readback: prototype-01/20261104T152855.484298Z-post-addressed-upload/verification.json.
UID, exact application and checked bootloader match. Entire saved routing
user-data partition matches the fresh backup byte for byte before testing.
Full readback SHA256 5e34aaa1fe1abc749ca1befa263467a7d6fb4de2519ddae7544a93b144aecadd.

The first verification attempt stopped on an assertion over a printed diagnostic
line split by host chunk printing; controller ACKs and progression were valid.
Cleanup confirmed all OFF. Host logging now preserves received chunks instead
of inserting artificial newlines. No second firmware flash was required.
Retained first log: build/flow-062-hold-clear-attempt1.log.

Repeated hardware verification used destination-only inputs 1,2,2,3,3 at
eight-second start spacing. Fresh FLOW status and all-node live polls verified:
1 ACTIVE/OFF/OFF; 2 LAST/ACTIVE/OFF; repeated 2 OFF/HOLD_OPEN/OFF with LAST=0;
3 OFF/LAST/ACTIVE; repeated 3 OFF/OFF/HOLD_OPEN with LAST=0. Explicit final
new-auction clear and live polls confirmed all OFF. This verifies reported
node states and controller role clearing; a separate user optical pass is
not claimed. Logs: build/controller-062-*.log and flow-062-hold-clear-test.log;
structured results: build/flow-062-hold-clear-result.json. Nodes remain 0.6.0.
