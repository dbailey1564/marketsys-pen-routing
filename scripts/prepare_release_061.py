"""Freeze the controller-only 0.6.1 build; never uploads firmware."""
from pathlib import Path
import hashlib,json
root=Path(__file__).resolve().parents[1]
release=root/'build/releases/0.6.1';release.mkdir(parents=True,exist_ok=True)
data=(root/'build/radio-central/radio_central.ino.bin').read_bytes()
assert 0<len(data)<=196*1024
image=release/'controller-0.6.1.bin'
if image.exists() and image.read_bytes()!=data:raise RuntimeError('Existing frozen image differs; review before replacing')
if not image.exists():image.write_bytes(data);image.chmod(0o444)
paths=['firmware/radio_central/radio_central.ino']+[str(p.relative_to(root)) for p in sorted((root/'libraries/MarketSysBench/src').glob('*.h'))]
m=dict(version='0.6.1',scope='controller only; nodes remain 0.6.0',status='prepared_not_flashed',images=[dict(target='controller',expected_uid='0027001c4b33501220333734',file='build/releases/0.6.1/controller-0.6.1.bin',size_bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),application_address='0x08006000')],source_sha256={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},hardware_validation='Pending fresh backup, upload/readback and destination-only advancement walkthrough')
s=json.dumps(m,indent=2)+'\n';(root/'docs/firmware-release-061.json').write_text(s);(release/'manifest.json').write_text(s)
print(s)
