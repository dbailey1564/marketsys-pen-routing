"""Query controller sleep settings and disable LPM for bench programming."""
from pathlib import Path
import time
import serial

ports = list(Path('/dev/serial/by-id').glob('usb-ARM_DAPLink_CMSIS-DAP_*-if01'))
if len(ports) != 1:
    raise SystemExit('Expected one DAPLink UART')
with serial.Serial(str(ports[0]), 115200, timeout=0.1, exclusive=True) as port:
    def read_for(seconds):
        end = time.monotonic() + seconds
        payload = bytearray()
        while time.monotonic() < end:
            payload.extend(port.read(port.in_waiting or 1))
        return payload.decode('ascii', errors='replace')
    port.write(b'\r\n')
    read_for(0.3)
    for command in ['AT', 'AT+LPM=?', 'AT+LPMLVL=?', 'AT+LPM=0', 'AT+LPM=?']:
        print('TX:', command, flush=True)
        port.write((command+'\r\n').encode('ascii'))
        port.flush()
        response = read_for(1)
        print(response.strip(), flush=True)
        if 'OK' not in [line.strip() for line in response.splitlines()]:
            raise RuntimeError('Command failed: '+command)
