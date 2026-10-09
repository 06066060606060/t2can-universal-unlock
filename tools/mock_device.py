#!/usr/bin/env python3
"""Local-only emulator fixture. Never embedded in the APK or sent to a vehicle."""
import argparse
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import queue
import re
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument("firmware_zip")
parser.add_argument("--port", type=int, default=8080)
args = parser.parse_args()
source = Path(args.firmware_zip)
if source.is_dir():
    html = (source / "dashboard_source.html").read_text()
    mock = (source / "tools/v38_preview_mock.js").read_text()
    fonts_source = (source / "lab_fonts.h").read_text()
else:
    with zipfile.ZipFile(source) as archive:
        roots = [name[:-len("dashboard_source.html")] for name in archive.namelist() if name.endswith("dashboard_source.html")]
        if len(roots) != 1: raise SystemExit("Expected one dashboard source")
        prefix = roots[0]
        html = archive.read(prefix + "dashboard_source.html").decode()
        mock = archive.read(prefix + "tools/v38_preview_mock.js").decode()
        fonts_source = archive.read(prefix + "lab_fonts.h").decode()

fonts = {}
for name, body in re.findall(r"(LAB_GEIST(?:_MONO)?_FONT)\[\].*?=\s*\{(.*?)\};", fonts_source, re.S):
    fonts[name] = bytes(int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]{2})", body))

qa = r"""
const originalDeviceFetch = window.fetch.bind(window);
""" + mock + r"""
const previewFetch = window.fetch;
window.fetch = (input, init) => {
  const url = typeof input === 'string' ? input : input.url;
  if (url.includes('/__qa/') || url.includes('.csv')) return originalDeviceFetch(input, init);
  return previewFetch(input, init);
};
setInterval(async () => {
  try {
    const command = await (await originalDeviceFetch('/__qa/next')).json();
    if (!command.code) return;
    let result;
    try { result = {ok:true, value:await (0,eval)(command.code)}; }
    catch (error) { result = {ok:false,error:String(error)}; }
    result.id = command.id;
    await originalDeviceFetch('/__qa/result', {method:'POST',body:JSON.stringify(result)});
  } catch (_) {}
}, 300);
"""
html = html.replace("<head>", "<head><script>" + qa + "</script>", 1).encode()
commands = queue.Queue()
results = queue.Queue()
csv_data = ("time,value\n" + "123,hello\n" * 18000).encode()


class Handler(BaseHTTPRequestHandler):
    def respond(self, data, content_type="application/json"):
        if not isinstance(data, bytes):
            data = json.dumps(data).encode()
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        path = self.path.split("?")[0]
        if path == "/": return self.respond(html, "text/html; charset=utf-8")
        if path == "/api/profile/status": return self.respond({"setupMode": False, "profile": 1})
        if path.endswith(".csv"): return self.respond(csv_data, "text/csv")
        if path == "/__qa/next":
            try: self.respond(commands.get_nowait())
            except queue.Empty: self.respond({})
            return
        if path == "/__qa/result":
            try: self.respond(results.get_nowait())
            except queue.Empty: self.respond({"pending": True})
            return
        if path.startswith("/fonts/"):
            key = "LAB_GEIST_MONO_FONT" if "mono" in path else "LAB_GEIST_FONT"
            return self.respond(fonts.get(key, b""), "font/woff2")
        self.respond({})

    def do_POST(self):
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        if self.path == "/__qa/control": commands.put(json.loads(body))
        elif self.path == "/__qa/result": results.put(json.loads(body))
        self.respond({"ok": True})

    def log_message(self, fmt, *values):
        if not self.path.startswith("/__qa/"): super().log_message(fmt, *values)


print("Mock device listening on localhost:", args.port, flush=True)
ThreadingHTTPServer(("127.0.0.1", args.port), Handler).serve_forever()
