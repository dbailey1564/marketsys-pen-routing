# Indicator revision 0.4.0: deployment in progress

October 24, 2026. User explicitly requested a backup of the current Node1
iteration before the revised firmware upload, readback verification and LoRa
tests. The DAPLink has been transferred to Node1 and a fresh backup completed
before uploading. Previous backups remain preserved. See the deployment
record below for completed steps and remaining hardware validation.

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

## Deployment record

Node1's connected radio image reported 0.3.0, READY=1 and state OFF. Its UID
matched 00330045 5233500d 20303938. Before uploading, two complete 262144-byte
flash reads matched. Backup directory on Lin:
`/home/dbailey1564/Development/Backups/marketsys/proto-node1-v1/20261024T220547.954762Z/`.
SHA256 of both images:
`b5092d22a00826bcb4b5f16ec0446ef0f81c3ce5331d200c73d533869c4dd4d5`.
This also matches the earlier 0.3.0 post-upload snapshot. The original current
iteration is retained privately outside Git, including its then-current main
flash settings; restore remains untested. Log: build/node1-pre-040-backup.log.

The official RAK UART uploader completed the Node1 0.4.0 transfer. Runtime
status reported FW=0.4.0, READY=1, state OFF and all GPIO LOW. Full flash
readback record:
`20261024T221033.524887Z-post-radio-node-upload/verification.json` in Node1's
private backup area. All 175512 application bytes matched the prepared image.
Bootloader code and activation marker remained unchanged outside the reviewed
update-record range. Snapshot SHA256:
`0848ade0c31d89d93ce3e9252a4e7542977c1637fd3c281b9ec17ba6d126b98d`.
The update record and settings changed from the factory reference; settings
preservation is not claimed. The pre-revision current-iteration backup above
is retained separately and was completed before this upload.

Central remains on 0.3.1 until the programmer is returned. No new LoRa pattern
is claimed verified yet. The historical LED harness remains unchanged.
