"""Restart a connected node and verify boot-test timing and GPIO reports."""
import argparse
import json
from pathlib import Path
import re
import time
import serial
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--node',type=int,choices=range(1,51),required=True)
parser.add_argument('--version', choices=['0.4.3','0.4.4'], default='0.4.4')
args=parser.parse_args()
tag=args.version.replace('.','')
ports=list(Path('/dev/serial/by-id').glob('usb-ARM_DAPLink_CMSIS-DAP_*-if01'))
if len(ports)!=1: raise SystemExit('Expected one DAPLink UART')
root=Path(__file__).resolve().parents[1]
expected=['ACTIVE','LAST','HOLD_OPEN','FULL','FAULT']
pattern=re.compile(r'MS_NODE_STATE=(\w+);PB3=(\d);PB4=(\d);PB5=(\d);TOKEN=(\d+);LIMIT_MS=(\d+);PHASE_MS=(\d+);BOOT_TEST=(\d)')
steps=[];samples=[];wait_samples=0;wait_at=None;done=None;version_ok=False;id_ok=False
with serial.Serial(str(ports[0]),115200,timeout=.02,exclusive=True) as port:
    port.write(b'\r\n');time.sleep(.1);port.reset_input_buffer()
    began=time.monotonic();port.write(b'ATZ\r\n');port.flush()
    buffer='';next_query=0;started=False;waiting=False;final=False
    while time.monotonic()-began<40:
        now=time.monotonic();chunk=port.read(port.in_waiting or 1).decode('ascii',errors='replace');buffer+=chunk
        while '\n' in buffer:
            line,buffer=buffer.split('\n',1);line=line.strip()
            if not line:continue
            elapsed=time.monotonic()-began
            print(f'{elapsed:07.3f} {line}',flush=True)
            if f'MS_RADIO=NODE{args.node};READY=1;' in line and f'FW={args.version}' in line:version_ok=True
            if f'MS_NODE_ID={args.node};MAX_NODES=50' in line:id_ok=True
            if line=='MS_BOOT_TEST=WAIT;DELAY_MS=10000':
                started=True;waiting=True;wait_at=elapsed
            if line.startswith('MS_BOOT_TEST=START;'):started=True;waiting=False
            if line.startswith('MS_BOOT_TEST=STEP;STATE='):steps.append((line.split('STATE=',1)[1],elapsed))
            if line=='MS_BOOT_TEST=DONE;STATE=OFF':done=elapsed
            match=pattern.search(line)
            if match:
                state=match[1];gpio=tuple(map(int,match.group(2,3,4)));token,limit,phase,active=map(int,match.group(5,6,7,8))
                assert sum(gpio)<=1, 'More than one color HIGH'
                allowed={'OFF':[(0,0,0)],'ACTIVE':[(0,0,1)],'LAST':[(1,0,0)],
                         'HOLD_OPEN':[(0,0,0),(0,0,1)],'FULL':[(0,1,0)],'FAULT':[(0,0,0),(0,1,0)]}
                assert gpio in allowed[state], 'Incorrect color GPIO'
                if active and waiting:
                    assert state=='OFF' and gpio==(0,0,0) and token==0 and limit==10000 and phase==500
                    wait_samples+=1
                elif active:
                    assert limit==2000 and phase==250 and token==0
                    samples.append((state,gpio,elapsed))
                if done is not None and not active and state=='OFF':
                    assert gpio==(0,0,0) and phase==500 and limit==10000
                    final=True
        if final:break
        if started and now>=next_query:
            port.write(b'ATC+NODE=?\r\n');port.flush();next_query=now+.10
    assert version_ok and id_ok, 'Wrong node identity/version or not READY'
    assert [s[0] for s in steps]==expected, 'Missing or wrong boot steps'
    assert done is not None and final, 'No final OFF confirmation'
    spans=[steps[i+1][1]-steps[i][1] for i in range(4)]+[done-steps[-1][1]]
    assert all(1.75<=span<=2.25 for span in spans), 'Boot step not approximately 2 seconds'
    for state in ['HOLD_OPEN','FAULT']:
        gpios={s[1] for s in samples if s[0]==state}
        assert (0,0,0) in gpios and any(sum(g)==1 for g in gpios), 'Missing flashing ON or OFF sample'
    if args.version=='0.4.4':
        assert wait_at is not None and wait_samples>0, 'Missing dark startup delay'
        assert 9.75<=steps[0][1]-wait_at<=10.25, 'Startup delay not approximately ten seconds'
    summary=dict(node=args.node,firmware=args.version,sequence=[s[0] for s in steps],
                 measured_step_seconds=spans,total_test_seconds=done-steps[0][1],
                 gpio_samples=len(samples),dark_delay_samples=wait_samples,
                 measured_dark_delay_seconds=steps[0][1]-wait_at if wait_at is not None else None,one_color_at_a_time=True,final_gpio_low=True,
                 boot_timing_report_ms=[250,2000],normal_timing_report_ms=[500,10000],
                 optical_verification=False)
    path=root/'build'/f'node{args.node}-{tag}-boot-summary.json'
    path.write_text(json.dumps(summary,indent=2)+'\n')
    print('PASS: runtime boot states, 2s steps, flashing GPIO, one-color outputs, final OFF; summary:',path)
