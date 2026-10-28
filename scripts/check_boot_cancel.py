"""Restart a 0.4.3 node, cancel its boot lights via UART OFF, and verify it stays OFF."""
import argparse
from pathlib import Path
import re
import time
import serial
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--node',type=int,choices=range(1,51),required=True)
args=parser.parse_args()
ports=list(Path('/dev/serial/by-id').glob('usb-ARM_DAPLink_CMSIS-DAP_*-if01'))
if len(ports)!=1:raise SystemExit('Expected one DAPLink UART')
with serial.Serial(str(ports[0]),115200,timeout=.02,exclusive=True) as port:
    port.write(b'\r\n');time.sleep(.1);port.reset_input_buffer()
    port.write(b'ATZ\r\n');port.flush()
    began=time.monotonic();buffer='';sent=False;cancelled=False;confirmed=False;stopped=None;next_query=0
    identity=False
    while time.monotonic()-began<18:
        buffer+=port.read(port.in_waiting or 1).decode('ascii',errors='replace')
        while '\n' in buffer:
            line,buffer=buffer.split('\n',1);line=line.strip()
            if not line:continue
            print(f'{time.monotonic()-began:07.3f} {line}',flush=True)
            if f'MS_RADIO=NODE{args.node};READY=1;' in line and 'FW=0.4.3' in line:identity=True
            if line=='MS_BOOT_TEST=STEP;STATE=ACTIVE' and not sent:
                port.write(b'ATC+NODE=OFF\r\n');port.flush();sent=True
            if line=='MS_BOOT_TEST=CANCELLED;REASON=COMMAND':cancelled=True;stopped=time.monotonic()
            if cancelled:
                assert not line.startswith('MS_BOOT_TEST=STEP;'), 'Boot test resumed after OFF'
                if line.startswith('MS_NODE_STATE='):
                    assert re.fullmatch(r'MS_NODE_STATE=OFF;PB3=0;PB4=0;PB5=0;TOKEN=0;LIMIT_MS=10000;PHASE_MS=500;BOOT_TEST=0',line)
                    confirmed=True
        now=time.monotonic()
        if cancelled and now>=next_query:
            port.write(b'ATC+NODE=?\r\n');port.flush();next_query=now+.2
        if stopped is not None and now-stopped>=2.5:break
    assert identity and sent and cancelled and confirmed, 'Cancellation not confirmed'
    print('PASS: UART OFF cancelled startup immediately; GPIO remained LOW beyond next step; normal timing restored')
