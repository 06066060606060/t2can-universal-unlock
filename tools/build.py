#!/usr/bin/env python3
"""Dependency-free Android build using official SDK tools and a JDK (no Gradle)."""
import hashlib
import os
from pathlib import Path
import argparse
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def first(pattern):
    matches = sorted(ROOT.glob(pattern))
    return str(matches[-1]) if matches else ""


def run(*args):
    subprocess.run([str(arg) for arg in args], check=True, cwd=ROOT)


def build():
    jdk = Path(os.environ.get("T2CAN_JDK", first(".toolchain/jdk-*/Contents/Home")))
    tools = Path(os.environ.get("T2CAN_BUILD_TOOLS", first(".toolchain/build-tools/*")))
    android = Path(os.environ.get("T2CAN_ANDROID_JAR", first(".toolchain/platform/*/android.jar")))
    for executable in [jdk / "bin/javac", jdk / "bin/java", tools / "aapt2", android]:
        if not executable.is_file():
            raise SystemExit("Set T2CAN_JDK, T2CAN_BUILD_TOOLS, T2CAN_ANDROID_JAR. Missing: " + str(executable))
    # Unique build folders preserve earlier output and avoid stale class files.
    import tempfile
    (ROOT / "build").mkdir(exist_ok=True)
    build_dir = Path(tempfile.mkdtemp(prefix="release-", dir=ROOT / "build"))
    classes = build_dir / "classes"
    generated = build_dir / "generated"
    dex = build_dir / "dex"
    for directory in [classes, generated, dex, ROOT / "artifacts", ROOT / "private"]:
        directory.mkdir(exist_ok=True)
    resources = build_dir / "resources.zip"
    unsigned = build_dir / "unsigned.apk"
    run(tools / "aapt2", "compile", "--dir", ROOT / "res", "-o", resources)
    run(tools / "aapt2", "link", "-o", unsigned, "-I", android,
        "--manifest", ROOT / "AndroidManifest.xml", "--java", generated,
        "--min-sdk-version", "26", "--target-sdk-version", "36", "-A", ROOT / "assets", resources)
    sources = sorted((ROOT / "src").rglob("*.java")) + sorted(generated.rglob("*.java"))
    run(jdk / "bin/javac", "-encoding", "UTF-8", "--release", "8",
        "-classpath", android, "-d", classes, *sources)
    class_jar = build_dir / "classes.jar"
    with zipfile.ZipFile(class_jar, "w", zipfile.ZIP_DEFLATED) as archive:
        for source in sorted(classes.rglob("*.class")):
            archive.write(source, source.relative_to(classes))
    run(jdk / "bin/java", "-cp", tools / "lib/d8.jar", "com.android.tools.r8.D8",
        "--release", "--min-api", "26", "--lib", android, "--output", dex, class_jar)
    with zipfile.ZipFile(unsigned, "a", zipfile.ZIP_DEFLATED) as archive:
        for source in sorted(dex.glob("*.dex")):
            archive.write(source, source.name)
    aligned = build_dir / "aligned.apk"
    run(tools / "zipalign", "-f", "-p", "4", unsigned, aligned)
    key = Path(os.environ.get("T2CAN_SIGNING_KEY", ROOT / "private/release.p12"))
    password_file = Path(os.environ.get("T2CAN_SIGNING_PASSWORD_FILE", ROOT / "private/signing-password.txt"))
    if not key.is_file() or not password_file.is_file():
        raise SystemExit("Existing signing key/password required. Set T2CAN_SIGNING_KEY and T2CAN_SIGNING_PASSWORD_FILE; no replacement key is generated.")
    output = Path(os.environ.get("T2CAN_OUTPUT", ROOT / "artifacts"))
    output.mkdir(parents=True, exist_ok=True)
    apk = output / "Tesla-Unlock-1.2.1.apk"
    run(jdk / "bin/java", "-jar", tools / "lib/apksigner.jar", "sign",
        "--ks", key, "--ks-key-alias", "tesla-unlock", "--ks-pass", "file:" + str(password_file),
        "--v4-signing-enabled", "false", "--out", apk, aligned)
    run(jdk / "bin/java", "-jar", tools / "lib/apksigner.jar", "verify", "--verbose", "--print-certs", apk)
    run(tools / "zipalign", "-c", "4", apk)
    print("APK:", apk)
    print("Bytes:", apk.stat().st_size)
    print("SHA256:", hashlib.sha256(apk.read_bytes()).hexdigest())


if __name__ == "__main__":
    build()
