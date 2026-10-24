# Indicator bench wiring and resistor stage

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

## Resistor stage: proposed, awaiting user installation and validation

With both rails off, remove manual gate-to-source shorting jumpers and all
manual 3.3 V gate links. Install three 10 kohm gate-to-source resistors:
IC1 P1-P5, IC1 P3-P2, IC2 P1-P5. A 10 kohm resistor replaces each short;
do not leave the short in parallel. With GPIO disconnected, repeat all-OFF.
No RAK GPIO connection or firmware upload is established by this document.

Prepare one 1 kohm series resistor per gate, leaving the controller end isolated
until the correct node firmware and power arrangement are verified.

Proposed output assignment (verified against manufacturer header table and
installed RUI3 4.2.4 variant.h on Lin, not yet physically connected):

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

Central RAK remains powered/connected to laptop through RAKDAP1. Pen-node
hardware uses the user-verified HW-131 rails: 3.3 V logic and 5 V driver,
approximately +/-0.01 V reported. USB powers HW-131; barrel jack unused.
No external 3.3 V is to be paralleled with the central programmer supply.

Pending: resistor installation/all-OFF test, explicit output initialization,
separate indicator-test firmware build, correct target identification and
backup before programming the pen node, then GPIO-driven state verification.
Driver EN control and measured whole-node OFF current remain separate work.
Dark LED does not establish zero current. Radio and host integration remain
unimplemented in the current recorded harness.
