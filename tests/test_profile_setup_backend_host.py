"""Run production setup HTTP + shutdown + restart with the SDK's absent-driver code."""
from pathlib import Path
import ast
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def extract(file, name):
    text = (ROOT / file).read_text()
    match = re.search(r'^static\s+[^;{}]*?\b' + name + r'\([^;{}]*?\)\s*\{', text, re.M)
    assert match, name
    end = text.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# Reuse only the literal boundary fixture, never import/execute its test suite.
fixture = ast.parse((ROOT / 'tests/test_maintenance_stop_host.py').read_text())
code = next(ast.literal_eval(node.value) for node in fixture.body
            if isinstance(node, ast.Assign)
            and any(isinstance(t, ast.Name) and t.id == 'code' for t in node.targets))
code += r'''
#include <map>
#include <string>
constexpr int VEHICLE_MODEL_YL=1, VEHICLE_MODEL_3_HIGHLAND=4;
constexpr int TURN_SIGNAL_UNSET=0, VEHICLE_TOPOLOGY_STANDARD_PARTY_CHASSIS=3;
struct Arg { std::string text; int toInt() const {return std::stoi(text);} };
struct Server {
 std::map<std::string,std::string> args; int status=0; std::string body;
 bool hasArg(const char* k){return args.count(k);}
 Arg arg(const char* k){return {args.at(k)};}
 void send(int s,const char*,const char* b){status=s;body=b;}
} server;
int saves=0,reboots=0; bool saveOk=true;
bool vehicleProfileValid(uint8_t p){return p==1;}
uint8_t vehicleProfileDefaultTopology(uint8_t){return 1;}
bool vehicleProfileTopologyValid(uint8_t p,uint8_t t){return p==1&&t==1;}
uint8_t vehicleProfileDefaultTurn(uint8_t){return 1;}
bool vehicleProfileTurnValid(uint8_t p,uint8_t t,uint8_t v){return p==1&&t==1&&v==1;}
bool vehicleProfileSave(uint8_t p,uint8_t t,uint8_t v){
 assert(p==1&&t==1&&v==1);assert(canMaintenanceStopped&&canTxAdministrativeHold);
 ++saves;return saveOk;
}
void delay(int n){now+=n;}
struct Esp {void restart(){assert(canMaintenanceStopped&&canTxAdministrativeHold);++reboots;}} ESP;
void resetSetup(){
 reset();vehicleProfileSetupMode=true;vehicleProfileNvsError=false;
 canSupervisorHandle=canTaskMcpHandle=canTaskTwaiHandle=0;mcpReady=twaiReady=false;
 supervisorProgress=tasksProgress=false;statusResult=ESP_ERR_INVALID_ARG;
 saves=reboots=0;saveOk=true;server.status=0;server.body.clear();server.args={{"profile","1"}};
}
'''
code += extract('can_core.h', 'canMaintenanceActive')
code += extract('can_core.h', 'setCanTxAdministrativeHold')
code += extract('can_runtime.h', 'prepareCanForMaintenance')
code += extract('can_runtime.h', 'restartT2CanSafely')
code += extract('web_api.h', 'httpProfileSelect')
code += r'''
int main(){
 (void)TWAI_STATE_BUS_OFF;
 // The real SDK v2 wrapper returns INVALID_ARG when its driver handle is null.
 resetSetup();httpProfileSelect();
 assert(server.status==200&&saves==1&&reboots==1);
 assert(server.body=="{\"ok\":true,\"rebooting\":true}");
 assert(!contains(31)&&!contains(32));
 // Reboot-only maintenance uses the same safe setup boundary.
 resetSetup();restartT2CanSafely();assert(reboots==1&&saves==0);
 // Persistence failure must never reboot or reopen the TX gate.
 resetSetup();saveOk=false;httpProfileSelect();
 assert(server.status==500&&saves==1&&reboots==0&&canTxAdministrativeHold);
 // Outside empty setup, INVALID_ARG remains a failed controller handshake.
 resetSetup();vehicleProfileSetupMode=false;httpProfileSelect();
 assert(server.status==503&&saves==0&&reboots==0&&canTxAdministrativeHold);
 // Readiness captured before prepare clears it must preclude the exception.
 resetSetup();twaiReady=true;httpProfileSelect();assert(server.status==503&&saves==0&&reboots==0);
 resetSetup();mcpReady=true;httpProfileSelect();assert(server.status==503&&saves==0&&reboots==0);
 resetSetup();canTaskTwaiHandle=3;tasksProgress=true;httpProfileSelect();assert(server.status==503&&saves==0&&reboots==0);
 // An unrelated SDK error is never reclassified as absent-driver success.
 resetSetup();statusResult=0x105;httpProfileSelect();assert(server.status==503&&saves==0&&reboots==0);
 // NVS recovery may restart but cannot save a profile before storage recovers.
 resetSetup();vehicleProfileNvsError=true;httpProfileSelect();assert(server.status==503&&saves==0&&reboots==0);
 resetSetup();vehicleProfileSetupMode=false;vehicleProfileNvsError=true;
 restartT2CanSafely();assert(reboots==1&&saves==0);
}
'''
with tempfile.TemporaryDirectory(prefix='profile-setup-backend-') as directory:
    source = Path(directory) / 'test.cpp'
    executable = Path(directory) / 'test'
    source.write_text(code)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra',
                    '-Werror', str(source), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True, timeout=10)
print('profile setup backend host: 10 production HTTP/maintenance/restart cases passed')
