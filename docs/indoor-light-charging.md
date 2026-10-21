# Charging from penning-barn house lighting

User clarification: the intended photovoltaic source is artificial house
lighting inside the penning barn, not an outdoor solar installation. Lighting
schedule, illuminance at the panel, lamp spectrum and usable panel area remain
unknown. No energy sufficiency or selected charging hardware is established.

## Manufacturer reference and illustrative energy comparison

[Epishine](https://www.epishine.com/faq/power-output-indoor-solar-cells)
states up to 22 microwatts/cm2 at 500 lux for its indoor cells. This is a
manufacturer reference condition, not measured barn output or a universal
lux-to-power conversion. [PowerFilm's indoor panels](https://www.powerfilmsolar.com/products/electronic-component-solar-panels/indoor-light-series)
also specify low-light electrical output separately from outdoor ratings.

At the Epishine reference density, 100 cm2 of active cell area would produce
2.2 mW. Assuming that output throughout every light-on hour:

| Light-on hours/week | Cell output Wh/week, before charging/storage losses |
| --- | ---: |
| 168 | 0.3696 |
| 56 (8 h daily) | 0.1232 |
| 8 (auction session only) | 0.0176 |

The reported blue LED point requires 3.6512 Wh at the LED per eight-hour
continuous-blue session. Thus even the 24/7 example supplies only about one
tenth of that LED energy, before conversion/storage losses or electronics.
This is a scenario calculation, not a measured shortfall for the actual barn.
Actual LED-state mix and required brightness remain open.

At the same optimistic constant reference density, covering only that LED
energy would require about 988 cm2 active cell area with lights on 168 h/week,
2,964 cm2 at 56 h/week, or 20,745 cm2 at 8 h/week. These are ideal arithmetic
lower bounds under the stated assumptions, not panel recommendations.
Charger, battery, driver, electronics and real light variation add requirements.

## Revised feasibility sequence

1. Establish house-light schedule during and between auctions, including periods
   with no light. Confirm acceptable panel area and mounting orientation.
2. Measure illuminance at the proposed panel surface/orientation, including
   dim locations, shadows and representative dust buildup. Identify lamp type.
3. Measure candidate indoor cell loaded power in that actual light. Open-circuit
   voltage alone is not charge power. Characterize usable voltage/current and
   charger cold-start/operating behavior using the chosen component datasheets.
4. Measure actual LED-state duty and visibility at reduced LED currents, plus
   bare-board and integrated electronics current as planned separately.
5. Compare usable weekly harvested energy against weekly consumption and
   required reserve. A larger battery alone cannot correct a recurring energy
   deficit; it only delays depletion (energy-balance inference).

Working assessment: indoor harvesting is unproven and appears demanding for
the provisional high-current indicator. Evaluate measured feasibility before
choosing battery/charger/panel. If insufficient, options for discussion include
lower indicator energy, more harvesting area, an outdoor panel with wiring, or
another recharge arrangement. None has been selected.

Outdoor location/sun-hours sizing is superseded for this indoor-only proposal.
Keep temperature, enclosure and reserve requirements in the design, but base
harvesting on house-light exposure and measurements at the panel.
