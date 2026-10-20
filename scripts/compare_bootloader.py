"""Compare packaged bootloader bytes to the private original backup, without printing data."""
from pathlib import Path
import json
import sys

ROOT = Path(__file__).resolve().parents[1]
lock = json.loads((ROOT / 'toolchain.json').read_text())
backup = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT.parent / 'Backups/marketsys/prototype-01/20261020T193401.957087Z/original-main-flash.bin'
data = backup.read_bytes()
prebuilt = ROOT.parent / 'Tools/marketsys-rui3/data/packages/rak_rui/hardware/stm32' / lock['core_version'] / 'prebuilt'
for source in sorted(prebuilt.glob('*.hex')):
    address_base = 0
    payload = {}
    for line in source.read_text().splitlines():
        record = bytes.fromhex(line.removeprefix(':'))
        if sum(record) & 0xff or len(record) != record[0] + 5:
            raise RuntimeError('Invalid Intel HEX record')
        count, kind = record[0], record[3]
        offset = int.from_bytes(record[1:3], 'big')
        if kind == 4:
            address_base = int.from_bytes(record[4:6], 'big') << 16
        elif kind == 2:
            address_base = int.from_bytes(record[4:6], 'big') << 4
        elif kind == 0:
            for index, value in enumerate(record[4:4+count]):
                payload[address_base + offset + index] = value
    if not payload:
        raise RuntimeError('Empty bootloader image')
    relevant = {address:value for address,value in payload.items() if 0x08000000 <= address < 0x08006000}
    differences = sum(data[address-0x08000000] != value for address,value in relevant.items())
    print(json.dumps({'reference':source.name, 'reference_bytes':len(payload), 'bootloader_bytes_compared':len(relevant), 'different_bytes':differences, 'all_reference_bytes_within_bootloader_region':len(relevant)==len(payload)}), flush=True)
