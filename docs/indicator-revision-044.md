# Node revision 0.4.4: delayed power-on light test

Prepared October 30, 2026 after the user reported seeing only the final red
portion when applying power and approved an additional ten-second delay.
This is a prepared revision, not a completed hardware upload. All three nodes
still run verified 0.4.3; central remains compatible on 0.4.2.

## Startup behavior

- Outputs remain LOW during the existing five-second startup window and setup.
- After successful node/radio setup, schedule a further **10000 ms dark delay**.
- Then run ACTIVE solid green, LAST solid blue, HOLD_OPEN flashing green,
  FULL solid red and FAULT flashing red, two seconds each, followed by OFF.
- Flash phases remain 250 ms on / 250 ms off. The indication sequence itself
  remains ten seconds long. The first indication starts roughly fifteen seconds
  after power-on plus variable initialization time; this is not a hard fifteen-
  second deadline measured from an externally sensed supply edge.
- A failed initialization keeps the outputs dark and does not schedule the test.

The added wait is nonblocking. Normal serial/radio handling continues after
setup. A valid addressed SET command, or UART OFF, cancels either the pending
delay or a running light sequence. The cancelled test must never start later
over an application command. Wrong-node/invalid commands do not cancel it.
Normal indicator commands retain 500 ms phases and the ten-second bench cutoff.

The helper's running() covers both pending delay and active light steps;
waiting() distinguishes them. During the wait, status reports OFF, GPIO LOW,
BOOT_TEST=1 and MS_BOOT_WAITING=1. START is logged only when the indication
sequence actually begins, following WAIT; DONE reports final OFF.

## Verification

Host tests passed with AddressSanitizer and UndefinedBehaviorSanitizer for
initial dark delay, the 9999/10000 ms wait boundary, all five states, 250 ms
phase boundaries, two-second steps, final OFF, millis() rollover, skipped/late
updates, cancellation both before and after light-test start, and unchanged
normal command timing. Existing codec/CRC/state-pattern and all-50-address
isolation tests passed. No on-device or optical result is claimed before upload.

The three explicit-address images compiled with pinned RUI3 4.2.4 SiP tools.
Maximum program storage: 160952 bytes; static RAM: 33928 bytes; reported
remaining static allocation: 14712 bytes. This is not runtime stack measurement.

| Prepared node | SHA256 |
| --- | --- |
| Node1 | `211e06542fca790ae822be28bf12dede89d9bca8214c26169637ca0453a677b5` |
| Node2 | `dce80ff796c97521554337e161368d92f21e873f8245d74b4a7128f858ba51e9` |
| Node3 | `5f230ddde6f3420d6b536391c2ffa7fcec92ea2886eff79ff410c1e296a4fdbe` |

Runtime verification tools now default to 0.4.4; use --version 0.4.3 for the
previous revision. observe_boot_light_test.py records the dark interval and
checks GPIO stays LOW, measures its duration and observes the full sequence.
check_boot_cancel.py --during-delay tests cancellation before the first light
and watches past its scheduled start. Without that flag it cancels ACTIVE.

## Deployment and remaining diagnosis

The programmer was last confirmed on central. Next, with breadboard power off
and DAPLink USB disconnected, move all six programmer connections to Node1.
Use DAPLink as its only power source while programming; keep its antenna
attached. Verify its hardware identity, take a fresh two-read private backup,
upload its address-1 image and verify the application and preserved bootloader
against its own backup. Repeat individually for Nodes2/3 if proceeding with
the same revision. Central needs no firmware change.

The user's reported optical inconsistency, EN hand sensitivity and possible
power sequencing remain separate hardware observations. The new delay gives
additional readiness/observation time; it does not establish or prove repair
of their root cause. A physical power-cycle observation is still required
with the LED driver powered and stable. Avoid issuing OFF or other state commands
that cancel the pending sequence while observing startup.

## Pre-update Node1 radio light test

At the user's request, the controller ran the existing normal indication
commands before Node1 was connected for its 0.4.4 upload. Each illuminated
state had its full ten-second window, followed by a fresh OFF. These are
normal 500 ms flashing phases, not the accelerated 250 ms boot sequence.

| State | Token | Result | Attempt |
| --- | --- | --- | --- |
| ACTIVE | 2704632808 | APPLIED / ACTIVE | 1 |
| LAST | 3449343149 | APPLIED / LAST | 1 |
| HOLD_OPEN | 3722287221 | APPLIED / HOLD_OPEN | 1 |
| FULL | 3328906513 | APPLIED / FULL | 1 |
| FAULT | 1778992942 | APPLIED / FAULT | 1 |
| OFF | 1249496033 | APPLIED / OFF | 1 |

All six commands were acknowledged on the first attempt. Final confirmed
state is OFF. Optical confirmation is pending user observation; an ACK does
not establish that the requested LED die illuminated. Central reported
0.4.2; Node1's last verified firmware remains 0.4.3. No 0.4.4 upload occurred.
Logs: build/node1-pre044-active.log, node1-pre044-last.log,
node1-pre044-hold-open.log, node1-pre044-full.log, node1-pre044-fault.log
and node1-pre044-final-off.log.
