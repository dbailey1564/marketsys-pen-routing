from pathlib import Path
import sys,time,serial,re
node=int(sys.argv[1]);assert node in (1,2,3)
ports=list(Path('/dev/serial/by-id').glob('usb-ARM_DAPLink_CMSIS-DAP_*-if01'));assert len(ports)==1
with serial.Serial(str(ports[0]),115200,timeout=0.05,exclusive=True) as port:
 # Drain programmer-buffered output and synchronize before requesting a fresh boot.
 port.write(b'\r\n');port.flush();drain=time.monotonic()+1
 while time.monotonic()<drain:port.read(port.in_waiting or 1)
 port.write(b'AT\r\n');port.flush();probe=bytearray();limit=time.monotonic()+10
 while time.monotonic()<limit:
  probe.extend(port.read(port.in_waiting or 1))
  if b'OK' in probe:break
 assert b'OK' in probe, 'Node did not answer synchronization AT'
 port.reset_input_buffer();port.write(b'ATZ\r\n');port.flush();data=bytearray();end=time.monotonic()+30
 while time.monotonic()<end:
  data.extend(port.read(port.in_waiting or 1))
  if b'MS_BOOT_TEST=DONE;STATE=OFF' in data:break
 text=data.decode('ascii',errors='replace');print(text,flush=True)
 expected=['ACTIVE','LAST','HOLD_OPEN','FULL','FAULT']
 steps=re.findall(r'MS_BOOT_TEST=STEP;STATE=([A-Z_]+)',text)
 assert steps==expected and 'MS_BOOT_TEST=DONE;STATE=OFF' in text, 'Complete boot sequence not observed'
 port.reset_input_buffer();port.write(b'ATC+NODE=?\r\n');port.flush();data=bytearray();end=time.monotonic()+2
 while time.monotonic()<end:data.extend(port.read(port.in_waiting or 1))
 text=data.decode('ascii',errors='replace');print(text,flush=True)
 assert f'MS_RADIO=NODE{node};READY=1;' in text and 'FW=0.6.0' in text
 assert f'MS_NODE_ID={node};MAX_NODES=50' in text
 assert 'MS_NODE_STATE=OFF;PB3=0;PB4=0;PB5=0;' in text
 assert 'LIMIT_MS=0;PHASE_MS=500;BOOT_TEST=0' in text
 print(f'PASS: Node{node} identity/version/radio, complete startup sequence, final OFF, no normal cutoff',flush=True)
