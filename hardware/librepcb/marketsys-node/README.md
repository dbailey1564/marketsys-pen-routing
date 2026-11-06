# MarketSys standard node schematic

Open marketsys-node.lpp in LibrePCB 2.x, or open the accompanying .lppz archive.
All schematic symbols/components are embedded; no external library installation
is needed. This is an editable electrical schematic, not a PCB layout.

The original standard allocation is preserved: IC1A blue (G1/S5/D6), IC1B red
(G3/S2/D4), IC2A green (G1/S5/D6); IC2B unused. Node2's repair moved red to IC2B;
that repair is documented separately and is not imposed on every node here.

PB3=blue, PB4=red, PB5=green, each through 1k. Each gate has a 10k pull-down to
its own source/driver L-. LED positives join driver L+; negatives join the
respective drains. LED_RETURN/driver L- is distinct from supply GND. EN is tied
to IN+. Inputs are externally regulated 5 V for the driver and 3.3 V for RAK.
J2 is a logical six-wire programming interface, not a claim about the physical
pin order of a particular programmer connector. Do not parallel its 3.3 V feed
with another powered source. Attach the antenna before powering the module.

RAK is the existing breakout assembly, shown with its used functional signals;
it is not a bare-module land pattern. RGB die terminals are functional labels;
actual LED package pad numbering is unverified. No footprints, devices or board
are assigned. Verify module/header dimensions and LED pads before PCB layout.

The schematic does not add an unselected battery, charger, regulator or EN GPIO.
Firmware state colours and the unresolved intermittent startup observation do
not change these nets. connections.csv provides the explicit connectivity list.

Sources:
- Project docs/indicator-bench-wiring.md (original allocation plus Node2 repair).
- https://www.onsemi.com/download/data-sheet/pdf/fdc6401n-d.pdf
- https://docs.rakwireless.com/product-categories/wisduo/rak3272-sip-breakout-board/datasheet/
- https://eletechsup.com/products/adjustable-28-1500ma-output-current-high-brightness-led-constant-current-driver-board-for-flashlight-headlights-emergency-lights

Generated with native format references from LibrePCB/librepcb-example-projects.
Bundled NewStroke/FontoBene stroke font is CC0-1.0, as noted in its file header.

