"""Bench radio controls on Lin; only use state commands with central attached."""
import argparse
from pathlib import Path
import re
import secrets
import time
import serial

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('action', choices=['node-status', 'central-status', 'off', 'active',
    'last', 'hold_open', 'full', 'fault', 'poll', 'poll_last', 'poll_all', 'poll_all_last', 'auction-clear', 'faults', 'roster', 'destination', 'flow-status', 'flow-retry', 'sleep', 'wake'])
parser.add_argument('--node', type=int, choices=range(1, 51), default=1, metavar='1..50')
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
        deadline = time.monotonic() + 90
        while True:
            print('TX:', text, flush=True)
            port.write((text+'\r\n').encode('ascii'))
            port.flush()
            response = ''
            until = time.monotonic() + 2
            while time.monotonic() < until:
                response += read_for(0.1)
                lines = [line.strip() for line in response.splitlines()]
                if 'OK' in lines or any(line.startswith('AT_') and 'ERROR' in line for line in lines):
                    break
            print(response, end="", flush=True)
            lines = [line.strip() for line in response.splitlines()]
            if 'OK' in lines:
                return response
            # Recovery occupies the one radio transaction. Wait for it to yield.
            if 'AT_ERROR' in lines and time.monotonic() < deadline:
                time.sleep(0.4)
                continue
            raise RuntimeError('Command failed: '+text)
    port.write(b'\r\n')
    read_for(0.2)
    command('AT')
    node = args.action == 'node-status'
    response = command('ATC+NODE=?' if node else 'ATC+PEN=?')
    role = f'NODE{args.node}' if node else 'CENTRAL'
    if 'MS_RADIO='+role+';READY=1;FREQ=915000000;' not in response:
        raise RuntimeError('Wrong firmware role or radio initialization failed')
    if args.action == 'flow-status':
        command('ATC+FLOW=?')
    elif args.action in ('destination', 'flow-retry'):
        # An accepted DEST is a new bench head. Never automatically resend it
        # after a lost/ambiguous reply; query FLOW instead.
        result = command(f'ATC+DEST={args.node}' if args.action == 'destination' else 'ATC+FLOW=RETRY')
        terminal = re.compile(r'MS_FLOW=(DONE|PAUSED);')
        end = time.monotonic() + 180
        while not terminal.search(result) and time.monotonic() < end:
            more = read_for(0.2)
            if more:
                print(more, end="", flush=True)
                result += more
        match = terminal.search(result)
        if not match or match[1] != 'DONE':
            raise RuntimeError('Flow is unresolved; query FLOW, do not repeat destination as a retry')
        print('PASS: controller-owned advancement complete', flush=True)
    elif args.action in ('sleep', 'wake'):
        token = args.token if args.token is not None else secrets.randbelow(0xffffffff) + 1
        if not 1 <= token <= 0xffffffff:
            raise ValueError('Token must be 1..4294967295')
        result = command(f'ATC+POWER={token}:{args.node}:{args.action.upper()}')
        terminal = re.compile(rf'MS_POWER_NODE={args.node};TOKEN={token};RESULT=(ACK|REJECTED|FAILED);MODE=(SLEEP|AWAKE|UNKNOWN)')
        end = time.monotonic() + (305 if args.action == 'wake' else 10)
        while not terminal.search(result) and time.monotonic() < end:
            more = read_for(0.2)
            if more:
                print(more, end='', flush=True)
                result += more
        match = terminal.search(result)
        if not match or match[1] != 'ACK':
            raise RuntimeError(f'Node{args.node} power change unconfirmed: ' + (match[1] if match else 'host timeout'))
        print(f'PASS: Node{args.node} confirmed {match[2]}', flush=True)
    elif args.action == 'faults':
        command('ATC+FAULTS=?')
    elif args.action == 'roster':
        command('ATC+ROSTER=?')
    elif args.action == 'auction-clear':
        result = command('ATC+AUCTION=CLEAR')
        terminal = re.compile(r'MS_RECONCILE_DONE=NEW_AUCTION;FAULTS=(\d+);')
        end = time.monotonic() + 3600
        while not terminal.search(result) and time.monotonic() < end:
            more = read_for(0.2)
            if more:
                print(more, end="", flush=True)
                result += more
        match = terminal.search(result)
        if not match:
            raise RuntimeError('New-auction clearing is not confirmed complete')
        if int(match[1]):
            raise RuntimeError(f'New auction saved, but {match[1]} nodes remain unconfirmed; inspect controller faults')
        print('PASS: new-auction OFF confirmed for configured nodes', flush=True)
    elif args.action == 'poll_last':
        command(f'ATC+POLL_LAST={args.node}')
    elif args.action == 'poll_all_last':
        result = command('ATC+POLL_ALL_LAST=?')
        end = time.monotonic() + 5
        while 'MS_CACHE_ALL=DONE;COUNT=50' not in result and time.monotonic() < end:
            more = read_for(0.2)
            if more:
                print(more, end="", flush=True)
                result += more
        if 'MS_CACHE_ALL=DONE;COUNT=50' not in result:
            raise RuntimeError('Incomplete cached-state report')
    elif args.action in ('poll', 'poll_all'):
        token = args.token if args.token is not None else secrets.randbelow(0xffffffff) + 1
        if not 1 <= token <= 0xffffffff:
            raise ValueError('Token must be 1..4294967295')
        all_nodes = args.action == 'poll_all'
        result = command(f'ATC+POLL_ALL={token}' if all_nodes else f'ATC+POLL={token}:{args.node}')
        terminal = re.compile(r'MS_POLL_ALL=DONE;OK=(\d+);FAILED=(\d+);COUNT=50' if all_nodes else
            r'MS_POLL_NODE='+str(args.node)+r';TOKEN='+str(token)+r';RESULT=(OK|TIMEOUT|BOOTING);')
        end = time.monotonic() + (480 if all_nodes else 10)
        while not terminal.search(result) and time.monotonic() < end:
            more = read_for(0.2)
            if more:
                print(more, end="", flush=True)
                result += more
        match = terminal.search(result)
        if not match:
            raise RuntimeError('Poll did not complete within host timeout')
        if all_nodes:
            print(f'COMPLETE: {match[1]} responding, {match[2]} unreachable; no states changed', flush=True)
        elif match[1] != 'OK':
            raise RuntimeError(f'Node{args.node} did not respond; cached state was not replaced')
        else:
            print(f'PASS: Node{args.node} live state received without changing its indication', flush=True)
    elif args.action not in ('node-status', 'central-status'):
        token = args.token if args.token is not None else secrets.randbelow(0xffffffff) + 1
        if not 1 <= token <= 0xffffffff:
            raise ValueError('Token must be 1..4294967295')
        result = command(f'ATC+PEN={token}:{args.node}:{args.action.upper()}')
        end = time.monotonic() + 9
        terminal = re.compile(r'MS_LINK=(ACK|REJECTED|FAILED);TOKEN='+str(token)+r';')
        while not terminal.search(result) and time.monotonic() < end:
            more = read_for(0.2)
            if more:
                print(more, end="", flush=True)
                result += more
        match = terminal.search(result)
        if not match or match[1] != 'ACK':
            raise RuntimeError(f'State not confirmed by Node{args.node}: '+(match[1] if match else 'host timeout'))
        if f'MS_TARGET_NODE={args.node};' not in result:
            raise RuntimeError('Controller reported a different target node')
        print(f'PASS: Node{args.node} acknowledged '+args.action.upper(), flush=True)
