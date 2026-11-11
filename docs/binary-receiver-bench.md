# Controller binary receiver bench mode

Controller 0.7.3 accepts the 16-byte version 02 HEAD_DESTINATION frame in
interface-protocol-specification.md. Node firmware and LoRa framing are unchanged.

The controller starts in its normal AT console at 115200 baud, 8N1. Wait for
startup assessment and any auction clear to finish. To bind an auction session
and switch the same UART to raw mode, send ATC+HOST=<decimal session ID>.
Wait for MS_HOST=READY before sending any frame. The Windows test sender's
--session option performs this setup automatically. Omit --session on
subsequent sends until the controller restarts. Restart to return to the AT
console. A different session can bind after a new auction clear with both
routing roles empty. Use a new nonzero session ID for each auction.

Within the bound session, send progressive nonzero transaction IDs in order.
Wait for the terminal flow result before sending a new event. An exact repeat
of the most recently accepted ID and destination reports DUPLICATE without
advancing. Reusing that ID with another destination reports CONFLICT. Older IDs
report STALE. Acceptance commits the event identity and routing intent in the
same flash record. The same session can be armed again after a restart if the
controller is still in that auction. Older MSR1 and MSR2 records can be loaded.

Only configured numeric node addresses 1–50 with alpha byte zero can route in
this build. A valid event emits an ASCII MS_HOST=ACCEPTED diagnostic after
saving intent. MS_FLOW=DONE or MS_FLOW=PAUSED reports the subsequent routing
outcome. Binary reply frames remain to be defined. CRC failure, an unknown
session, and an unsupported destination leave routing unchanged.

Use the verified application backup and its matching journal together if a
rollback to a controller release older than 0.7.3 is required. Such firmware
cannot load the new MSR3 record format.
