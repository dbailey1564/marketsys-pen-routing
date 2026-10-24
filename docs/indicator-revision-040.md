# Indicator revision 0.4.0: prepared, not uploaded

October 24, 2026. User explicitly requested a backup of the current Node1
iteration before the revised firmware upload, readback verification and LoRa
tests. The DAPLink is currently on central; a physical transfer is required
before Node1 can be freshly backed up. No board has been reflashed for this
revision yet. Previous backups remain preserved.

## Intended state table

| State | Wire value | Indication |
| --- | --- | --- |
| OFF | 0x00 | All off |
| ACTIVE | 0x01 | Solid green |
| NEXT | 0x02 | Solid blue |
| HOLD_OPEN | 0x04 | Flashing green |
| FULL | 0x05 | Solid red |
| FAULT | 0x06 | Flashing red |

STANDBY is removed. Its former wire value 0x03 is rejected, including packets
with otherwise valid CRC. It is not reassigned. Central's command parser and
the host radio helper no longer offer STANDBY. Nodes reject its retired value.
Remaining wire IDs and radio frame format are unchanged.

500 ms on/off phases and the 10-second bench cutoff remain in place. Startup
is OFF; one-color switching, ACK/retry and duplicate handling remain in place.
No automatic business-state transition or production failure policy is added.
The controller's one-shot lost-ACK control remains default-disabled.

Both targets compiled and host tests passed state mapping, solid/flashing
phase boundaries, the 10-second cutoff, retired-state rejection, frame CRC,
validation and duplicate/expired/conflict handling.

Prepared Node1 0.4.0 image SHA256:
`a7ce49ceaa2dbb5d2249f3ea76a877938a4b9d788849e4b402f927088eebad25`.
Prepared central 0.4.0 image SHA256:
`30975cbc76c39566c64fe4f4fb05a2f9e379bad2b96ff871ba82cdc640c4179b`.
The existing 0.2.0 indicator harness and its original tests remain historical;
the revised patterns are implemented in the current radio applications/shared
BenchPattern header, not retroactively applied to old test records.

## Required deployment order

1. Switch off breadboard power and unplug DAPLink USB.
2. Disconnect breadboard 3.3 V from Node1 before attaching DAPLink 3V3.
3. Move all six programmer wires from central to Node1; leave indicator/GPIO
   and breadboard ground wiring intact, with breadboard power OFF.
4. Power Node1 only from DAPLink. Verify UID against the prior Node1 record
   (00330045 5233500d 20303938). Read current full flash twice, compare and
   retain the separate dated backup before any upload.
5. Upload Node1 0.4.0 and verify its actual application bytes by SWD readback.
6. With all power off, detach DAPLink from Node1 and restore its breadboard
   3.3 V supply; move DAPLink back to central. Avoid parallel supply outputs.
7. Update/readback-verify central 0.4.0, then test all six states over LoRa
   with acknowledgements and user visual observation.

Current status: deployment awaits the Node1 programmer transfer. Compilation
and host tests do not establish that the new indications have run on hardware.
