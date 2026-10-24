"""Bench radio controls on Lin; only use state commands with central attached."""
import argparse
from pathlib import Path
import re
import secrets
import time
import serial

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('action', choices=['node-status', 'central-status', 'off', 'active',
    'last', 'hold_open', 'full', 'fault'])
parser.add_argument('--token', type=int, help='Explicit transaction token for duplicate tests')
args = parser.parse_args()
ports = list(Path('/dev/serial/by-id').glob('usb-ARM_DAPLink_CMSIS-DAP_*-if01'))
if len(ports) != 1:
    raise SystemExit(f'Expected one DAPLink UART, found {len(ports)}')
with serial.Serial(str(ports[0]), 115200, timeout=0.05, exclusive=True) as port:
    def read_for(seconds):
        data = bytearray()
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            data.extend(port.read(port.in_waiting or 1))
        return data.decode('ascii', errors='replace')
    def command(text):
        print('TX:', text, flush=True)
        port.write((text+'\r\n').encode('ascii'))
        port.flush()
        response = read_for(0.5)
        print(response.strip(), flush=True)
        if 'OK' not in [line.strip() for line in response.splitlines()]:
            raise RuntimeError('Command failed: '+text)
        return response
    port.write(b'\r\n')
    read_for(0.2)
    command('AT')
    node = args.action == 'node-status'
    response = command('ATC+NODE=?' if node else 'ATC+PEN=?')
    role = 'NODE1' if node else 'CENTRAL'
    if 'MS_RADIO='+role+';READY=1;FREQ=915000000;' not in response:
        raise RuntimeError('Wrong firmware role or radio initialization failed')
    if args.action not in ('node-status', 'central-status'):
        token = args.token if args.token is not None else secrets.randbelow(0xffffffff) + 1
        if not 1 <= token <= 0xffffffff:
            raise ValueError('Token must be 1..4294967295')
        result = command(f'ATC+PEN={token}:{args.action.upper()}')
        end = time.monotonic() + 9
        terminal = re.compile(r'MS_LINK=(ACK|REJECTED|FAILED);TOKEN='+str(token)+r';')
        while not terminal.search(result) and time.monotonic() < end:
            more = read_for(0.2)
            if more:
                print(more.strip(), flush=True)
                result += more
        match = terminal.search(result)
        if not match or match[1] != 'ACK':
            raise RuntimeError('State not confirmed by Node1: '+(match[1] if match else 'host timeout'))
        print('PASS: Node1 acknowledged '+args.action.upper(), flush=True)
