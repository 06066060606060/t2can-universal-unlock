"""Build identical frozen sources with -Os and -O2; never uploads firmware."""
import argparse
import hashlib
import json
import re
import shutil
import subprocess
from datetime import datetime
from pathlib import Path

DEFAULT_FQBN = "esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,LoopCore=1,EventsCore=0"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fqbn", default=DEFAULT_FQBN)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--reuse-output", action="store_true",
                        help="Reuse an existing comparison directory after a source correction")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    cli = shutil.which("arduino-cli")
    if not cli:
        raise SystemExit("arduino-cli unavailable")
    output = (args.output or root / ".build" / "can-performance" / datetime.now().strftime("%Y%m%d-%H%M%S")).resolve()
    if args.reuse_output and (not args.output or not (output / "source-manifest.json").is_file()):
        raise SystemExit("--reuse-output requires --output with an existing source manifest")
    output.mkdir(parents=True, exist_ok=args.reuse_output)
    source = output / "TMR_Performance"
    source.mkdir(exist_ok=args.reuse_output)
    manifest = {}
    for path in sorted(root.iterdir()):
        if path.suffix not in (".h", ".cpp") and path.name != "TMR-Universal-v3.21.2.ino":
            continue
        data = path.read_bytes()
        manifest[path.name] = hashlib.sha256(data).hexdigest()
        (source / ("TMR_Performance.ino" if path.suffix == ".ino" else path.name)).write_bytes(data)
    expected = {"TMR_Performance.ino" if name.endswith(".ino") else name for name in manifest}
    if {p.name for p in source.iterdir()} != expected:
        raise SystemExit("Snapshot contains unexpected files; use a fresh output directory")
    (output / "source-manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    report = {"fqbn": args.fqbn, "hardwareMeasurements": "not performed", "builds": {}}
    for mode in ("Os", "O2"):
        build = output / mode
        label = " -DTMR_PERF_BUILD_O2=1" if mode == "O2" else ""
        command = [cli, "compile", "--fqbn", args.fqbn, "--build-path", str(build),
                   "--verbose", "--build-property", f"compiler.optimization_flags=-{mode}",
                   "--build-property", f'compiler.cpp.extra_flags=-I"{source}"{label}', str(source)]
        print(f"Building {mode}: {build}", flush=True)
        with (output / f"{mode}.log").open("w", encoding="utf-8") as log:
            completed = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, text=True)
        log_text = (output / f"{mode}.log").read_text(encoding="utf-8")
        result = {"exitCode": completed.returncode}
        if completed.returncode:
            print(log_text[-6000:], flush=True)
            report["builds"][mode] = result
            (output / "comparison.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
            raise SystemExit(completed.returncode)
        # Verify flags for the two production translation units, not IDF archives.
        database = json.loads((build / "compile_commands.json").read_text(encoding="utf-8"))
        for unit in ("TMR_Performance.ino.cpp", "mcp2515.cpp"):
            entries = [entry for entry in database if Path(entry["file"]).name == unit]
            lines = [" ".join(entry.get("arguments", [])) or entry.get("command", "") for entry in entries]
            # Cached objects still have a compilation database entry. Reject
            # later optimization overrides in the sketch response files too.
            response_opts = (build / "build_opt.h").read_text() + " " + (build / "file_opts").read_text()
            if not lines or not all(re.findall(r"(?<!\w)-O[0-3sg](?!\w)", line + " " + response_opts)[-1:] == [f"-{mode}"] for line in lines):
                raise RuntimeError(f"Could not verify {mode} flags for {unit}")
        result["productionFlagsVerified"] = True
        for key, pattern in (("programBytes", r"Sketch uses (\d+) bytes"),
                             ("staticRamBytes", r"Global variables use (\d+) bytes")):
            match = re.search(pattern, log_text)
            result[key] = int(match.group(1)) if match else None
        binary = build / "TMR_Performance.ino.bin"
        result["applicationBin"] = str(binary)
        result["applicationBinBytes"] = binary.stat().st_size
        result["sha256"] = hashlib.sha256(binary.read_bytes()).hexdigest()
        report["builds"][mode] = result
        print(json.dumps({mode: result}), flush=True)
    report["sourceUnchangedDuringBuild"] = all(hashlib.sha256((root / name).read_bytes()).hexdigest() == digest for name, digest in manifest.items())
    (output / "comparison.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    if not report["sourceUnchangedDuringBuild"]:
        raise SystemExit("Sources changed during build; rerun comparison")
    print(f"Comparison: {output / 'comparison.json'}", flush=True)


if __name__ == "__main__":
    main()
