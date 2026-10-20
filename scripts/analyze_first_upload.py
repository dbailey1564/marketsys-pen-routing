"""Report changed address ranges, never private flash values."""
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1].parent / 'Backups/marketsys/prototype-01'
before = (root / '20261020T193401.957087Z/original-main-flash.bin').read_bytes()
after = (root / '20261020T195515.754373Z-post-first-upload/main-flash.bin').read_bytes()
for name, start, end in [('bootloader-reserved', 0, 0x6000), ('above-application-allocation', 0x37000, 0x40000)]:
    changes = [index for index in range(start,end) if before[index] != after[index]]
    runs = []
    for index in changes:
        if runs and index == runs[-1][1]+1:
            runs[-1][1] = index
        else:
            runs.append([index,index])
    print(json.dumps({'region':name, 'changed_bytes':len(changes), 'changed_address_ranges':[[hex(0x08000000+a),hex(0x08000000+b)] for a,b in runs]}))
version = b'MarketSys-SiP-check-0.1.0'
location = after.find(version, 0x37000)
print(json.dumps({'custom_application_version_found_above_application_allocation':location >= 0, 'address':hex(0x08000000+location) if location >= 0 else None}))
