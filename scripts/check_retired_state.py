"""Confirm central 0.4.0 rejects removed STANDBY without starting a transaction."""
from pathlib import Path
import re
import time
import serial

ports = list(Path('/dev/serial/by-id').glob('usb-ARM_DAPLink_CMSIS-DAP_*-if01'))
if len(ports) != 1:
    raise SystemExit('Expected one DAPLink UART')
with serial.Serial(str(ports[0]), 115200, timeout=0.05, exclusive=True) as port:
    def receive(seconds):
        data = bytearray()
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            data.extend(port.read(port.in_waiting or 1))
        return data.decode('ascii', errors='replace')
    def command(text, expected='OK'):
        print('TX:', text, flush=True)
        port.write((text+'\r\n').encode('ascii')); port.flush()
        reply = receive(0.4)
        print(reply.strip(), flush=True)
        if expected not in [line.strip() for line in reply.splitlines()]:
            raise RuntimeError('Unexpected command response')
        return reply
    port.write(b'\r\n'); receive(0.2)
    command('AT')
    before = command('ATC+PEN=?')
    if 'MS_RADIO=CENTRAL;READY=1;' not in before or 'FW=0.4.0' not in before:
        raise RuntimeError('Expected central 0.4.0')
    old_link = re.search(r'MS_LINK=[^\r\n]+', before)
    if not old_link: raise RuntimeError('Missing link status')
    reply = command('ATC+PEN=1024202801:STANDBY', 'AT_PARAM_ERROR')
    if 'MS_TX_ATTEMPT' in reply: raise RuntimeError('Removed state caused a transmission')
    after = command('ATC+PEN=?')
    new_link = re.search(r'MS_LINK=[^\r\n]+', after)
    # Confirmation age naturally increases; transaction fields must be unchanged.
    if not new_link or old_link[0].split(';AGE_MS=')[0] != new_link[0].split(';AGE_MS=')[0]:
        raise RuntimeError('Removed state changed the transaction')
    print('PASS: STANDBY rejected, transaction unchanged', flush=True)
