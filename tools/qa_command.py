#!/usr/bin/env python3
"""Evaluate an expression in the emulator's test-only mock dashboard."""
import json
import sys
import time
import urllib.request
import uuid

def command(code, timeout=15):
    command_id = str(uuid.uuid4())
    request = urllib.request.Request("http://127.0.0.1:8080/__qa/control",
                                     json.dumps({"code": code, "id": command_id}).encode(), method="POST")
    urllib.request.urlopen(request, timeout=5).close()
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        with urllib.request.urlopen("http://127.0.0.1:8080/__qa/result", timeout=5) as response:
            result = json.load(response)
        if result.get("id") == command_id:
            if not result.get("ok"):
                raise RuntimeError(result)
            return result.get("value")
        time.sleep(.3)
    raise TimeoutError("No mock-dashboard response")

if __name__ == "__main__":
    print(json.dumps(command(sys.argv[1]), ensure_ascii=False))
