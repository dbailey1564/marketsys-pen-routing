"""Read back Node1 application and compare preserved regions to its own backup."""
from datetime import datetime, timezone
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
os.umask(0o077)
original = ROOT.parent / 'Backups/marketsys/proto-node1-v1/20261024T192523.820282Z/original-main-flash.bin'
image = ROOT / 'build/indicator-test/indicator_test.ino.bin'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sha256', default='4260f24e80da0deebdd8ef0a24116cd10a3358bb5ca480bf23bd9ecabdd49e12')
expected_hash = parser.parse_args().sha256
if len(expected_hash) != 64 or any(c not in '0123456789abcdef' for c in expected_hash):
    raise RuntimeError('Expected a lowercase SHA256 digest')
image_data = image.read_bytes()
if hashlib.sha256(image_data).hexdigest() != expected_hash:
    raise RuntimeError('Unexpected firmware image hash')
stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
destination = original.parent.parent / (stamp + '-post-indicator-upload')
destination.mkdir(parents=True, exist_ok=False)
snapshot = destination / 'main-flash.bin'
base = (ROOT / 'tools/inspect-flash.cfg').read_text().rstrip()
if not base.endswith('shutdown'):
    raise RuntimeError('Unexpected inspection configuration')
config = destination / 'verify-readonly.cfg'
config.write_text(base.removesuffix('shutdown') + f'\necho [capture {{flash read_bank 0 {{{snapshot}}} 0 262144}}]\nshutdown\n')
with (destination / 'openocd-read.log').open('xb') as output:
    subprocess.run(['openocd', '-f', str(config)], cwd=ROOT, stdout=output,
                   stderr=subprocess.STDOUT, timeout=180, check=True)
observed = snapshot.read_bytes()
old = original.read_bytes()
if len(observed) != 262144:
    raise RuntimeError('Unexpected readback size')
report = {
    'board': 'PROTO - Node1 V1',
    'created_utc': stamp,
    'application_base': '0x08006000',
    'image_size_bytes': len(image_data),
    'image_sha256': expected_hash,
    'application_matches': observed[0x6000:0x6000+len(image_data)] == image_data,
    'bootloader_except_update_record_unchanged': observed[:0x4004] == old[:0x4004] and observed[0x4010:0x6000] == old[0x4010:0x6000],
    'boot_update_record_changed': observed[0x4004:0x4010] != old[0x4004:0x4010],
    'settings_region_changed': observed[0x37000:] != old[0x37000:],
    'snapshot_sha256': hashlib.sha256(observed).hexdigest(),
    'restore_tested': False,
}
(destination / 'verification.json').write_text(json.dumps(report, indent=2)+'\n')
snapshot.chmod(0o400)
print('Verification record:', destination / 'verification.json')
print(json.dumps(report, indent=2))
if not report['application_matches'] or not report['bootloader_except_update_record_unchanged']:
    raise RuntimeError('Readback verification failed')
