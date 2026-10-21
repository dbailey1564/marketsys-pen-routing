# Node power and recharge: requirements before component selection

Status: preliminary analysis, 2026-10-20. Battery chemistry, capacity, charger,
regulator topology, and connectors have not been selected.

## User requirements now established

- Solar remains connected for continuous recharging (available energy still
  depends on sunlight; continuous charging current is not assumed).
- Provisional operating duty: one **8-hour auction session per week**. The
  auction house has not yet confirmed actual operating hours.
- Support sleep with the indicator off between sessions.
- Central activation from that sleep state should take **no more than five
  minutes**, including acknowledgement. This is not the active-session command
  latency specification.
- Proposed initial timing is described in standby-wake-design.md. It remains
  a design candidate pending radio, clock, and current measurements.

## Established inputs

- The photographed target is RAK3272-SiP Ver B / RAK3172-SiP.
- RAK specifies **2.0–3.6 V operating supply**, with typical radio figures of
  **87 mA at 20 dBm TX**, **6.14 mA RX**, and **1.69 uA sleep**. These are
  manufacturer conditions, not measured consumption of our complete node.
  See the [board datasheet](https://docs.rakwireless.com/product-categories/wisduo/rak3272-sip-breakout-board/datasheet/).
- The current architecture uses one LD1500SB constant-current board per node
  and three switched RGB color paths. The roughly 160 mA LED setting remains
  provisional pending visibility testing. Source: project-context.md and the
  earlier user-reported bench results.
- The board supplier advertises a 2.9–6.1 V input range for LD1500SB and says
  input must exceed LED forward voltage plus required dropout. These are
  supplier claims, not verified limits for the actual bench unit; inspect the
  board/chip and measure dropout/idle draw before selecting the supply rail.
  [Supplier product page](https://eletechsups.com/driver-board-c-1_23/ld1500sb-adjustable-281500ma-output-current-high-brightness-led-constant-current-driver-board-for-flashlight-headlights-emergency-lights-p-502.html).

## What the LED measurements imply

These calculations use the earlier reported operating points; they are LED
electrical output power, not battery input power, optical power, or whole-node
consumption.

| Color | Reported operating point | Calculated LED electrical power |
|---|---|---:|
| Green | 2.63 V, 152 mA | 0.400 W |
| Red | 2.02 V, 162 mA | 0.327 W |
| Blue, good sample | 2.80 V, 163 mA | 0.456 W |

For the provisional eight-hour session: continuous blue at that point consumes
about **3.65 Wh in the LED**. A hypothetical 50% on/off duty cycle gives about
**1.83 Wh**, excluding driver and electronics consumption. Fifty percent is an
example, not an accepted flash-timing requirement. Alternating
colors without a dark interval does not provide that same duty-cycle saving.

The eventual budget must sum time spent in each indicator state, radio receive
and transmit time, MCU activity, conversion/driver losses, idle currents, and
usable battery energy at the chosen cutoff and temperature. Battery mAh alone
is not a rail-independent energy budget; use Wh until voltage/topology is set.

## Requirements still needed from the user

1. Auction-house confirmation of the provisional weekly eight-hour session.
2. Approximate time in solid/flash/off states and required active-session command
   latency. The five-minute bound applies only to waking from inter-session sleep.
3. Solar location, orientation, shading, and worst-season conditions; required
   reserve for missed charging days or an extra auction session.
4. Available panel area and charging installation arrangement.
5. Enclosure/mounting constraints and expected outdoor temperature/exposure.

Do not finalize battery capacity or a charging module until duty, solar exposure,
temperature, and LED visibility/current are established. Keep battery
and LED-driver power disconnected from prototype-01 during this requirements
step; its current wiring is a debugger-powered bare-board bring-up arrangement.

## Planned bench measurements

- Actual RAK module current during idle, RX, TX, and the proposed listen/sleep
  schedule; the timer-only harness has passed, but no current was measured.
- LED driver input current, dropout behavior, and disabled-state current at
  the measured RGB operating points and proposed supply voltages.
- Regulated logic-rail behavior under LED switching and radio current peaks.

See current-measurement-plan.md for the next measurement sequence and pending
instrument details. These measurements follow a reviewed power arrangement; this document does not
authorize connecting an unspecified battery or assuming the programmer powers
the final LED circuit.
