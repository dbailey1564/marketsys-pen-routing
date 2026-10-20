"""Query RUI3 version over UART2 using Linux's standard serial APIs."""
import fcntl
import os
from pathlib import Path
import select
import termios
import time

devices = list(Path('/dev/serial/by-id').glob('usb-ARM_DAPLink_CMSIS-DAP_*-if01'))
if len(devices) != 1:
    raise SystemExit(f'Expected one DAPLink UART, found {len(devices)}')
port = devices[0]
print(f'UART: {port} at 115200 8N1; no flow control', flush=True)
fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
original = termios.tcgetattr(fd)

def receive(seconds):
    deadline = time.monotonic() + seconds
    payload = bytearray()
    while time.monotonic() < deadline:
        ready, _, _ = select.select([fd], [], [], max(0, deadline-time.monotonic()))
        if ready:
            part = os.read(fd, 4096)
            if part:
                payload.extend(part)
            else:
                break
    return bytes(payload)

try:
    fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    settings = termios.tcgetattr(fd)
    settings[0] = 0
    settings[1] = 0
    settings[2] = termios.CLOCAL | termios.CREAD | termios.CS8
    settings[3] = 0
    settings[4] = settings[5] = termios.B115200
    settings[6][termios.VMIN] = 0
    settings[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, settings)
    initial = receive(0.5)
    if initial:
        print('Initial output:', repr(initial), flush=True)
    for command in (b'AT\r\n', b'AT+VER=?\r\n'):
        print('TX:', command.decode().strip(), flush=True)
        sent = os.write(fd, command)
        if sent != len(command):
            raise RuntimeError('Incomplete serial write')
        termios.tcdrain(fd)
        reply = receive(2)
        print('RX:', repr(reply), flush=True)
finally:
    termios.tcsetattr(fd, termios.TCSANOW, original)
    os.close(fd)
