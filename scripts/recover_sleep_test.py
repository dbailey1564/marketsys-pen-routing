"""Restore verified application and updater record; preserve bootloader/settings."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import os
import subprocess

os.umask(0o077)
root = Path(__file__).resolve().parents[1]
backup = root.parent / 'Backups/marketsys/prototype-01/20261020T195515.754373Z-post-first-upload/main-flash.bin'
data = backup.read_bytes()
assert hashlib.sha256(data).hexdigest() == '3f74352ff11e134cc110dd32bbdb7bcbe38fdff05b38d0dce77a190755e4e309'
# Pin the failed image to its saved snapshot, never a mutable build output.
failed_snapshot = backup.parent.parent / '20261021T032832.288564Z-sleep-recovery/before.bin'
failed = failed_snapshot.read_bytes()[0x6000:0x6000 + 61392]
assert hashlib.sha256(failed).hexdigest() == '1cce650e122519ecb43cc8c9fc38b68a8f3db6ceadc4522e07b2587c6edf4537'
dest = root.parent / 'Backups/marketsys/prototype-01' / (datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ') + '-sleep-recovery')
dest.mkdir()
app = dest / 'working-app.bin'
record = dest / 'working-update-record-page.bin'
failed_image = dest / 'expected-failed-app.bin'
failed_image.write_bytes(failed)
app.write_bytes(data[0x6000:0x37000])
record.write_bytes(data[0x4000:0x4800])
base = (root / 'tools/diagnose-sleep-test.cfg').read_text().split('init\n')[0]
cfg = dest / 'recover.cfg'
cfg.write_text(base + f'''init
halt 2000
flash banks
flash read_bank 0 {{{dest / 'before.bin'}}} 0 262144
verify_image {{{failed_image}}} 0x08006000 bin
flash write_image erase {{{app}}} 0x08006000 bin
verify_image {{{app}}} 0x08006000 bin
flash write_image erase {{{record}}} 0x08004000 bin
verify_image {{{record}}} 0x08004000 bin
flash read_bank 0 {{{dest / 'after.bin'}}} 0 262144
reset run
shutdown
''')
print('Recovery directory:', dest, flush=True)
with (dest / 'openocd.log').open('w') as log:
    subprocess.run(['openocd', '-f', str(cfg)], stdout=log, stderr=subprocess.STDOUT, timeout=240, check=True)
before = (dest / 'before.bin').read_bytes()
after = (dest / 'after.bin').read_bytes()
assert len(before) == len(after) == 262144
assert after[0x6000:0x37000] == data[0x6000:0x37000]
assert after[0x4000:0x4800] == data[0x4000:0x4800]
assert before[:0x4000] == after[:0x4000]
assert before[0x4800:0x6000] == after[0x4800:0x6000]
assert before[0x37000:] == after[0x37000:]
print('PASS: working application and update record restored; bootloader code and other settings unchanged.', flush=True)
