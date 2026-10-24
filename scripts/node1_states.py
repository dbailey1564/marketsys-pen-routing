"""Command/verify Node1's seven-state bench firmware (activates actual LEDs)."""
import argparse
from pathlib import Path
import re
import time
import serial

STATES = ['OFF', 'ACTIVE', 'NEXT', 'STANDBY', 'HOLD_OPEN', 'FULL', 'FAULT']
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('action', choices=['status', 'verify'] + [s.lower() for s in STATES])
args = parser.parse_args()
ports = list(Path('/dev/serial/by-id').glob('usb-ARM_DAPLink_CMSIS-DAP_*-if01'))
if len(ports) != 1:
    raise SystemExit(f'Expected one DAPLink UART, found {len(ports)}')

with serial.Serial(str(ports[0]), 115200, timeout=0.05, exclusive=True) as port:
    def receive(seconds):
        data = bytearray()
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            data.extend(port.read(port.in_waiting or 1))
        return data.decode('ascii', errors='replace')

    def command(text, expected='OK'):
        port.reset_input_buffer()
        port.write((text+'\r\n').encode('ascii'))
        port.flush()
        reply = receive(0.18)
        print('TX:', text, '\n'+reply.strip(), flush=True)
        lines = [line.strip() for line in reply.splitlines()]
        if expected == 'OK' and 'OK' not in lines:
            raise RuntimeError('Command failed: '+text)
        if expected != 'OK' and expected not in reply:
            raise RuntimeError('Expected command rejection: '+text)
        return reply

    def status(text):
        led = re.search(r'NODE1_LED=(\w+);PB3=(\d);PB4=(\d);PB5=(\d)', text)
        state = re.search(r'NODE1_STATE=(\w+);ELAPSED_MS=(\d+);PHASE_MS=500;FW=0.2.0', text)
        if not led or not state:
            raise RuntimeError('Expected seven-state v0.2.0 firmware response')
        color, *bits = led.groups()
        vector = tuple(map(int, bits))
        expected = {'OFF': (0, 0, 0), 'BLUE': (1, 0, 0),
                    'RED': (0, 1, 0), 'GREEN': (0, 0, 1)}
        if vector != expected.get(color):
            raise RuntimeError('GPIO readback disagrees with color or multiple outputs active')
        return state[1], int(state[2]), color

    # Terminate any partial command left by startup/reconnection before handshake.
    port.write(b'\r\n')
    receive(0.2)
    command('AT')
    status(command('ATC+STATE=?'))
    if args.action == 'verify':
        try:
            command('ATC+STATE=OFF')
            if status(command('ATC+STATE=?')) != ('OFF', 0, 'OFF'):
                raise RuntimeError('OFF state failed')
            patterns = {'ACTIVE': ('GREEN', 'OFF'), 'NEXT': ('BLUE', 'OFF'),
                        'STANDBY': ('BLUE', 'BLUE'), 'HOLD_OPEN': ('GREEN', 'BLUE'),
                        'FULL': ('RED', 'RED'), 'FAULT': ('RED', 'OFF')}
            for name, phases in patterns.items():
                command('ATC+STATE='+name)
                seen = set()
                end = time.monotonic() + 2.1
                while time.monotonic() < end:
                    state, elapsed, color = status(command('ATC+STATE=?'))
                    if state != name:
                        raise RuntimeError('State changed unexpectedly')
                    # Skip samples immediately next to a switching boundary.
                    if 25 < elapsed % 500 < 475:
                        phase = (elapsed // 500) % 2
                        if color != phases[phase]:
                            raise RuntimeError(f'{name}: wrong color at {elapsed} ms')
                        seen.add(color)
                if seen != set(phases):
                    raise RuntimeError('Incomplete phase coverage: '+name)
                print('PASS pattern:', name, flush=True)
            command('ATC+STATE=HOLD_OPEN')
            print(receive(10.3).strip(), flush=True)
            if status(command('ATC+STATE=?')) != ('OFF', 0, 'OFF'):
                raise RuntimeError('Automatic OFF failed')
            command('ATC+STATE=FULL')
            command('ATC+STATE=INVALID', 'AT_PARAM_ERROR')
            if status(command('ATC+STATE=?')) != ('OFF', 0, 'OFF'):
                raise RuntimeError('Invalid-command OFF failed')
            print('PASS: all seven states, phases, GPIO exclusivity, timeout, invalid input', flush=True)
        finally:
            command('ATC+STATE=OFF')
    elif args.action == 'status':
        pass
    else:
        command('ATC+STATE='+args.action.upper())
