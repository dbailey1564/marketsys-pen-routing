"""Enter the controller's existing RUI bootloader over UART; no image upload."""
from pathlib import Path
import time
import serial

ports = list(Path('/dev/serial/by-id').glob('usb-ARM_DAPLink_CMSIS-DAP_*-if01'))
if len(ports) != 1:
    raise SystemExit('Expected one DAPLink UART')
with serial.Serial(str(ports[0]), 115200, timeout=0.1, exclusive=True) as port:
    port.write(b'\r\n')
    time.sleep(0.2)
    port.reset_input_buffer()
    print('TX: AT+BOOT (bootloader entry only)', flush=True)
    port.write(b'AT+BOOT\r\n')
    port.flush()
    deadline = time.monotonic() + 2
    reply = bytearray()
    while time.monotonic() < deadline:
        reply.extend(port.read(port.in_waiting or 1))
    print(reply.decode('ascii', errors='replace').strip(), flush=True)
