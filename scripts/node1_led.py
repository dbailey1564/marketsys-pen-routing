"""Node1 bench LED commands. Run using the marketsys-uart-venv Python on Lin."""
import argparse
from pathlib import Path
import time
import serial

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('action', choices=['status', 'off', 'blue', 'red', 'green', 'verify'])
args = parser.parse_args()
ports = list(Path('/dev/serial/by-id').glob('usb-ARM_DAPLink_CMSIS-DAP_*-if01'))
if len(ports) != 1:
    raise SystemExit(f'Expected one DAPLink UART, found {len(ports)}')

with serial.Serial(str(ports[0]), 115200, timeout=0.1, exclusive=True) as port:
    def read_for(seconds):
        result = bytearray()
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            result.extend(port.read(port.in_waiting or 1))
        return result.decode('ascii', errors='replace')

    def command(text, expected='OK'):
        port.reset_input_buffer()
        print('TX:', text, flush=True)
        port.write((text+'\r\n').encode('ascii'))
        port.flush()
        reply = read_for(0.8)
        print(reply.strip(), flush=True)
        if expected not in reply:
            raise RuntimeError(f'Unexpected reply to {text}')
        return reply

    command('AT')
    version = command('AT+VER=?')
    # AT+VER reports the RUI SDK version, not firmwareVersion.set().
    identity = command('ATC+LED=?')
    if 'NODE1_LED=' not in identity or 'LIMIT_MS=10000' not in identity:
        raise RuntimeError('Connected target does not report the Node1 LED test command')
    if args.action == 'verify':
        try:
            command('ATC+LED=OFF')
            assert 'NODE1_LED=OFF;PB3=0;PB4=0;PB5=0' in command('ATC+LED=?')
            for color, state in [('BLUE', 'PB3=1;PB4=0;PB5=0'),
                                 ('RED', 'PB3=0;PB4=1;PB5=0'),
                                 ('GREEN', 'PB3=0;PB4=0;PB5=1')]:
                assert state in command('ATC+LED='+color)
                assert 'NODE1_LED='+color+';'+state in command('ATC+LED=?')
            print('Waiting for the 10-second automatic OFF...', flush=True)
            print(read_for(10.5).strip(), flush=True)
            assert 'NODE1_LED=OFF;PB3=0;PB4=0;PB5=0' in command('ATC+LED=?')
            command('ATC+LED=BLUE')
            command('ATC+LED=INVALID', 'ERROR')
            assert 'NODE1_LED=OFF;PB3=0;PB4=0;PB5=0' in command('ATC+LED=?')
            print('PASS: GPIO readback, exclusive colors, timeout, invalid-command OFF', flush=True)
        finally:
            command('ATC+LED=OFF')
    elif args.action == 'status':
        command('ATC+LED=?')
    else:
        command('ATC+LED='+args.action.upper())
