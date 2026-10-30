# Indicator bench wiring and resistor stage

## Current EN connection reported October 30

The user confirmed EN is tied to driver IN+ on the soldered assembly. Earlier
manual-test notes and the October 26 PNG show the prior unconnected-EN setup;
that EN detail is historical. No GPIO is connected to EN and no enable GPIO
firmware has been added. Red illuminated; blue/green have been intermittent,
with a suspected loose connection and retesting recorded in indicator-revision-044.md.


[Complete node wiring diagram - high-resolution PNG](images/node-complete-wiring.png)
shows the operating power/GPIO wiring and separate programming-only connections.

Updated October 24, 2026. User-reported manual OFF/ON tests passed for all
three colors on the rebuilt assembly. Earlier suspect assemblies were set
aside; no conclusion about their root cause was established. Replacement
LED measurements reportedly matched the original within 0.01 A and 0.02 V.
The provisional driver target remains approximately 160 mA, one color at a time.

## Corrected FDC6401N mapping

The earlier conversational pin table was wrong and is superseded by this table.
The manufacturer's page 1 was rendered and visually inspected to confirm it.

| Device | Package | Gate | Source | Drain | Color |
| --- | --- | --- | --- | --- | --- |
| Q1 | IC1 | P1 | P5 | P6 | Blue |
| Q2 | IC1 | P3 | P2 | P4 | Red |
| Q3 | IC2 | P1 | P5 | P6 | Green |
| Q4 | IC2 | P3 | P2 | P4 | Unused |

[onsemi FDC6401N datasheet](https://www.onsemi.com/download/data-sheet/pdf/fdc6401n-d.pdf).
Adapter numbering must match installed chip orientation; square pad convention
varies by adapter side/footprint. Follow printed numbers and verify connections.

## Rebuilt driver lead colors, superseding original driver colors

From the latest photographed labels: IN+ green, IN- red, EN brown,
L+ yellow, L- orange. EN remained disconnected during successful manual tests.
LED positive terminals join L+; each negative reaches its selected FET drain.
All three used FET sources reach driver L-, not an arbitrarily substituted GND.
Driver IN- and RAK ground are the supply common ground.

## Resistor stage: installation procedure (subsequently bench-tested)

With both rails off, remove manual gate-to-source shorting jumpers and all
manual 3.3 V gate links. Install three 10 kohm gate-to-source resistors:
IC1 P1-P5, IC1 P3-P2, IC2 P1-P5. A 10 kohm resistor replaces each short;
do not leave the short in parallel. With GPIO disconnected, repeat all-OFF.
The subsequent hookup, firmware upload and tests are recorded in
node1-indicator-test.md; the installation instructions here are retained.

Prepare one 1 kohm series resistor per gate, leaving the controller end isolated
until the correct node firmware and power arrangement are verified.

Installed output assignment (verified against manufacturer header table,
installed RUI3 4.2.4 variant.h on Lin, and subsequent physical color tests):

| Color | RAK physical connection | SDK name | Through 1 kohm to |
| --- | --- | --- | --- |
| Blue | J8 pin 2 | PB3 | IC1 P1 |
| Red | J8 pin 3 | PB4 | IC1 P3 |
| Green | J8 pin 4 | PB5 | IC2 P1 |

[RAK3272-SiP pinout](https://docs.rakwireless.com/product-categories/wisduo/rak3272-sip-breakout-board/datasheet/).
The three pins are available GPIO/analog-capable pins; firmware will explicitly
configure digital outputs. Do not infer output initialization from firmware
that was only built for serial/timer tests.

## Power arrangement and outstanding work

The single RAKDAP1 has been moved to PROTO - Node1 V1. Its 3V3 output powers
Node1; HW-131 5 V powers the driver, with common supply GND. Breadboard 3.3 V
is unused. USB powers HW-131; barrel jack unused. Driver L- remains the
MOSFET source return and is not arbitrarily shorted to supply GND.

The user confirmed initial all-OFF and all three physical color tests after
correcting green's source connection from IC2 P2 to P5. Node1 is backed up
and programmed; the seven-state bench harness and GPIO verification are
recorded in node1-indicator-test.md. The user also visually confirmed all
seven state patterns and their automatic return to OFF.
Driver EN control and measured whole-node OFF current remain separate work.
Dark LED does not establish zero current. Radio and host integration remain
unimplemented in the current recorded harness.
