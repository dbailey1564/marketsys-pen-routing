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
