# Questions for the penning-application engineer

Open questions for interface review. These are not finalized protocol choices.

## 1. Event identity: persistent sequence or session plus sequence?

Can the app maintain one persistent, non-reused head-entry event counter
across application restarts, or would a session identifier plus a transaction
counter within each session be more appropriate?

Here, session means an application event-stream namespace, not an auction,
buyer, sale or animal identifier. Its purpose is to distinguish reused
transaction numbers after restart from retries of older events.

Please weigh in on:

- Whether a persistent event counter fits the app's existing storage and
  recovery model, and what counter width/wrap behavior is appropriate.
- If using sessions, how session IDs are generated and retained, and how
  pending events keep their original identities after restart/reconnection.
- How the app and controller recover the last accepted event after either
  side restarts, so a retry cannot be mistaken for another head.

Required outcome: a retransmission never advances pen flow or triggers
HOLD_OPEN; a genuinely new head going to the same pen does. Reconnection
alone must not clear ACTIVE/LAST state. The proposed separate session field
is not approved or mandatory pending this decision.

## Related destination proposal to retain for frame review

The user proposes low byte = pen number (up to 255), high byte reserved for
an alphabetic designation. Suggested encoding, not yet approved: high byte
00 for no letter, otherwise uppercase ASCII A-Z. For example, pen 20B would
be transmitted as 14 42 hex, low byte first. This changes interpretation of
the two-byte destination, not its size. See host-frame-proposal-v2.md.
