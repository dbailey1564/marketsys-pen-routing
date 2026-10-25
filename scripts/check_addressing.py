"""Check the connected 0.4.2 controller's address parser; sends only OFF to node 50."""
from pathlib import Path
import re
import secrets
import time
import serial
ports = list(Path('/dev/serial/by-id').glob('usb-ARM_DAPLink_CMSIS-DAP_*-if01'))
if len(ports) != 1: raise SystemExit('Expected one DAPLink UART')
with serial.Serial(str(ports[0]), 115200, timeout=0.05, exclusive=True) as port:
    def read(seconds):
        end = time.monotonic()+seconds; data = bytearray()
        while time.monotonic() < end: data.extend(port.read(port.in_waiting or 1))
        return data.decode('ascii', errors='replace')
    def command(text):
        port.write((text+'\r\n').encode()); port.flush()
        reply = read(.6); print('TX:',text); print(reply.strip()); return reply
    port.write(b'\r\n'); read(.2)
    baseline = command('ATC+PEN=?')
    assert 'MS_RADIO=CENTRAL;READY=1;' in baseline and 'FW=0.4.2' in baseline
    token = re.search(r'MS_LINK=\w+;TOKEN=(\d+);',baseline)[1]
    for fields in ['123:0:OFF','123:51:OFF','123:255:OFF','123:-1:OFF',
                   '4294967296:1:OFF','0:1:OFF','123:2:STANDBY','123:2:NEXT']:
        reply = command('ATC+PEN='+fields)
        assert 'AT_PARAM_ERROR' in reply and 'MS_TX_ATTEMPT=' not in reply
        status = command('ATC+PEN=?')
        assert re.search(r'MS_LINK=\w+;TOKEN=(\d+);',status)[1] == token
    new_token = secrets.randbelow(0xffffffff)+1
    reply = command(f'ATC+PEN={new_token}:50:OFF')
    assert 'MS_LINK=PENDING;' in reply and 'MS_TARGET_NODE=50;MAX_NODES=50' in reply
    end = time.monotonic()+9
    terminal = re.compile(r'MS_LINK=(ACK|REJECTED|FAILED);TOKEN='+str(new_token)+';')
    while not terminal.search(reply) and time.monotonic()<end:
        more=read(.2)
        if more: print(more.strip()); reply+=more
    match=terminal.search(reply)
    assert match and match[1]=='FAILED', 'Expected no node 50 on this bench'
    assert 'ATTEMPTS=3;' in reply and 'CONFIRMED=UNKNOWN;' in reply
    print('PASS: invalid addresses/states rejected without changing transaction; address 50 accepted, three sends, no false confirmation')
