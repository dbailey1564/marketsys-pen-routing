"""Freeze built 0.6.0 images and checksums for later verified flashing; never programs hardware."""
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
release = root / 'build/releases/0.6.0'
release.mkdir(parents=True, exist_ok=True)
targets = [
    ('controller', 'radio-central/radio_central.ino.bin', '0027001c4b33501220333734'),
    ('node1', 'radio-node-1/radio_node.ino.bin', '003300455233500d20303938'),
    ('node2', 'radio-node-2/radio_node.ino.bin', '003300265233500d20303938'),
    ('node3', 'radio-node-3/radio_node.ino.bin', '003400345233500d20303938'),
]
images = []
# Validate every target before copying any file.
for label, relative, uid in targets:
    data = (root / 'build' / relative).read_bytes()
    if not 0 < len(data) <= 196*1024:
        raise RuntimeError('Invalid application size: ' + label)
    name = label + '-0.6.0.bin'
    destination = release / name
    if destination.exists() and destination.read_bytes() != data:
        raise RuntimeError('Frozen image differs; do not overwrite: ' + str(destination))
    images.append(dict(target=label, expected_uid=uid, file='build/releases/0.6.0/'+name,
        size_bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
        application_address='0x08006000'))
for entry, (_, relative, _) in zip(images, targets):
    destination = root / entry['file']
    if not destination.exists():
        destination.write_bytes((root / 'build' / relative).read_bytes())
        destination.chmod(0o444)
source_paths = ['firmware/radio_central/radio_central.ino', 'firmware/radio_node/radio_node.ino']
source_paths += [str(p.relative_to(root)) for p in sorted((root/'libraries/MarketSysBench/src').glob('*.h'))]
manifest = dict(version='0.6.0', status='prepared_not_flashed',
    images=images, source_sha256={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in source_paths},
    hardware_validation='Pending backup, upload, full readback, UART, LoRa and visual checks')
text = json.dumps(manifest, indent=2)+'\n'
(root/'docs/firmware-release-060.json').write_text(text)
(release/'manifest.json').write_text(text)
print(text)
