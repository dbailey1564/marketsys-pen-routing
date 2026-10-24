"""Read the connected prototype's main flash twice; never unlock or program it."""
from datetime import datetime, timezone
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--board', default='prototype-01', choices=['prototype-01', 'proto-node1-v1'])
parser.add_argument('--expected-uid', help='Expected 24 hex digits: three UID words in displayed order')
args = parser.parse_args()
if args.expected_uid and not re.fullmatch(r'[0-9a-fA-F]{24}', args.expected_uid):
    parser.error('Expected UID must contain exactly 24 hexadecimal digits')
os.umask(0o077)
stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
destination = ROOT.parent / "Backups" / "marketsys" / args.board / stamp
destination.mkdir(parents=True, exist_ok=False)
first = destination / "original-main-flash.bin"
second = destination / "verification-read.bin"
config = destination / "backup-readonly.cfg"
base_config = (ROOT / "tools/inspect-flash.cfg").read_text()
if not base_config.rstrip().endswith("shutdown"):
    raise RuntimeError("Unexpected inspection configuration")
base_config = base_config.rstrip().removesuffix("shutdown")
for path in (first, second):
    if any(char in str(path) for char in "{}\n\r"):
        raise RuntimeError("Unsupported backup path")
commands = base_config + """
set observed_optr [lindex [stm32wlx.cpu0 read_memory 0x58004020 32 1] 0]
if {($observed_optr & 0xff) != 0xaa} {
    error "Read protection is not level 0; refusing backup without any unlock"
}
echo [capture {stm32wlx.cpu0 mdw 0x1fff7590 3}]
"""
commands += f"echo [capture {{flash read_bank 0 {{{first}}} 0 262144}}]\n"
commands += f"echo [capture {{flash read_bank 0 {{{second}}} 0 262144}}]\n"
commands += "shutdown\n"
config.write_text(commands)
print("Backup directory:", destination, flush=True)
log = destination / "openocd-read.log"
with log.open("xb") as output:
    result = subprocess.run(["openocd", "-f", str(config)], cwd=ROOT,
                            stdout=output, stderr=subprocess.STDOUT, timeout=300)
if result.returncode:
    raise RuntimeError(f"Read failed ({result.returncode}); see {log}. No backup verified.")
uid_match = re.search(r'0x1fff7590:\s+([0-9a-fA-F]{8})\s+([0-9a-fA-F]{8})\s+([0-9a-fA-F]{8})', log.read_text())
if not uid_match:
    raise RuntimeError('Target UID was not recorded')
uid = ''.join(uid_match.groups()).lower()
if args.expected_uid and uid != args.expected_uid.lower():
    raise RuntimeError('Target UID does not match the expected board; do not program')
data = first.read_bytes()
verification = second.read_bytes()
if len(data) != 262144 or len(verification) != 262144:
    raise RuntimeError("Unexpected backup length; no backup verified")
if data != verification:
    raise RuntimeError("Flash reads differ; no backup verified")
if len(set(data)) <= 1:
    raise RuntimeError("Uniform flash data is suspicious; no backup verified")
digest = hashlib.sha256(data).hexdigest()
metadata = {
    "created_utc": stamp,
    "board_label": args.board,
    "target_uid_words": uid,
    "photographed_hardware": "RAK3272-SiP Ver B / RAK3172-SiP",
    "base_address": "0x08000000",
    "size_bytes": len(data),
    "sha256": digest,
    "verified_against_second_full_read": True,
    "read_protection": "Level 0 (0xAA), checked before reading",
    "files": [first.name, second.name],
    "scope": "Main flash only; includes bytes currently present in bootloader/application/settings areas. Does not include OTP/system ROM or a complete option-byte image.",
    "cpu_halted": False,
    "target_reset": False,
    "flash_written": False,
    "protection_changed": False,
    "restore_tested": False,
}
(destination / "manifest.json").write_text(json.dumps(metadata, indent=2) + "\n")
for path in (first, second):
    path.chmod(0o400)
print("Verified two matching full-flash reads:", len(data), "bytes", flush=True)
print("SHA256:", digest, flush=True)
print("Manifest:", destination / "manifest.json", flush=True)
