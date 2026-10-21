# Available RF-PBTWN power banks: photographed labels

October 21, 2026. Two images retrieved from the ChatGPT conversation
"Power bank specifications" (6ad843e0-aaa0-83ea-bf81-531c3d67b086), latest
user image attachments. These are user-supplied label observations, not a
manufacturer manual or measured performance report. No brand is established
by these photographs alone.

## Direct observations

- Two enclosed banks shown, each marked capacity 5200 mAh.
- Their base label reads model RF-PBTWN.
- USB-A output: 5 V / 2.4 A, 12 W maximum.
- USB-C input: 5 V / 2 A, 10 W maximum.
- USB-C output: 5 V / 3 A, 15 W maximum.
- Combined output per bank: 5 V / 3 A, 15 W maximum.
- Base charging-input label: 1 A minimum, 2 A maximum.

The pictures do not show the previously described 13500 cells or any regulator
module. Do not equate these banks with the 3.03 Wh loose-cell proposal. Internal
nominal voltage, usable output Wh, low-load cutoff and pass-through behavior
are not established by these labels.

## Preferred next bench option

Use one existing bank per node through its USB-A 5 V output, followed by the
fuse/switch and split 5 V LED-driver / regulated 3.3 V logic rails already drawn
in thursday-usb-prototype.md. Replace the proposed purchased bank in that drawing
with the available 5200 mAh RF-PBTWN. No upstream 5 V buck is needed because
the bank already supplies 5 V. The 3.3 V regulator remains necessary.

Do not connect the banks' USB outputs in series or parallel. Keep them enclosed
and use their designated USB ports. Charge separately with the node off for
this demonstration unless the exact model's documentation establishes another
supported arrangement.

Their labeled 12 W USB-A ceiling exceeds the current 1.065 W planning load,
but this does not validate regulation, low-load behavior or runtime. If the
5200 mAh rating is at 3.7 V internally, nominal stored energy would be 19.24 Wh;
that voltage assumption is unconfirmed, and delivered USB energy will be lower.
No eight-hour runtime claim is established.

Next: identify/obtain the actual 3.3 V regulator; verify the bank's 5 V output;
test node boot, OFF-state bank shutdown and sustained LED operation. The
current record still lacks driver/LED photos for final color-switch wiring.
