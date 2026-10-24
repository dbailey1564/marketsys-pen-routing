"""Live one-shot ACK-loss, retry and original Node1 timeout verification."""
from pathlib import Path
import re
import secrets
import time
import serial

ports = list(Path('/dev/serial/by-id').glob('usb-ARM_DAPLink_CMSIS-DAP_*-if01'))
if len(ports) != 1:
    raise SystemExit('Expected one DAPLink UART')
with serial.Serial(str(ports[0]), 115200, timeout=0.02, exclusive=True) as port:
    def receive(seconds):
        data = bytearray()
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            part = port.read(port.in_waiting or 1)
            if part:
                data.extend(part)
        reply = data.decode('ascii', errors='replace')
        if reply.strip(): print(reply.strip(), flush=True)
        return reply

    def command(text):
        print('TX:', text, flush=True)
        sent_at = time.monotonic()
        port.write((text+'\r\n').encode('ascii'))
        port.flush()
        reply = receive(0.25)
        if 'OK' not in [line.strip() for line in reply.splitlines()]:
            raise RuntimeError('Command failed: '+text)
        return reply, sent_at

    def terminal(reply, token, seconds=8):
        end = time.monotonic() + seconds
        match = re.compile(r'MS_LINK=(ACK|REJECTED|FAILED);TOKEN='+str(token)+r';')
        while not match.search(reply) and time.monotonic() < end:
            reply += receive(0.05)
        if not match.search(reply): raise RuntimeError('Missing terminal radio result')
        return reply, match.search(reply)[1]

    def fresh_token(): return secrets.randbelow(0xffffffff) + 1
    def off():
        token = fresh_token()
        reply, _ = command(f'ATC+PEN={token}:OFF')
        reply, result = terminal(reply, token)
        if result != 'ACK' or 'CONFIRMED=OFF;' not in reply:
            raise RuntimeError('Fresh OFF was not acknowledged')

    port.write(b'\r\n'); receive(0.2)
    command('AT')
    status, _ = command('ATC+PEN=?')
    supported = any('FW='+version in status for version in ('0.3.1', '0.4.0'))
    if not supported or 'MS_RADIO=CENTRAL;READY=1;' not in status:
        raise RuntimeError('Expected supported central firmware with radio ready')
    command('ATC+DROP=0')
    off()
    try:
        armed, _ = command('ATC+DROP=1')
        if 'MS_DROP_ARMED=1;' not in armed: raise RuntimeError('Drop control did not arm')
        token = fresh_token()
        reply, started = command(f'ATC+PEN={token}:ACTIVE')
        # Query during the first retry wait: a deliberately ignored ACK must
        # not update last-confirmed state or complete the transaction.
        pending, _ = command('ATC+PEN=?')
        reply += pending
        if not re.search(r'MS_LINK=PENDING;TOKEN='+str(token)+r';COMMANDED=ACTIVE;CONFIRMED=OFF;', pending):
            raise RuntimeError('Dropped ACK changed confirmation or ended the transaction')
        reply, result = terminal(reply, token)
        if result != 'ACK': raise RuntimeError('Retry did not recover the transaction')
        if len(re.findall(r'MS_TEST_ACK_IGNORED=1;TOKEN='+str(token)+r';RESULT=0;CONFIRMED=OFF', reply)) != 1:
            raise RuntimeError('Expected exactly one ignored initial APPLIED ACK')
        if re.findall(r'MS_TX_ATTEMPT=(\d+);TOKEN='+str(token), reply) != ['1', '2']:
            raise RuntimeError('Expected exactly two transmissions')
        if 'MS_ACK_RESULT=1;' not in reply or 'ATTEMPTS=2;' not in reply:
            raise RuntimeError('Second attempt did not receive DUPLICATE acknowledgement')
        print('PASS: ignored first ACK, retained previous confirmation, retry 2 duplicate ACK', flush=True)
        # Probe between the original 10-second deadline and a timer restarted
        # by the retry (~11 seconds). Same token cannot create a new command.
        while time.monotonic() < started + 10.35:
            receive(min(0.05, started + 10.35 - time.monotonic()))
        if time.monotonic() - started > 10.7:
            raise RuntimeError('Host missed the timer-discrimination window; no timing claim')
        probe, probe_at = command(f'ATC+PEN={token}:ACTIVE')
        probe, probe_result = terminal(probe, token)
        probe_done = time.monotonic() - started
        if probe_result != 'REJECTED' or 'MS_ACK_RESULT=2;' not in probe or 'CONFIRMED=OFF;' not in probe:
            raise RuntimeError('Node timer was extended or expired duplicate was not rejected')
        if probe_done >= 10.9:
            raise RuntimeError('Probe reply too late to distinguish original timeout from a restart')
        print(f'PASS: EXPIRED/OFF at probe {probe_at-started:.3f}s, reply by {probe_done:.3f}s; original timeout retained', flush=True)
    finally:
        # Leave the one-shot test control disarmed and obtain a fresh OFF ACK.
        command('ATC+DROP=0')
        off()
    print('PASS: lost-ACK test complete; test injection disabled, Node1 confirmed OFF', flush=True)
