# App-to-controller head-destination frame: review draft

October 24, 2026. The current consolidated draft is the
[Interface Protocol Specification](interface-protocol-specification.md).
That document is authoritative among the current draft documents and contains
the detailed COBOL CRC procedure, complete example, and conformance vectors.
It is not a frozen protocol or an implemented firmware interface.

The current HEAD_DESTINATION draft is **16 bytes**, with no payload-length
field. Destination consists of one numeric byte and one reserved alpha byte.
The session-versus-persistent-counter choice remains open for engineer input;
see [engineer questions](engineer-questions.md). The 16-byte example retains
the proposed session and transaction fields pending that decision.

```text
AA 55 | 02 | 10 | 78 56 34 12 | 03 00 00 00 | 14 | 00 | 35 0B
SYNC    VER  TYPE SESSION       TRANSACTION    PEN  ALPHA CRC
```

CRC-16/CCITT-FALSE covers the 12 raw bytes from VERSION through ALPHA,
excluding SYNC and CRC. This example produces numeric CRC 0B35, transmitted
low byte first as 35 0B. See the consolidated specification for parameters,
COBOL positions, byte conversion, arithmetic XOR and intermediate results.

The earlier 18-byte proposal, including length bytes 02 00 and CRC ED 93,
is superseded. The recovered 17-byte SET_STATES proposal remains historical
in [the cross-thread record](host-interface-record.md); the application no
longer sends ACTIVE/LAST state instructions for normal flow.

## Proposed acceptance and replay behavior

- Same event identity and same destination: retransmission; return the
  recorded outcome without advancing flow or restarting the indication.
- New event with the same destination as active: another head; apply HOLD_OPEN.
- Same event identity with different destination: conflict; reject.
- Unsupported version/type, invalid destination or CRC: do not advance.
- A radio resend within central is not a new application head-entry event.

The app reports one head's destination as it enters the penning alley after
sale. Central owns ACTIVE/LAST/HOLD_OPEN/OFF and radio delivery. Event ordering,
identity persistence, counter wrap, reset/replay handling, replies, incomplete
frame timeouts and resynchronization remain to be finalized. A new session
must not implicitly clear the pen displays. Host acceptance is not proof of
node-confirmed application or physical illumination.
