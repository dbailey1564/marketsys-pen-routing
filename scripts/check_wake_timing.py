"""Offline phase/loss analysis; no serial, GPIO, radio, or firmware access."""
import json
from pathlib import Path

WINDOW = 5000
BEACON_INTERVAL = 2000
AIRTIME = 250
CONFIRMATION_AND_ALLOWANCE = 15000
DEADLINE = 300000

def reception(request, phase, period, lost_windows, first_packet_lost):
    origin = request + phase
    window_index = request // period
    for cycle in range(window_index, window_index + 6):
        start, end = cycle * period, cycle * period + WINDOW
        earliest = max(request, start)
        n = max(0, (earliest - origin + BEACON_INTERVAL-1) // BEACON_INTERVAL)
        packet = origin + n * BEACON_INTERVAL
        if packet + AIRTIME > end:
            continue
        if lost_windows:
            lost_windows -= 1
            continue
        if first_packet_lost:
            packet += BEACON_INTERVAL
            first_packet_lost = False
        if packet + AIRTIME <= end:
            return packet + AIRTIME + CONFIRMATION_AND_ALLOWANCE - request
    raise AssertionError('No modeled reception')

results = []
for period in (120000, 240000):
    requests = set(range(0, period, 100)) | {0, 1, 249, 250, 4999, 5000, 5001, period-1}
    phases = set(range(0, BEACON_INTERVAL+1, 25)) | {1, 249, 1999}
    for lost in (0, 1, 2):
        worst = 0
        cases = 0
        for request in sorted(requests):
            for phase in sorted(phases):
                for packet_loss in (False, True):
                    delay = reception(request, phase, period, lost, packet_loss)
                    worst = max(worst, delay)
                    cases += 1
        result = {'period_ms':period, 'whole_windows_lost':lost,
                  'cases':cases, 'max_modeled_delay_ms':worst,
                  'within_300_seconds':worst <= DEADLINE}
        results.append(result)
        if period == 120000 and lost <= 1:
            assert worst <= 260000, result
        if (period == 120000 and lost == 2) or (period == 240000 and lost >= 1):
            assert worst > DEADLINE, result

# Check full-window phase coverage at 1 ms resolution independently of request
# timing. Even the latest first beacon leaves room for a second complete packet.
for phase in range(BEACON_INTERVAL+1):
    assert phase + BEACON_INTERVAL + AIRTIME <= WINDOW

report = {'assumptions':{'usable_window_ms':WINDOW, 'beacon_start_interval_ms':BEACON_INTERVAL,
          'airtime_envelope_ms':AIRTIME, 'confirmation_plus_allowance_ms':CONFIRMATION_AND_ALLOWANCE},
          'scope':'Finite phase grid plus full-window 1ms coverage; not hardware, RF, collision, or clock validation',
          'results':results}
destination = Path(__file__).resolve().parents[1] / 'build/wake-timing-analysis.json'
destination.parent.mkdir(exist_ok=True)
destination.write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
