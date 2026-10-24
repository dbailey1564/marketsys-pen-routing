# Penning-application interface: reconciled cross-thread record

October 24, 2026. The user clarified that the application engineer holds the
source and will implement the interface we provide. We do not require access
to the application source or compatibility with its existing binary format.
Earlier notes that made framing decisions contingent on that inspection are
superseded. No host protocol implementation or firmware upload occurs here.
Latest clarification: the app reports each head's destination as that head
enters the penning alley after sale. The new full frame review proposal is
host-frame-proposal-v2.md; its byte layout is not yet approved or implemented.

## Recovered earlier packet proposal

The first pass through local repository notes missed a concrete earlier host
frame example. Direct retrieval of Prototyping MS returned only five recent
exchanges with no older-page cursor. Related chat **Update Pen Indicator V1
state table** (01a16401-7f1d-7430-a255-cf3a7c9f7cb5), created from Prototyping
MS, contains the missing example in turn 01a16449-088e-7c73-a1a1-ab3b66054adc.
The user asked: "show binary output with header, transid, payload, etc".

The recovered assistant example explicitly calls its framing and CRC proposed,
not finalized. It sends sale 3's state updates: pen 10 -> NEXT (now called LAST,
same code 0x02) and pen 20 -> ACTIVE (0x01):

```text
AA 55 01 01 09 00 03 00 02 0A 00 02 14 00 01 92 BD
```

| Field | Bytes | Meaning |
| --- | --- | --- |
| Sync | AA 55 | Start marker |
| Version | 01 | Proposed version 1 |
| Type | 01 | Proposed SET_STATES |
| Payload length | 09 00 | 9 bytes, includes transaction ID and record count |
| Transaction ID | 03 00 | Transaction 3 |
| Record count | 02 | Two pen-state records |
| Record 1 | 0A 00 02 | Pen 10 -> old NEXT/current LAST |
| Record 2 | 14 00 01 | Pen 20 -> ACTIVE |
| CRC16 | 92 BD | 0xBD92, low byte first |

All multibyte fields are little-endian. Candidate CRC-16/CCITT-FALSE uses
poly 0x1021, init 0xFFFF, no reflection/final XOR, covers VERSION through the
last payload byte and excludes SYNC. Recalculation with Python binascii.crc_hqx
confirmed 0xBD92 and the 17-byte frame. This is a real prior proposal, not a
newly invented frame. No evidence in the recovered turn freezes it as final.

The current controller-owned assignment flow supersedes its SET_STATES payload
for normal app advancement. The header/transaction/length/CRC structure is a
recoverable candidate; a new assignment payload and its type/identity semantics
still require specification. Do not reintroduce app-managed ACTIVE/LAST updates
just because that older example encoded state records.

## Recorded agreements and proposals

- Binary, host-agnostic communication over a USB virtual COM connection.
- Application-to-controller assignment events identify the next ACTIVE pen.
  The controller derives ACTIVE, LAST, HOLD_OPEN and OFF changes; the app
  does not send separate ACTIVE and LAST instructions for normal advancement.
- Cattle, buyer and routing metadata are not required for the node protocol.
- An assignment/event identity is proposed to distinguish a genuinely new
  same-pen assignment from a retransmission; exact representation is not fixed.
- Replies should distinguish commanded state from last-node-confirmed state.
- POLL, POLL_LAST, POLL_ALL and POLL_ALL_LAST are earlier planned polling
  commands, not implemented/frozen host message IDs. In those old names,
  LAST refers to previously recorded information, not the renamed LAST pen role.
- A concrete header/framing/CRC and SET_STATES example exists above. Its values
  were proposed, not frozen. Assignment message ID/payload, session handling,
  deduplication and response codes remain open.

## Latest logical assignment payload

The current assignment-driven contract establishes a logical event. A revised
serialized frame is now proposed for review in host-frame-proposal-v2.md; it
is not frozen. The earlier SET_STATES candidate is above.
For illustration only, a new assignment can be shown
as message kind ASSIGN_PEN, event ID 1042, pen number 14. Those field names
and that command name are explanatory placeholders, not previously approved
numeric codes or a new finalized packet definition.

If 12 is active, that new event selects 14 ACTIVE and 12 LAST, clearing an
older LAST according to pen-flow-logic.md. A distinct new event naming 14
again produces HOLD_OPEN. A retransmission of event 1042 does not advance
the flow. Exact session/event deduplication is still to be specified.

## Separate implemented radio frame

The implemented 14-byte MS/version/type/node/state/result/token/CRC frame is
controller-to-node LoRa transport, not the penning application's USB packet.
It does not establish our host framing or assignment-event contract. Bench
ATC+PEN controls also remain separate from the intended binary host interface.
