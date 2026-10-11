"""Execute production registration statements against the HTTP registry boundary."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
api = (ROOT / 'web_api.h').read_text()
registrations = re.findall(r'^\s*server\.on\([^\n]+\);', api, re.M)
assert registrations, 'production HTTP registrations unavailable'
handlers = sorted(set(re.findall(r'\b(http\w+)\b', '\n'.join(registrations))))
code = r'''
#include <cassert>
#include <string>
#include <map>
#include <utility>
constexpr int HTTP_GET=0,HTTP_POST=1;
struct Registry {
 std::map<std::pair<std::string,int>,void(*)()> routes;
 void on(const char*path,int method,void(*handler)()) { routes[{path,method}]=handler; }
 void on(const char*path,int method,void(*handler)(),void(*)()) { on(path,method,handler); }
 int request(const char*path,int method) { auto it=routes.find({path,method});if(it==routes.end())return 404;it->second();return 200; }
} server;
'''
code += '\n'.join('void ' + name + '() {}' for name in handlers)
code += '\nint main() {\n' + '\n'.join(registrations)
code += r'''
 for(const char*path:{"/api/lab/vision-speed/stats","/api/lab/vision-speed/update",
                     "/api/lab/adaptive-speed/stats","/api/lab/adaptive-speed/update",
                     "/api/lab/lane-graph/stats","/api/lab/lane-graph/update",
                     "/api/lab/parked-injection/stats","/api/lab/parked-injection/update",
                     "/api/lab/can-a-rx/stats","/api/lab/can-a-rx/update",
                     "/api/lab/ulc-monitor/stats","/api/can/3fd-timing.csv",
                     "/api/can/3fd-timing/capture"}) {
   assert(server.request(path,HTTP_GET)==404);
   assert(server.request(path,HTTP_POST)==404);
 }
 assert(server.request("/api/lab/vision-control/stats",HTTP_GET)==200);
 assert(server.request("/api/lab/vision-control/update",HTTP_POST)==200);
 assert(server.request("/api/vision-control/stats",HTTP_GET)==200);
 assert(server.request("/api/vision-control/update",HTTP_POST)==200);
 assert(server.request("/api/ulc/stats",HTTP_GET)==200);
 assert(server.request("/api/ulc/update",HTTP_POST)==200);
}
'''
with tempfile.TemporaryDirectory() as temp:
    source = Path(temp) / 'test.cpp'
    binary = Path(temp) / 'test'
    source.write_text(code)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS retired speed/LAB/timing routes return 404; Visual Speed Control production/compatibility and ULC registrations preserved')
