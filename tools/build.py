#!/usr/bin/env python3
"""Stage the Arduino sketch under its required name and build one universal image."""
import argparse
import shutil
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--config-file")
parser.add_argument("--libraries")
parser.add_argument("--ctags", help="Native Arduino ctags directory, if needed on Apple Silicon")
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
build = root / "build"
sketch = build / "t2can-universal-unlock"
sketch.mkdir(parents=True, exist_ok=True)
for source in [*root.glob("*.h"), root / "t2can-universal-unlock.ino"]:
    shutil.copy2(source, sketch / source.name)
command = ["arduino-cli"]
if args.config_file:
    command += ["--config-file", args.config_file]
command += ["compile", "--fqbn", "esp32:esp32:esp32s3:FlashSize=16M,USBMode=hwcdc,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,LoopCore=1,EventsCore=1",
            "--build-path", str(build / "compile"), "--output-dir", str(build / "universal")]
if args.libraries:
    command += ["--libraries", args.libraries]
if args.ctags:
    command += ["--build-property", "runtime.tools.ctags.path=" + args.ctags]
subprocess.run([*command, str(sketch)], check=True)
