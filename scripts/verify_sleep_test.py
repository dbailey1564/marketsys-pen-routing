"""Read back the completed timer harness without halting or resetting the MCU."""
from pathlib import Path
import hashlib
import subprocess

root = Path(__file__).resolve().parents[1]
image = (root / 'build/sleep-test/sleep_test.ino.bin').read_bytes()
assert hashlib.sha256(image).hexdigest() == 'af6ef4b70dcd044d027183caa1b73afacf7e23283fed9239322470b4e133392c'
base = (root / 'tools/inspect-flash.cfg').read_text().removesuffix('shutdown\n')
observed = root / 'build/sleep-test/application-readback.bin'
config = root / 'build/sleep-test/verify-readback.cfg'
config.write_text(base + f'flash read_bank 0 {{{observed}}} 24576 {len(image)}\nshutdown\n')
with (root / 'build/sleep-test/verify-readback.log').open('w') as log:
    subprocess.run(['openocd', '-f', str(config)], stdout=log, stderr=subprocess.STDOUT, timeout=150, check=True)
assert observed.read_bytes() == image, 'Application readback mismatch'
print(f'PASS: all {len(image)} application bytes match the compiled timer harness.')
