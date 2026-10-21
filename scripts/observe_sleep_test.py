"""Observe the finite sleep harness and inject a read-only UART interruption."""
import json
from pathlib import Path
import re
import time
import serial

ROOT = Path(__file__).resolve().parents[1]
ports = list(Path('/dev/serial/by-id').glob('usb-ARM_DAPLink_CMSIS-DAP_*-if01'))
if len(ports) != 1:
    raise SystemExit('Expected exactly one DAPLink UART')
started = time.monotonic()
records = []
query_at = None
query_sent = None
query_ok = False
complete = False

with serial.Serial(str(ports[0]), 115200, timeout=0.2, exclusive=True) as port, (ROOT/'build/sleep-test-observation.jsonl').open('w') as log:
    def record(kind, **values):
        item = {'host_elapsed_s':round(time.monotonic()-started, 3), 'kind':kind, **values}
        records.append(item)
        log.write(json.dumps(item)+'\n')
        log.flush()
        print(json.dumps(item), flush=True)

    pending = bytearray()
    while time.monotonic()-started < 330:
        chunk = port.read(1024)
        pending.extend(chunk)
        while b'\n' in pending:
            line, _, rest = pending.partition(b'\n')
            pending = bytearray(rest)
            text = line.decode(errors='replace').strip()
            if not text:
                continue
            record('serial', text=text)
            if 'MS_SLEEP FAIL' in text:
                raise RuntimeError('Harness reported failure')
            if query_sent is not None and text == 'OK':
                query_ok = True
            if text.startswith('MS_SLEEP WINDOW_CLOSE phase=FIELD cycle=1 '):
                query_at = time.monotonic()+20
            if text.startswith('MS_SLEEP COMPLETE'):
                complete = True
                if 'restore_lpm=1 restore_level=1' not in text:
                    raise RuntimeError('Original low-power settings were not restored')
                break
        if complete:
            break
        if query_at is not None and query_sent is None and time.monotonic() >= query_at:
            port.write(b'\r\n')  # Allow the UART wake edge before the actual query.
            port.flush()
            time.sleep(0.25)
            port.write(b'AT\r\n')
            port.flush()
            query_sent = time.monotonic()
            record('query_sent', command='AT')
        if query_sent is not None and not query_ok and time.monotonic()-query_sent > 5:
            raise RuntimeError('No AT response after UART interruption')

if not complete:
    raise RuntimeError('Harness did not complete within observation deadline')

opens = {'FAST':[], 'FIELD':[]}
closes = {'FAST':[], 'FIELD':[]}
for item in records:
    text = item.get('text','')
    match = re.match(r'MS_SLEEP WINDOW_(OPEN|CLOSE) phase=(FAST|FIELD) cycle=(\d+) tick_ms=(\d+) (?:phase_elapsed_ms|duration_ms)=(\d+)', text)
    if match:
        action, phase, cycle, tick, value = match.groups()
        (opens if action=='OPEN' else closes)[phase].append({'cycle':int(cycle),'tick':int(tick),'value':int(value),'host_s':item['host_elapsed_s']})
for phase, expected_period, expected_window in [('FAST',10000,1000),('FIELD',120000,5000)]:
    if len(opens[phase])!=2 or len(closes[phase])!=2:
        raise RuntimeError(f'Incomplete {phase} events')
    interval = (opens[phase][1]['tick']-opens[phase][0]['tick']) & 0xffffffff
    host_interval = opens[phase][1]['host_s']-opens[phase][0]['host_s']
    if abs(interval-expected_period)>1000 or abs(host_interval-expected_period/1000)>1:
        raise RuntimeError(f'{phase} periodic timing exceeded one-second bench tolerance')
    if any(not expected_window <= event['value'] <= expected_window+500 for event in closes[phase]):
        raise RuntimeError(f'{phase} simulated-window timing outside bench tolerance')
    if abs(opens[phase][0]['value']-expected_period)>1000:
        raise RuntimeError(f'{phase} initial timer was incorrect')
if not query_ok:
    raise RuntimeError('UART interruption was not verified')
summary = {'passed':True, 'opens':opens, 'closes':closes, 'uart_interruption_answered':query_ok,
           'scope':'RTC/timer callback scheduling and UART interruption only; no proof of electrical sleep depth, current, or RF behavior'}
(ROOT/'build/sleep-test-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
print('SUMMARY '+json.dumps(summary), flush=True)
