# Current measurements before solar and battery selection

Status: prepared plan, not a completed measurement. Target: prototype-01,
RAK3272-SiP Ver B / RAK3172-SiP. Timer results: sleep-timer-test.md.

## Inputs required before wiring

- Meter/current analyzer make and model, including range, burden voltage,
  logging/sample capability and peak limits from its manual.
- Regulated bench supply make/model and available controls.
- Installation town/state, panel orientation/shade, reserve-session target.

The documented setup powers the target from RAKDAP1 3V3/GND, with UART and SWD
signals attached. That connection does not provide a current measurement.
No wiring changes or additional firmware uploads were made for this plan.

## Proposed arrangement: finalize against instrument manuals

Use one regulated 3.3 V source for the bare board, measuring current in its
supply path. Verify voltage at the target under load. RAK specifies 2.0–3.6 V
operation and requires the antenna before powering. Keep LED/driver, battery
and charger separate during this first measurement.

Change wiring with power removed. Do not connect another supply to the
programmer's 3V3 output. For the standalone sleep baseline, disconnect the
programmer's target power and UART/SWD signals: debug behavior and signal
paths can affect consumption. A later comparison can deliberately add UART.

Never connect a meter in current mode across the supply. Its series range must
handle startup/active current without excessive target voltage drop. Confirm
the actual meter's connections and limits first; an unknown microamp range
is not assumed suitable for the whole startup/wake sequence.

Sources: [RAK board datasheet](https://docs.rakwireless.com/product-categories/wisduo/rak3272-sip-breakout-board/datasheet/)
and hardware-identification.md for the observed connection. Instrument-specific
wiring remains pending identification of the available equipment.

## Measurement sequence

1. Verify source voltage and current measurement using a known load appropriate
   to the identified equipment; record measurement range and lead arrangement.
2. Record bare-board startup and ordinary idle as a baseline.
3. Prepare separate measurement firmware with long repeatable phases and no
   recurring serial printing. Use the SDK configuration that boots successfully;
   the radio-disabled configuration faulted before sketch setup.
4. Measure requested sleep with UART/SWD disconnected: steady current, periodic
   wake behavior, supply voltage and integrated charge/energy if supported.
5. Repeat with UART attached only to quantify its effect if needed.
6. Measure actual finite RX/TX and transitions after specifying radio settings.
   The timer harness's simulated window is not radio reception.
7. Measure LED-driver input across colors, supply voltage and brightness,
   including dropout and disabled draw. Then measure the integrated load.

The existing harness restores lpm=0 and level=1 at completion and reruns after
reset. A reading after completion would measure that restored state, not its
requested sleep phase. Its serial logging is also unsuitable for an unqualified
minimum-sleep-current benchmark.

Record firmware hash, target voltage, instrument/range, attached signals,
temperature, duration and supported current/energy quantities. A slow reading
alone cannot establish peaks or integrated energy of short wake/radio events.

## Energy sensitivity: illustrative assumptions, not selected parts

The earlier user-reported blue point (2.80 V, 0.163 A) gives 3.6512 Wh at the
LED for eight continuous hours. Battery-to-LED efficiency is unmeasured:

| Assumed efficiency | One session, LED-related battery Wh | Two sessions, LED-related battery Wh |
| --- | ---: | ---: |
| 100%, ideal arithmetic lower bound | 3.651 | 7.302 |
| 85%, illustrative | 4.296 | 8.591 |
| 70%, illustrative | 5.216 | 10.432 |

These omit logic/radio, idle losses, inter-session time, temperature, aging,
permitted discharge fraction and reserve. They are not complete usable-energy
requirements or nominal battery sizes. Two sessions are a sensitivity case.

Using manufacturer typical RX/sleep currents for the proposed 5/120 duty yields
0.25745 mA, excluding transitions/MCU/TX. At assumed 3.3 V over 160 hours this
is 0.13594 Wh before conversion losses. This is not measured standby energy;
see standby-wake-design.md. The 160 hours assumes one eight-hour session in
a 168-hour week; other schedules require recalculation.

Final sizing uses measured input energy by state. Divide required delivered
energy by an explicitly justified usable battery fraction for nominal Wh.
Panel sizing must specify replenishment energy, recovery days, worst-season
sun-hours and solar-path losses. Do not finalize parts using an assumed generic
solar location, battery chemistry or efficiency.

Next decisions: equipment-specific wiring, LED-state mix, installation exposure
and reserve; then battery, charger, panel and regulators.
