# Resident node sleep mode (0.7.1 bench build)

A configured node can enter sleep only when its actual and controller-commanded
indicator states are OFF. The controller sends an addressed SLEEP request.
The node acknowledges the mode, stays receptive for five seconds for retry,
then sleeps between finite receive windows. Windows start every 120 seconds
and last five seconds. Early interrupts and unrelated packets do not move
the next opening or extend the current window.

The controller sends an addressed WAKE every two seconds until it receives a
matching awake acknowledgement or reaches 300 seconds from command acceptance.
A wake leaves the light OFF if it was OFF. Normal destination or state commands
can follow the acknowledgement. Wake transactions are serialized by the
controller, so the five-minute budget applies to one accepted wake request.
No acknowledgement is reported as unconfirmed. Wake before normal routing,
and wake nodes before an auction clear if each node must be verified live.

From the checkout on Lin, using a matching 0.7.1 controller and node image:

```sh
../Tools/marketsys-uart-venv/bin/python scripts/radio_link.py sleep --node 1
../Tools/marketsys-uart-venv/bin/python scripts/radio_link.py wake --node 1
```

Replace 1 with any configured node address. Sleep and the controller's
knowledge of sleeping nodes are volatile. A rebooted node comes up awake
and runs its normal startup light test. After a controller-only restart,
explicitly wake sleeping nodes before resolving any startup assessment block.
A sleep request with no acknowledgement may have reached the node. Resolve
that uncertainty with WAKE. Routine controller reconciliation skips nodes
believed asleep. The lamp outputs remain low; the demo driver's EN is
hardwired, so this firmware cannot remove its idle draw.

Bench acceptance is still required. Measure node supply current, verify
start-to-start timing and the usable RF window, and test wake after missing
one complete window. The timeout is a reporting deadline. Successful delivery
depends on RF reception. The installed 0.6.x fleet uses a different radio
frame version. Update the controller and participating nodes together.
