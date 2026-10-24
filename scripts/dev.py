"""Pinned Linux x86_64 RUI3 setup and compile tasks. Never uploads firmware."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import platform
import subprocess
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
LOCK = json.loads((ROOT / "toolchain.json").read_text())
TOOLS = ROOT.parent / "Tools"
CLI = TOOLS / "arduino-cli" / LOCK["arduino_cli_version"] / "arduino-cli"
ENV = TOOLS / "marketsys-rui3"
CONFIG = ENV / "arduino-cli.yaml"
REFERENCE = TOOLS / "RUI3-Best-Practice"

def run(*args, **kwargs):
    return subprocess.run([str(arg) for arg in args], check=True, **kwargs)

def cli(*args, **kwargs):
    return run(CLI, "--config-file", CONFIG, *args, **kwargs)

def download(url):
    request = urllib.request.Request(url, headers={"User-Agent": "MarketSys-build-setup"})
    with urllib.request.urlopen(request, timeout=120) as response:
        return response.read()

def setup():
    if platform.system() != "Linux" or platform.machine() != "x86_64":
        raise SystemExit("Run setup in the Linux x86_64 development environment (Lin).")
    version = LOCK["arduino_cli_version"]
    if not CLI.exists():
        url = f"https://github.com/arduino/arduino-cli/releases/download/v{version}/arduino-cli_{version}_Linux_64bit.tar.gz"
        payload = download(url)
        if hashlib.sha256(payload).hexdigest() != LOCK["arduino_cli_linux_x64_sha256"]:
            raise RuntimeError("Arduino CLI checksum mismatch")
        CLI.parent.mkdir(parents=True, exist_ok=True)
        with tarfile.open(fileobj=io.BytesIO(payload), mode="r:gz") as archive:
            member = archive.getmember("arduino-cli")
            if not member.isfile():
                raise RuntimeError("Unexpected Arduino CLI archive member")
            with CLI.open("xb") as target:
                target.write(archive.extractfile(member).read())
        CLI.chmod(0o755)
    settings = {
        "board_manager": {"additional_urls": [LOCK["package_index_url"]]},
        "directories": {"data": str(ENV / "data"), "downloads": str(ENV / "downloads"), "user": str(ENV / "user")},
    }
    ENV.mkdir(parents=True, exist_ok=True)
    if CONFIG.exists() and json.loads(CONFIG.read_text()) != settings:
        raise RuntimeError("Existing toolchain configuration differs; refusing to overwrite")
    CONFIG.write_text(json.dumps(settings, indent=2) + "\n")
    cli("core", "update-index")
    cli("core", "install", f'{LOCK["core"]}@{LOCK["core_version"]}')
    if not REFERENCE.exists():
        run("git", "clone", "--no-checkout", LOCK["reference_url"], REFERENCE)
        run("git", "-C", REFERENCE, "checkout", "--detach", LOCK["reference_commit"])
    ensure_reference()

def ensure_reference():
    actual = run("git", "-C", REFERENCE, "rev-parse", "HEAD", capture_output=True, text=True).stdout.strip()
    if actual != LOCK["reference_commit"]:
        raise RuntimeError("Reference revision differs from toolchain.json; review before building")
    changed = run("git", "-C", REFERENCE, "status", "--porcelain", capture_output=True, text=True).stdout.strip()
    if changed:
        raise RuntimeError("Reference has local changes; review before building")

def verify():
    if not CLI.is_file() or not CONFIG.is_file():
        raise SystemExit("Toolchain is not configured. Run: python3 scripts/dev.py setup")
    version = run(CLI, "version", "--format", "json", capture_output=True, text=True)
    if json.loads(version.stdout)["VersionString"] != LOCK["arduino_cli_version"]:
        raise RuntimeError("Arduino CLI version differs from toolchain.json")
    installed = cli("core", "list", "--format", "json", capture_output=True, text=True)
    platforms = json.loads(installed.stdout).get("platforms", [])
    if not any(item["id"] == LOCK["core"] and item["installed_version"] == LOCK["core_version"] for item in platforms):
        raise RuntimeError("Pinned RUI3 core is not installed; run setup")
    print(f'Using Arduino CLI {LOCK["arduino_cli_version"]}, {LOCK["core"]}@{LOCK["core_version"]}', flush=True)

def build(target):
    verify()
    if target == "build-check":
        sketch = ROOT / "firmware/build_check"
        fqbn = LOCK["fqbn"]
    elif target == "sleep-test":
        sketch = ROOT / "firmware/sleep_test"
        fqbn = LOCK["sleep_test_fqbn"]
    elif target == "indicator-test":
        sketch = ROOT / "firmware/indicator_test"
        fqbn = LOCK["fqbn"]
    elif target in ("radio-node", "radio-central"):
        sketch = ROOT / "firmware" / target.replace('-', '_')
        fqbn = LOCK["fqbn"]
    else:
        ensure_reference()
        name = "LoRa_P2P_RX" if target == "reference-rx" else "LoRa_P2P_TX"
        sketch = REFERENCE / "LoRa_P2P" / name
        # Vendor examples require LED_GREEN/LED_BLUE from the evaluation board.
        fqbn = LOCK["reference_fqbn"]
    output = ROOT / "build" / target
    output.mkdir(parents=True, exist_ok=True)
    print(f"Compile target: {fqbn}", flush=True)
    cli("compile", "--fqbn", fqbn, "--libraries", ROOT / "libraries", "--build-path", output, sketch)

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("task", choices=["setup", "verify", "build-check", "sleep-test", "indicator-test", "radio-node", "radio-central", "reference-rx", "reference-tx"])
task = parser.parse_args().task
if task == "setup":
    setup()
elif task == "verify":
    verify()
else:
    build(task)
