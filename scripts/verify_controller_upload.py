"""Read controller flash and verify radio application, hardware identity, bootloader."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

os.umask(0o077)
root = Path(__file__).resolve().parents[1]
area = root.parent / 'Backups/marketsys/prototype-01'
original = area / '20261020T193401.957087Z/original-main-flash.bin'
old = original.read_bytes()
if hashlib.sha256(old).hexdigest() != '17edd1313d7e11163100b740c254091bcc6fe2d42798211a4c94eac5e54ba53b':
    raise RuntimeError('Unexpected original controller backup')
image = (root / 'build/radio-central/radio_central.ino.bin').read_bytes()
expected = 'daa355302123ccb06d87539379c8ceddf27747442de6d7da45c4bf2734a5593c'
if hashlib.sha256(image).hexdigest() != expected:
    raise RuntimeError('Unexpected controller firmware image')
stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
destination = area / (stamp + '-post-radio-central-upload')
destination.mkdir(parents=True, exist_ok=False)
snapshot = destination / 'main-flash.bin'
base = (root / 'tools/inspect-flash.cfg').read_text().rstrip()
if not base.endswith('shutdown'):
    raise RuntimeError('Unexpected inspection configuration')
config = destination / 'verify-readonly.cfg'
config.write_text(base.removesuffix('shutdown') + '\necho [capture {stm32wlx.cpu0 mdw 0x1fff7590 3}]\n' +
    f'echo [capture {{flash read_bank 0 {{{snapshot}}} 0 262144}}]\nshutdown\n')
log = destination / 'openocd-read.log'
with log.open('xb') as output:
    subprocess.run(['openocd', '-f', str(config)], cwd=root, stdout=output,
                   stderr=subprocess.STDOUT, timeout=180, check=True)
uid_match = re.search(r'0x1fff7590:\s+([0-9a-fA-F]{8})\s+([0-9a-fA-F]{8})\s+([0-9a-fA-F]{8})', log.read_text())
observed = snapshot.read_bytes()
if len(observed) != 262144:
    raise RuntimeError('Unexpected readback length')
report = {
    'board': 'controller (prototype-01)',
    'created_utc': stamp,
    'controller_uid_matches': bool(uid_match) and ''.join(uid_match.groups()).lower() == '0027001c4b33501220333734',
    'image_size_bytes': len(image),
    'image_sha256': expected,
    'application_matches': observed[0x6000:0x6000+len(image)] == image,
    'bootloader_except_update_record_unchanged': observed[:0x4004] == old[:0x4004] and observed[0x4010:0x6000] == old[0x4010:0x6000],
    'settings_region_changed': observed[0x37000:] != old[0x37000:],
    'snapshot_sha256': hashlib.sha256(observed).hexdigest(),
    'restore_tested': False,
}
(destination / 'verification.json').write_text(json.dumps(report, indent=2)+'\n')
snapshot.chmod(0o400)
print('Verification record:', destination / 'verification.json')
print(json.dumps(report, indent=2))
if not all(report[key] for key in ['controller_uid_matches', 'application_matches', 'bootloader_except_update_record_unchanged']):
    raise RuntimeError('Controller readback verification failed')
