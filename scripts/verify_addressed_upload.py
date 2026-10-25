"""Read flash and verify the expected UID, application and preserved bootloader."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--backup', type=Path, required=True, help='Verified pre-upload backup directory')
parser.add_argument('--image', type=Path, required=True)
parser.add_argument('--sha256', required=True)
parser.add_argument('--expected-uid', required=True)
args = parser.parse_args()
os.umask(0o077)
root = Path(__file__).resolve().parents[1]
manifest = json.loads((args.backup / 'manifest.json').read_text())
if not manifest['verified_against_second_full_read'] or manifest['target_uid_words'] != args.expected_uid.lower():
    raise RuntimeError('Backup identity or verification mismatch')
old = (args.backup / 'original-main-flash.bin').read_bytes()
if hashlib.sha256(old).hexdigest() != manifest['sha256']:
    raise RuntimeError('Backup checksum mismatch')
image = args.image.read_bytes()
if hashlib.sha256(image).hexdigest() != args.sha256 or not 0 < len(image) <= 196*1024:
    raise RuntimeError('Firmware checksum or size mismatch')
stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
dest = args.backup.parent / (stamp + '-post-addressed-upload')
dest.mkdir(exist_ok=False)
snapshot = dest / 'main-flash.bin'
base = (root / 'tools/inspect-flash.cfg').read_text().rstrip()
if not base.endswith('shutdown'): raise RuntimeError('Unexpected inspection configuration')
config = dest / 'verify-readonly.cfg'
config.write_text(base.removesuffix('shutdown') + '\necho [capture {stm32wlx.cpu0 mdw 0x1fff7590 3}]\n' +
    f'echo [capture {{flash read_bank 0 {{{snapshot}}} 0 262144}}]\nshutdown\n')
with (dest / 'openocd-read.log').open('xb') as log:
    subprocess.run(['openocd', '-f', str(config)], cwd=root, stdout=log, stderr=subprocess.STDOUT, timeout=180, check=True)
match = re.search(r'0x1fff7590:\s+([0-9a-fA-F]{8})\s+([0-9a-fA-F]{8})\s+([0-9a-fA-F]{8})', (dest / 'openocd-read.log').read_text())
data = snapshot.read_bytes()
report = dict(board=manifest['board_label'], created_utc=stamp,
    target_uid_matches=bool(match) and ''.join(match.groups()).lower() == args.expected_uid.lower(),
    image_sha256=args.sha256, image_size_bytes=len(image),
    application_matches=len(data)==262144 and data[0x6000:0x6000+len(image)]==image,
    bootloader_except_update_record_unchanged=data[:0x4004]==old[:0x4004] and data[0x4010:0x6000]==old[0x4010:0x6000],
    settings_region_changed=data[0x37000:]!=old[0x37000:],
    snapshot_sha256=hashlib.sha256(data).hexdigest(), pre_upload_backup=str(args.backup), restore_tested=False)
(dest / 'verification.json').write_text(json.dumps(report, indent=2)+'\n')
snapshot.chmod(0o400)
print('Verification record:', dest / 'verification.json')
print(json.dumps(report, indent=2))
if not all(report[k] for k in ['target_uid_matches','application_matches','bootloader_except_update_record_unchanged']):
    raise RuntimeError('Upload readback verification failed')
