"""Freeze the tested startup-assessment images. Never uploads firmware."""
from pathlib import Path
import hashlib,json
root=Path(__file__).resolve().parents[1]
release=root/'build/releases/0.7.0';release.mkdir(parents=True,exist_ok=True)
prior=json.loads((root/'docs/firmware-release-060.json').read_text())
images=[]
for old in prior['images']:
 target=old['target']
 build='radio-central/radio_central.ino.bin' if target=='controller' else f'radio-node-{target[-1]}/radio_node.ino.bin'
 data=(root/'build'/build).read_bytes();assert 0<len(data)<=196*1024
 dest=release/f'{target}-0.7.0.bin'
 if dest.exists() and dest.read_bytes()!=data:raise RuntimeError(f'Frozen image differs: {dest}')
 if not dest.exists():dest.write_bytes(data);dest.chmod(0o444)
 images.append(dict(target=target,expected_uid=old['expected_uid'],file=str(dest.relative_to(root)),size_bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),application_address='0x08006000'))
paths=['firmware/radio_central/radio_central.ino','firmware/radio_node/radio_node.ino','libraries/MarketSysBench/library.properties']
paths += [str(p.relative_to(root)) for p in sorted((root/'libraries/MarketSysBench/src').glob('*.h'))]
manifest=dict(version='0.7.0',radio_wire_version=2,status='prepared_not_flashed',images=images,source_sha256={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},host_validation='14 host tests pass with C++11; all four target builds pass',hardware_validation='Pending backups, identities, all-board upload/readback and startup classification/power-cut tests',notes='All boards require radio version 2. Application events/replies remain deferred. Existing controller routing format retained.')
content=json.dumps(manifest,indent=2)+'\n'
(root/'docs/firmware-release-070.json').write_text(content);(release/'manifest.json').write_text(content)
print(content)
