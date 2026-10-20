# Solar node standby and central activation

Status: **proposed timing design**, 2026-10-20. No sleep firmware or radio
configuration has been uploaded. Numbers below are candidate budgets, not
measured performance or a guarantee under arbitrary RF loss.

## Accepted requirements versus proposed implementation

User requirements: continuous solar connection, a provisional eight-hour weekly
auction session, sleep support between sessions, and at most **300 seconds**
from a central activation request to confirmed node activation.

Proposed implementation: passive, periodic P2P receive windows. The central
retains command/ACK/retry ownership and repeats an activation announcement while
nodes are pending. Nodes do not need a synchronized weekly auction schedule.
The initial design assumes the central is powered, awake, and listening for
acknowledgements during activation. Central power arrangements remain to be
confirmed.

Power/session modes must be separate from indicator states: the existing
indicator state **STANDBY means solid blue**. It must not be reused to mean
inter-session sleep. Inter-session sleep forces indicator OFF and disables its
driver; the exact board EN polarity/off leakage remains a bench-verification
item.

## Candidate timing budget

| Quantity | Candidate value | Meaning |
|---|---:|---|
| Receive-window period | 120,000 ms | Start-to-start, not 120 s sleep plus a window |
| Usable listening time | 5,000 ms | Radio is ready throughout this time |
| Activation-beacon spacing | At most 2,000 ms | Start-to-start while activation is pending |
| Beacon airtime envelope | At most 250 ms | Must be checked against final payload/PHY settings |
| Node ACK/central confirmation budget | 10,000 ms | Includes bounded retries/backoff; not yet specified on air |
| Clock/startup/scheduling allowance | 5,000 ms | Budget to validate with measurements |
| Activation deadline | 300,000 ms | Missing nodes remain unconfirmed at expiry |

Sleep until the next absolute deadline, accounting for work already performed.
Wake the MCU/radio before the usable window begins. An early UART/other interrupt
must not silently move the next periodic deadline or start continuous listening.
Do not use unbounded RX constants (65533/65534/65535) for the quiescent window.
The pinned RUI3 4.2.4 RAKLoRa.h declares finite `api.lora.precv(timeout)` in
**milliseconds**; some older example prose describes units differently.

With no synchronization, a request can just miss a node's opportunity. Budget
up to one full period plus its receive window and the remaining allowances:

```text
first-window bound = 120 + 5 + 10 + 5 = 140 seconds
one lost whole-window bound = 2×120 + 5 + 10 + 5 = 260 seconds
reserve to five minutes = 40 seconds
```

These bounds assume that, once available, a valid complete beacon arrives in
the usable window and ACK/confirmation succeeds within its budget. If one whole
window is lost, the next must succeed. Two consecutive lost complete windows
can exceed five minutes. On an unavailable/busy link there is no finite delivery
guarantee: the central must show the affected node as unconfirmed/unreachable,
not ready. A request received near the end of a window begins the separate
confirmation budget rather than forcing the node back to sleep immediately.

The two-second beacon spacing and 250-ms airtime envelope allow at least two
complete beacons within a full five-second receive window for arbitrary beacon
phase in the timing model. This is not permission to transmit at any particular
RF setting. Select frequency, SF, bandwidth, coding rate, payload length, TX
power, and allowed channel-access behavior before radio tests. Calculate actual
airtime and central RX gaps. Account for half-duplex operation and collisions;
the timing model does not simulate a radio or establish regulatory compliance.

## State and acknowledgement behavior

1. The central starts a session and marks each intended node pending. It repeats
   addressed/group activation requests with a session identifier and sequence.
2. A sleeping node wakes on its local timer and listens for a bounded period.
   It checks packet validity, destination, and session freshness.
3. If no valid activation is received, it turns RX off and returns to sleep.
   Invalid or unrelated packets must not extend the window indefinitely.
4. A valid request moves the node to activation-pending and starts an ACK timer.
   It does not select ACTIVE/green merely because an auction session opened.
   Indicator state changes remain explicit central commands.
5. Central retries, node reply scheduling/backoff, and duplicate handling must
   fit the ten-second confirmation envelope. A new node ACK confirms an actually
   applied state/session; central intent alone does not confirm node readiness.
6. During a session use a separate, faster command-response policy. Eight hours
   is a power-sizing target, not an automatic hard shutdown timer. Session close,
   loss of central contact, boot/restart behavior, security/authentication, and
   any maximum session lease remain open protocol decisions.

## Preliminary energy comparison (module only)

RAK's datasheet gives typical 6.14 mA RX and 1.69 uA sleep for the SiP board.
Using those values at an assumed 3.3 V, ignoring wake/MCU/ACK overhead:

```text
Iaverage = 6.14×(5/120) + 0.00169×(115/120) = 0.25745 mA
inter-session energy = 3.3×0.25745/1000×160 hours = 0.136 Wh/week
continuous RX for the same 160 hours = 3.3×6.14/1000×160 = 3.242 Wh
```

This is a **rough component estimate**, not whole-node battery consumption.
Driver/charger/regulator idle current, TX, transitions, actual module behavior,
temperature, and reserve are excluded. The debugger/USB serial connection can
also affect low-power measurement. For comparison, blue LED output power at the
reported 2.80 V/163 mA point consumes about 3.65 Wh in eight continuous hours.
Solar and battery sizing must use measured whole-node energy and worst-season
recharge availability, not this idealized average alone.

## Implementation and acceptance work

Offline check completed with `python3 scripts/check_wake_timing.py`:

| Window period | Whole windows lost | Sampled cases | Maximum modeled delay |
|---|---:|---:|---:|
| 120 s | 0 | 202,608 | 135.25 s |
| 120 s | 1 | 202,608 | 255.25 s |
| 120 s | 2 | 202,608 | 375.25 s (fails target) |
| 240 s | 0 | 404,208 | 255.25 s |
| 240 s | 1 | 404,208 | 495.25 s (fails target) |
| 240 s | 2 | 404,208 | 735.25 s (fails target) |

The phase grid varies request and beacon start phases, with and without loss
of the first complete beacon after any wholly lost windows. A separate 1-ms
beacon-phase check confirms room for two full packets in a full usable window
under the stated 250-ms envelope. These are deterministic model checks, not
statistical reliability estimates. The more conservative 140/260-second design
bounds above remain the budgets. Output: build/wake-timing-analysis.json.

- First create a compile-only sleep/timer harness, without assigning radio PHY
  or changing the prototype's current verified image. Prove timer behavior and
  recovery before adding RF traffic.
- Measure sleep, wake, finite RX, TX/ACK, and LED-driver disabled currents using
  a reviewed standalone supply arrangement.
- Bench-test activation at arbitrary phases, a deliberately lost window,
  duplicated/invalid packets, ACK collisions, busy central, and central absence.
- Validate start-to-start period and usable window after radio startup; record
  request-to-confirmation time for each node. Passing requires <=300 s under
  the defined one-window-loss test condition; wider loss is reported as fault.
- Verify whether finite RX exits on packet receipt in the pinned implementation;
  if it does, rearm only for the remaining original window after irrelevant
  packets. Never restart a fresh five-second window for every received packet.
- Do not alter accepted power assumptions into an automatic RF/LED test or
  battery/panel purchase. Site sunlight, reserve duration, LED duty, and active
  response latency still require input/measurement.

Sources: user requirements in this conversation; [RAK SiP datasheet](https://docs.rakwireless.com/product-categories/wisduo/rak3272-sip-breakout-board/datasheet/);
the pinned installed RUI3 4.2.4 `RAKLoRa.h` and `sleep.h` API contracts;
[RAK low-power example](https://github.com/RAKWireless/RUI3-Best-Practice/tree/main/RUI3-LowPower-Example).
