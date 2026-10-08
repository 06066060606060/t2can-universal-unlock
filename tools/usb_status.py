#!/usr/bin/env python3
"""Read live dashboard status over USB on macOS/Linux, without resetting the ESP."""
import argparse
import fcntl
import json
import os
import select
import termios
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--port", required=True, help="Confirmed ESP USB serial port")
parser.add_argument("--method", choices=["GET", "POST"], default="GET")
parser.add_argument("paths", nargs="*", default=[
    "/api/profile/status", "/api/blinkA/stats",
    "/api/lab/auto-lane-change/stats", "/api/system/stats",
])
args = parser.parse_args()
fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
try:
    fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    settings = termios.tcgetattr(fd)
    settings[0] = settings[1] = settings[3] = 0
    settings[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    settings[4] = settings[5] = termios.B115200
    settings[6][termios.VMIN] = settings[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, settings)
    termios.tcflush(fd, termios.TCIFLUSH)
    for path in args.paths:
        if not path.startswith("/api/") or any(ord(c) < 33 or ord(c) > 126 for c in path) or len(path) + len(args.method) > 62:
            raise ValueError("Expected a short ASCII API path")
        command = ("\n" + args.method + " " + path + "\n").encode("ascii")
        if os.write(fd, command) != len(command):
            raise OSError("Incomplete USB request")
        buffer = b""
        deadline = time.monotonic() + 5
        matched = False
        while time.monotonic() < deadline:
            if not select.select([fd], [], [], max(0, deadline - time.monotonic()))[0]:
                break
            buffer += os.read(fd, 4096)
            while b"\n" in buffer:
                line, buffer = buffer.split(b"\n", 1)
                try:
                    reply = json.loads(line)
                except (ValueError, UnicodeDecodeError):
                    continue  # Ignore optional boot/diagnostic log lines.
                if not isinstance(reply, dict):
                    continue
                if "error" in reply:
                    raise RuntimeError(reply["error"])
                if reply.get("path") == path:
                    if "error" in reply.get("data", {}):
                        raise RuntimeError(reply["data"]["error"])
                    print(json.dumps(reply), flush=True)
                    matched = True
                    break
            if matched:
                break
        else:
            raise TimeoutError("No USB response for " + path)
        if not matched:
            raise TimeoutError("No USB response for " + path)
finally:
    os.close(fd)
