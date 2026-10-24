# App-to-controller head-destination frame: review draft

October 24, 2026. This is a proposal for review, not a frozen specification or
implemented host protocol. No firmware was uploaded. The user confirmed the
app should report each head's destination after its sale as it enters the
penning alley. It does not direct indicator-state changes.

## Confirmed business boundary

One newly entering head produces one destination event, even when successive
heads share the same destination. The controller owns ACTIVE/LAST/HOLD_OPEN/
OFF logic and LoRa delivery. Buyer, lot, animal ID, cattle attributes, head
count and explicit indicator-state fields are not required in this message.
The event ID below is transport identity, not an animal identification number.
Sending the event at sale-recording time and sending it at alley entry are
not interchangeable unless the user's operational process makes them so.

## Proposed 18-byte HEAD_DESTINATION frame

All multibyte integers are unsigned little-endian. IDs and field widths below
are proposals. Version 0x02 distinguishes this draft from the recovered older
version-1 SET_STATES proposal; type 0x10 is a proposed allocation.

| Offset | Bytes | Field | Proposed rule |
| --- | --- | --- | --- |
| 0-1 | 2 | SYNC | AA 55 |
| 2 | 1 | VERSION | 02 |
| 3 | 1 | TYPE | 10 = HEAD_DESTINATION |
| 4-5 | 2 | PAYLOAD_LENGTH | 2 for this message; excludes header and CRC |
| 6-9 | 4 | SESSION_ID | Identifies an application event stream |
| 10-13 | 4 | TRANSACTION_ID | Identifies this head-entry event within the stream |
| 14-15 | 2 | DESTINATION_PEN | Numeric pen number; 0 reserved/invalid |
| 16-17 | 2 | CRC16 | Low byte first; covers bytes 2-15 |

Header length is 14 bytes; business payload is 2; CRC is 2. No record count is
needed because this message describes exactly one head-entry event.
Pen numbers are application destinations mapped by the controller to device
addresses, not necessarily LoRa node IDs. The proposed encoding assumes numeric
pen numbers 1..65535; confirm that identifier model before freezing it.

CRC candidate is CRC-16/CCITT-FALSE: width 16, polynomial 0x1021, initial value
0xFFFF, no input/output reflection, xorout 0. SYNC and the CRC field itself
are excluded. Reference check value for ASCII 123456789 is 0x29B1.

## Worked example

Session 0x12345678, transaction 3, a head destined for pen 20:

```text
AA 55 | 02 | 10 | 02 00 | 78 56 34 12 | 03 00 00 00 | 14 00 | ED 93
SYNC    VER  TYPE LENGTH   SESSION       TRANSACTION    PEN     CRC
```

The CRC over `02 10 02 00 78 56 34 12 03 00 00 00 14 00` was recalculated
using Python binascii.crc_hqx with initial 0xFFFF: CRC 0x93ED, transmitted ED 93.
Total frame length was checked as 18 bytes. This example is newly proposed,
unlike the recovered 17-byte SET_STATES frame, which remains historical.

## Proposed acceptance and replay behavior

- Same session/transaction and same destination: retransmission; return the
  recorded outcome without advancing flow or restarting the indication.
- New transaction with the same destination as active: another head; apply
  the confirmed HOLD_OPEN rule.
- Same session/transaction with different destination: conflict; reject.
- Malformed version/type/length, invalid destination or CRC: do not advance.
- A radio resend within central is not a new application head-entry event.

Transactions should be ordered within a stream. The app must retain pending
events' original identities across retries/reconnection; it must not give a
retry a new identity. Session allocation/negotiation, counter wrap, persistent
deduplication after controller reset and old-session replay handling remain
open and must be settled before implementation. A new session must not
implicitly clear the pen displays. The fields alone do not solve reset/replay.

## Framing and replies to specify next

AA55 may appear inside IDs, payload or CRC. A decoder must use length and CRC,
not terminate at another AA55 inside a frame. Proposed safety bound for any
future payload is 256 bytes; this message requires exactly 2. Buffering,
resynchronization and incomplete-frame timeouts still need exact definitions.

The reply protocol must correlate session and transaction and distinguish
controller acceptance from node-confirmed application/failure. Those reply
message IDs, payloads, retry timeouts, error codes and queue/backpressure rules
are not finalized here. Host receipt is not proof of physical illumination.
This USB protocol remains distinct from the implemented LoRa frame and AT
bench commands. No automatic business transition is driven by CRC/header data.
