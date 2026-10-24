# Penning-application interface: latest recorded position

October 24, 2026. The user clarified that the application engineer holds the
source and will implement the interface we provide. We do not require access
to the application source or compatibility with its existing binary format.
Earlier notes that made framing decisions contingent on that inspection are
superseded. No host protocol implementation or firmware upload occurs here.

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
- Header/framing, command IDs, field widths, byte order, length encoding,
  CRC choice/coverage, session handling and response codes are not frozen.

## Latest logical assignment payload

The current record establishes a logical assignment message, not a serialized
byte-for-byte host frame. For illustration only, a new assignment can be shown
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
