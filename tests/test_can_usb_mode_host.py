"""Execute production USB mode transition and hold functions with offline doubles."""
from pathlib import Path
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


code = r'''
#include <cassert>
#include <cstdint>
#include <vector>
constexpr int pdTRUE=1,portMAX_DELAY=10000;
#define pdMS_TO_TICKS(x) (x)
bool canUsbPassiveMode=false,canUsbTransitionHeld=false,canUsbModePending=false;
bool canUsbModeResult=false,canUsbModeFinished=false,canTxAdministrativeRequested=false;
volatile bool canTxAdministrativeHold=false;
bool canMaintenanceRequested=false,canSubsystemBusy=false,canTasksStopping=false;
bool canTaskMcpQuiesced=false,canTaskTwaiQuiesced=false,mcpReady=true,twaiReady=true;
int canTxBarrierMutex=1;
void *canSupervisorHandle=(void*)1,*canTaskMcpHandle=(void*)2,*canTaskTwaiHandle=(void*)3;
uint32_t canUsbControllerEpoch=0,lastCanAFrameMs=100,lastCanBFrameMs=100,canInitTime=0,now=0;
bool locked=false,lockSuccess=true,taskProgress=true,aSuccess=true,bSuccess=true;
bool taskStartSuccess=true,saveSuccess=true,persisted=false,serviceProgress=true,inService=false;
bool selectedA=false,selectedB=false;
std::vector<int> events;
uint32_t millis(){return now;}
int xSemaphoreTake(int,int){if(!lockSuccess)return 0;assert(!locked);locked=true;return pdTRUE;}
void xSemaphoreGive(int){assert(locked);locked=false;}
void vTaskDelete(void* h){assert(canTaskMcpQuiesced&&canTaskTwaiQuiesced);events.push_back((int)(intptr_t)h);}
bool canUsbSaveMode(bool p){events.push_back(p?10:11);if(saveSuccess)persisted=p;return saveSuccess;}
void invalidateCanTxStateForFullRecovery(){events.push_back(20);assert(canTxAdministrativeHold);}
bool recoveryMcpColdInit(){events.push_back(21);selectedA=canUsbPassiveMode;mcpReady=aSuccess;return aSuccess;}
bool recoveryTwaiFullReinit(){events.push_back(22);selectedB=canUsbPassiveMode;twaiReady=bSuccess;if(bSuccess)++canUsbControllerEpoch;return bSuccess;}
bool recoveryStartCanTasks(){events.push_back(23);assert(!canTasksStopping);if(taskStartSuccess){canTaskMcpHandle=(void*)2;canTaskTwaiHandle=(void*)3;}return taskStartSuccess;}
static void canUsbModeService();
void vTaskDelay(int ticks){now+=ticks;if(inService){if(taskProgress)canTaskMcpQuiesced=canTaskTwaiQuiesced=true;}else if(serviceProgress&&canUsbModePending){inService=true;canUsbModeService();inService=false;}}
bool contains(int event){for(int e:events)if(e==event)return true;return false;}
void reset(){canUsbPassiveMode=canUsbTransitionHeld=canUsbModePending=canUsbModeResult=canUsbModeFinished=false;
 canTxAdministrativeRequested=canTxAdministrativeHold=canMaintenanceRequested=canSubsystemBusy=canTasksStopping=false;
 canTaskMcpQuiesced=canTaskTwaiQuiesced=false;mcpReady=twaiReady=true;
 canTxBarrierMutex=1;canSupervisorHandle=(void*)1;canTaskMcpHandle=(void*)2;canTaskTwaiHandle=(void*)3;
 canUsbControllerEpoch=now=0;lastCanAFrameMs=lastCanBFrameMs=100;canInitTime=0;
 locked=false;lockSuccess=taskProgress=aSuccess=bSuccess=taskStartSuccess=saveSuccess=serviceProgress=true;
 persisted=inService=selectedA=selectedB=false;events.clear();}
'''
code += extract('can_usb_logger.h', 'canUsbPassive')
code += extract('can_usb_logger.h', 'canUsbTxHeld')
code += extract('can_core.h', 'canMaintenanceActive')
code += extract('can_core.h', 'setCanTxAdministrativeHold')
code += extract('can_runtime.h', 'canUsbModeService')
code += extract('can_runtime.h', 'canUsbRequestMode')
code += r'''
int main(){
 reset();assert(canUsbRequestMode(true));assert(selectedA&&selectedB&&persisted);
 assert(canUsbPassive()&&canTxAdministrativeHold&&!canUsbTransitionHeld&&canUsbControllerEpoch==1);
 assert((events==std::vector<int>{10,2,3,20,21,22,23}));
 setCanTxAdministrativeHold(false);assert(canTxAdministrativeHold&&!canTxAdministrativeRequested);
 assert(canUsbRequestMode(false));assert(!selectedA&&!selectedB&&!persisted&&!canTxAdministrativeHold);
 assert(canUsbControllerEpoch==2&&lastCanAFrameMs==0&&lastCanBFrameMs==0);
 reset();setCanTxAdministrativeHold(true);assert(canUsbRequestMode(true));assert(canUsbRequestMode(false));assert(canTxAdministrativeHold&&canTxAdministrativeRequested);
 setCanTxAdministrativeHold(false);assert(!canTxAdministrativeHold);
 reset();canUsbPassiveMode=true;canTxBarrierMutex=0;setCanTxAdministrativeHold(false);assert(canTxAdministrativeHold);
 reset();assert(canUsbRequestMode(false));assert(events.empty()&&now==0);
 reset();lockSuccess=false;assert(!canUsbRequestMode(true));assert(!canUsbTransitionHeld&&!persisted&&events.empty());
 reset();saveSuccess=false;assert(!canUsbRequestMode(true));assert(canUsbTransitionHeld&&canTxAdministrativeHold&&!canUsbPassive());
 setCanTxAdministrativeHold(false);assert(canTxAdministrativeHold);
 reset();taskProgress=false;assert(!canUsbRequestMode(true));assert(!contains(2)&&!contains(21)&&canTasksStopping&&canUsbTransitionHeld&&persisted);
 reset();aSuccess=false;assert(!canUsbRequestMode(true));assert(contains(21)&&contains(22)&&!contains(23)&&canUsbTransitionHeld&&!mcpReady);
 reset();bSuccess=false;assert(!canUsbRequestMode(true));assert(!contains(23)&&canUsbTransitionHeld&&!twaiReady);
 reset();taskStartSuccess=false;assert(!canUsbRequestMode(true));assert(contains(23)&&canUsbTransitionHeld);
 reset();canUsbPassiveMode=true;persisted=true;canTxAdministrativeHold=true;saveSuccess=false;
 assert(!canUsbRequestMode(false));assert(!selectedA&&!selectedB&&persisted&&canUsbTransitionHeld&&canTxAdministrativeHold);
 reset();serviceProgress=false;assert(!canUsbRequestMode(true));assert(now==5000&&canUsbTransitionHeld&&canUsbModePending);
 reset();canMaintenanceRequested=true;assert(!canUsbRequestMode(true));assert(events.empty());
 reset();canTaskMcpHandle=0;assert(!canUsbRequestMode(true));assert(events.empty());
 reset();canUsbModePending=true;assert(!canUsbRequestMode(true));assert(events.empty());
}
'''
with tempfile.TemporaryDirectory(prefix='can-usb-mode-') as directory:
    source = Path(directory) / 'test.cpp'
    executable = Path(directory) / 'test'
    source.write_text(code)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', str(source), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print('USB mode host: 16 production transition, quiesce, persistence and sticky-hold scenarios passed')

# Boot and every cold/local controller recovery use the selected physical mode.
runtime = (ROOT / 'can_runtime.h').read_text()
ino = next(ROOT.glob('*.ino')).read_text()
assert 'canUsbPassive() ? Can_A.setListenOnlyMode() : Can_A.setNormalMode()' in extract('can_runtime.h', 'mcpInitChecked')
assert 'return mcpInitChecked();' in extract('can_runtime.h', 'mcpReinit')
assert 'canUsbPassive() ? TWAI_MODE_LISTEN_ONLY : TWAI_MODE_NORMAL' in extract('can_runtime.h', 'recoveryTwaiInstallFresh')
assert '__atomic_fetch_add(&canUsbControllerEpoch, 1U' in extract('can_runtime.h', 'recoveryTwaiInstallFresh')
assert 'canUsbPassive() ? TWAI_MODE_LISTEN_ONLY : TWAI_MODE_NORMAL' in ino
print('USB mode host: boot, local recovery and fresh reinstall mode selection verified')
