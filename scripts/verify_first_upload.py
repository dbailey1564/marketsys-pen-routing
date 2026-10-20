"""Read main flash after first upload and compare image/bootloader locally."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
os.umask(0o077)
original = ROOT.parent / 'Backups/marketsys/prototype-01/20261020T193401.957087Z/original-main-flash.bin'
image = ROOT / 'build/build-check/build_check.ino.bin'
expected_hash = 'e4772686fa466ac79132418576681da9f6d323fb36f8836393f4409d5dde3bca'
image_data = image.read_bytes()
if hashlib.sha256(image_data).hexdigest() != expected_hash:
    raise RuntimeError('Image changed since upload preparation; refusing verification')
stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
destination = original.parent.parent / f'{stamp}-post-first-upload'
destination.mkdir(parents=True, exist_ok=False)
snapshot = destination / 'main-flash.bin'
config = destination / 'verify-readonly.cfg'
base = (ROOT / 'tools/inspect-flash.cfg').read_text().rstrip()
if not base.endswith('shutdown'):
    raise RuntimeError('Unexpected inspection configuration')
config.write_text(base.removesuffix('shutdown') + f'\necho [capture {{flash read_bank 0 {{{snapshot}}} 0 262144}}]\nshutdown\n')
print('Post-upload snapshot directory:', destination, flush=True)
with (destination / 'openocd-read.log').open('xb') as output:
    subprocess.run(['openocd', '-f', str(config)], cwd=ROOT, stdout=output,
                   stderr=subprocess.STDOUT, timeout=180, check=True)
observed = snapshot.read_bytes()
old = original.read_bytes()
if len(observed) != 262144:
    raise RuntimeError('Unexpected readback length')
report = {
    'created_utc': stamp,
    'application_base': '0x08006000',
    'image_size_bytes': len(image_data),
    'image_sha256': expected_hash,
    'application_matches_uploaded_image': observed[0x6000:0x6000+len(image_data)] == image_data,
    'bootloader_region_0x08000000_to_0x08005fff_unchanged': observed[:0x6000] == old[:0x6000],
    'bootloader_code_and_activation_marker_unchanged': observed[:0x4004] == old[:0x4004] and observed[0x4010:0x6000] == old[0x4010:0x6000],
    'region_above_application_allocation_0x08037000_to_0x0803ffff_unchanged': observed[0x37000:] == old[0x37000:],
    'post_upload_snapshot_sha256': hashlib.sha256(observed).hexdigest(),
    'restore_tested': False,
}
(destination / 'verification.json').write_text(json.dumps(report, indent=2)+'\n')
snapshot.chmod(0o400)
print(json.dumps(report, indent=2), flush=True)
if not report['application_matches_uploaded_image']:
    raise RuntimeError('Application readback does not match uploaded image')
if not report['bootloader_code_and_activation_marker_unchanged']:
    raise RuntimeError('Changes outside the reviewed update-record range; inspect before further programming')
