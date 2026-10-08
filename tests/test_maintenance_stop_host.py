"""Exercise actual maintenance shutdown against mocked task/controller boundaries."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
def extract(file,name):
 text=(ROOT/file).read_text()
 m=re.search(r'^static\s+[^;{}]*?\b'+name+r'\([^;{}]*?\)\s*\{',text,re.M)
 assert m,name
 end=text.index('{',m.start())+1;depth=1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[m.start():end]+'\n'
code=r'''
#include <cassert>
#include <cstdint>
#include <vector>
using esp_err_t=int;
constexpr int ESP_OK=0,ESP_ERR_INVALID_ARG=0x102,ESP_ERR_INVALID_STATE=0x103,pdTRUE=1,portMAX_DELAY=10000;
constexpr int TWAI_STATE_RUNNING=0,TWAI_STATE_STOPPED=1,TWAI_STATE_BUS_OFF=2,TWAI_STATE_RECOVERING=3;
constexpr int MCP2515_RST=1,CAN_TX=2,CAN_RX=3,OUTPUT=1,INPUT=0,LOW=0;
#define pdMS_TO_TICKS(x) (x)
bool canMaintenanceRequested=false,canMaintenanceSupervisorParked=false,canMaintenancePreparing=false,canMaintenanceStopped=false;
volatile bool canTxAdministrativeHold=false;
bool canTxAdministrativeRequested=false,usbHeld=false;
bool vehicleProfileSetupMode=false,vehicleProfileNvsError=false;
bool canMaintenanceObservedCanRuntime=false;
bool canUsbTxHeld(){return usbHeld;}
int canSupervisorHandle=1,canTaskMcpHandle=2,canTaskTwaiHandle=3,canTxBarrierMutex=1,currentTask=9;
bool canTaskMcpQuiesced=false,canTaskTwaiQuiesced=false,mcpReady=true,twaiReady=true;
bool supervisorProgress=true,tasksProgress=true,lockSuccess=true;
int statusResult=ESP_OK,stopResult=ESP_OK,uninstallResult=ESP_OK,state=TWAI_STATE_RUNNING;
uint32_t now=0;std::vector<int> events;
struct twai_status_info_t{int state;};
uint32_t millis(){return now;}
int xTaskGetCurrentTaskHandle(){return currentTask;}
int xSemaphoreTake(int,int){events.push_back(1);return lockSuccess;}
void xSemaphoreGive(int){events.push_back(2);}
void vTaskDelay(int ticks){now+=ticks;if(supervisorProgress)canMaintenanceSupervisorParked=true;if(tasksProgress)canTaskMcpQuiesced=canTaskTwaiQuiesced=true;}
void pinMode(int pin,int){events.push_back(10+pin);}
void digitalWrite(int pin,int){assert(pin==MCP2515_RST);events.push_back(20);}
int twai_get_status_info(twai_status_info_t*s){events.push_back(30);s->state=state;return statusResult;}
int twai_stop(){events.push_back(31);if(stopResult==ESP_OK)state=TWAI_STATE_STOPPED;return stopResult;}
int twai_driver_uninstall(){events.push_back(32);if(uninstallResult==ESP_OK)statusResult=ESP_ERR_INVALID_ARG;return uninstallResult;}
void invalidateCanTxStateForFullRecovery(){events.push_back(40);}
bool contains(int n){for(auto e:events)if(e==n)return true;return false;}
void reset(){canMaintenanceObservedCanRuntime=false;vehicleProfileSetupMode=vehicleProfileNvsError=false;canMaintenanceRequested=canMaintenanceSupervisorParked=canMaintenancePreparing=canMaintenanceStopped=false;canTxAdministrativeHold=false;canSupervisorHandle=1;canTaskMcpHandle=2;canTaskTwaiHandle=3;canTxBarrierMutex=1;currentTask=9;canTaskMcpQuiesced=canTaskTwaiQuiesced=false;mcpReady=twaiReady=true;supervisorProgress=tasksProgress=lockSuccess=true;statusResult=stopResult=uninstallResult=ESP_OK;state=TWAI_STATE_RUNNING;now=0;events.clear();}
void setupOnlyReset(){
 reset();vehicleProfileSetupMode=true;
 canSupervisorHandle=canTaskMcpHandle=canTaskTwaiHandle=0;
 mcpReady=twaiReady=false;
 // ESP32 core 3.3.12 / IDF 5.5.5 libdriver.a returns INVALID_ARG for
 // twai_get_status_info_v2(NULL, valid_status), despite the header contract.
 statusResult=ESP_ERR_INVALID_ARG;
}
'''
code+=extract('can_core.h','canMaintenanceActive')+extract('can_core.h','setCanTxAdministrativeHold')+extract('can_runtime.h','prepareCanForMaintenance')
code+=r'''
int main(){
 reset();assert(prepareCanForMaintenance());assert(canMaintenanceStopped&&canMaintenanceRequested&&canTxAdministrativeHold&&!mcpReady&&!twaiReady);
 assert((events==std::vector<int>{1,2,11,20,30,31,32,12,13,40}));
 assert(statusResult==ESP_ERR_INVALID_ARG);
 events.clear();assert(prepareCanForMaintenance());assert(events.empty());
 setCanTxAdministrativeHold(false);assert(canTxAdministrativeHold);canTxBarrierMutex=0;setCanTxAdministrativeHold(false);assert(canTxAdministrativeHold);
 reset();supervisorProgress=false;assert(!prepareCanForMaintenance());assert(now==2000&&!contains(20)&&!contains(32)&&canTxAdministrativeHold&&!canMaintenancePreparing);
 reset();tasksProgress=false;assert(!prepareCanForMaintenance());assert(now>=500&&!contains(20)&&!contains(32));
 reset();lockSuccess=false;assert(!prepareCanForMaintenance());assert(!contains(20)&&!contains(32)&&canTxAdministrativeHold);
 reset();stopResult=9;assert(!prepareCanForMaintenance());assert(contains(20)&&contains(31)&&!contains(32)&&!canMaintenanceStopped);
 reset();uninstallResult=9;assert(!prepareCanForMaintenance());assert(contains(32)&&!contains(40));
 reset();state=TWAI_STATE_BUS_OFF;assert(prepareCanForMaintenance());assert(!contains(31)&&contains(32));
 reset();state=TWAI_STATE_STOPPED;assert(prepareCanForMaintenance());assert(!contains(31)&&contains(32));
 reset();state=TWAI_STATE_RECOVERING;assert(!prepareCanForMaintenance());assert(!contains(31)&&!contains(32));state=TWAI_STATE_STOPPED;assert(prepareCanForMaintenance());
 reset();statusResult=ESP_ERR_INVALID_STATE;assert(prepareCanForMaintenance());assert(!contains(32));
 reset();statusResult=9;assert(!prepareCanForMaintenance());assert(!contains(32));
 reset();canMaintenancePreparing=true;assert(!prepareCanForMaintenance());assert(events.empty()&&canMaintenanceRequested&&canMaintenancePreparing);
 reset();currentTask=canSupervisorHandle;supervisorProgress=false;assert(prepareCanForMaintenance());assert(canMaintenanceSupervisorParked);
 reset();canSupervisorHandle=canTaskMcpHandle=canTaskTwaiHandle=0;supervisorProgress=tasksProgress=false;assert(prepareCanForMaintenance());assert(now==0);
 reset();setCanTxAdministrativeHold(true);setCanTxAdministrativeHold(false);assert(!canTxAdministrativeHold);
 reset();usbHeld=true;setCanTxAdministrativeHold(false);assert(canTxAdministrativeHold);usbHeld=false;

 // Real blank-NVS setup: no tasks or drivers exist, but the TX barrier does.
 setupOnlyReset();assert(prepareCanForMaintenance());
 assert(canMaintenanceStopped&&canTxAdministrativeHold&&contains(40));
 assert(!contains(31)&&!contains(32)&&now==0);
 events.clear();assert(prepareCanForMaintenance());assert(events.empty());
 setupOnlyReset();vehicleProfileSetupMode=false;vehicleProfileNvsError=true;
 assert(prepareCanForMaintenance());assert(canMaintenanceStopped&&contains(40));

 // INVALID_ARG is not blanket permission to ignore an operational driver error.
 setupOnlyReset();vehicleProfileSetupMode=false;
 assert(!prepareCanForMaintenance());assert(!canMaintenanceStopped&&!contains(40));
 setupOnlyReset();mcpReady=true;
 assert(!prepareCanForMaintenance());assert(!canMaintenanceStopped&&!contains(40));
 assert(!prepareCanForMaintenance());assert(!canMaintenanceStopped&&!contains(40));
 setupOnlyReset();twaiReady=true;
 assert(!prepareCanForMaintenance());assert(!canMaintenanceStopped&&!contains(40));
 assert(!prepareCanForMaintenance());assert(!canMaintenanceStopped&&!contains(40));
 setupOnlyReset();canSupervisorHandle=1;
 assert(!prepareCanForMaintenance());assert(!canMaintenanceStopped&&!contains(40));
 setupOnlyReset();canTaskMcpHandle=2;
 assert(!prepareCanForMaintenance());assert(!canMaintenanceStopped&&!contains(40));
 setupOnlyReset();canTaskTwaiHandle=3;
 assert(!prepareCanForMaintenance());assert(!canMaintenanceStopped&&!contains(40));
 setupOnlyReset();statusResult=9;
 assert(!prepareCanForMaintenance());assert(!canMaintenanceStopped&&!contains(40));
 setupOnlyReset();lockSuccess=false;
 assert(!prepareCanForMaintenance());assert(canTxAdministrativeHold&&!contains(30));
 setupOnlyReset();statusResult=ESP_ERR_INVALID_STATE;
 assert(prepareCanForMaintenance());assert(canMaintenanceStopped&&!contains(32));

}
'''
with tempfile.TemporaryDirectory(prefix='maintenance-stop-') as d:
 p=Path(d)/'test.cpp';exe=Path(d)/'test';p.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror',str(p),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('maintenance stop host: 27 shutdown/setup-SDK/failure/idempotence/sticky-hold cases passed')

# Every software restart routes through the one verified shutdown wrapper.
files=list(ROOT.glob('*.h'))+list(ROOT.glob('*.ino'))
assert sum(f.read_text().count('ESP.restart();') for f in files)==1
runtime=(ROOT/'can_runtime.h').read_text()
assert runtime.count('while (canTasksStopping || canMaintenanceActive())')==2
supervisor=runtime.split('static void canSupervisorTask(void* arg) {',1)[1]
assert supervisor.index('if (canMaintenanceActive())') < supervisor.index('canRecoverySupervisorTick(now)')
assert 'while (!prepareCanForMaintenance())' in extract('can_runtime.h','restartT2CanSafely')
