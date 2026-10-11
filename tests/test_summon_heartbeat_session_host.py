"""Execute the RAM-only heartbeat session transaction and recovery reset."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
LOGIC = (ROOT / "vehicle_logic.h").read_text()


def extract(name):
    match = re.search(r"^static\s+[^;{}]*?\b" + name + r"\([^;{}]*?\)\s*\{", LOGIC, re.M)
    assert match, name
    end = LOGIC.index("{", match.start()) + 1
    depth = 1
    while depth:
        depth += (LOGIC[end] == "{") - (LOGIC[end] == "}")
        end += 1
    return LOGIC[match.start() : end] + "\n"


code = r'''
#include <cassert>
#include <cstdint>
#include "vehicle_profile.h"
constexpr int pdTRUE=1,portMAX_DELAY=1000;
int canTxBarrierMutex=1,lab3f8Mux=0;
bool lockOk=true,held=false,labMenuEnabled=true;
uint8_t activeVehicleProfile=1,activeVehicleTopology=1;
volatile uint32_t lab3f8Generation=1;
volatile bool summonHeartbeatOverrideEnabled=false;
volatile uint8_t summonHeartbeatOverrideValue=2;
volatile bool summonHeartbeatLastAppliedValid=false;
volatile uint8_t summonHeartbeatLastAppliedValue=0xFF;
volatile uint32_t summonHeartbeatLastAppliedMs=0;
int xSemaphoreTake(int,int){if(!lockOk)return 0;assert(!held);held=true;return pdTRUE;}
void xSemaphoreGive(int){assert(held);held=false;}
void portENTER_CRITICAL(int*){}
void portEXIT_CRITICAL(int*){}
'''
for function in ("summonHeartbeatOverrideSupported", "summonHeartbeatOverrideApply",
                 "summonHeartbeatOverrideResetUnderTxBarrier"):
    code += extract(function)
code += r'''
int main(){
  assert(!summonHeartbeatOverrideEnabled&&summonHeartbeatOverrideValue==2);
  for(uint8_t value=0;value<4;++value){const uint32_t before=lab3f8Generation;assert(summonHeartbeatOverrideApply(true,value));assert(summonHeartbeatOverrideEnabled&&summonHeartbeatOverrideValue==value&&lab3f8Generation==before+1&&!held);}
  const uint32_t generation=lab3f8Generation;const uint8_t value=summonHeartbeatOverrideValue;
  assert(!summonHeartbeatOverrideApply(true,4));assert(summonHeartbeatOverrideEnabled&&summonHeartbeatOverrideValue==value&&lab3f8Generation==generation&&!held);
  lockOk=false;assert(!summonHeartbeatOverrideApply(false,1));assert(summonHeartbeatOverrideEnabled&&summonHeartbeatOverrideValue==value&&lab3f8Generation==generation);lockOk=true;
  labMenuEnabled=false;assert(!summonHeartbeatOverrideApply(true,1));assert(summonHeartbeatOverrideApply(false,1));assert(!summonHeartbeatOverrideEnabled&&summonHeartbeatOverrideValue==1);
  labMenuEnabled=true;activeVehicleTopology=0;assert(!summonHeartbeatOverrideApply(true,2));activeVehicleTopology=1;
  assert(summonHeartbeatOverrideApply(true,3));summonHeartbeatLastAppliedValid=true;summonHeartbeatLastAppliedValue=3;summonHeartbeatLastAppliedMs=50;
  const uint32_t resetGeneration=lab3f8Generation;summonHeartbeatOverrideResetUnderTxBarrier();assert(!summonHeartbeatOverrideEnabled&&!summonHeartbeatLastAppliedValid&&summonHeartbeatLastAppliedValue==0xFF&&summonHeartbeatLastAppliedMs==0&&lab3f8Generation==resetGeneration+1);
}
'''

with tempfile.TemporaryDirectory(prefix="summon-heartbeat-session-host-") as directory:
    source = Path(directory) / "test.cpp"
    binary = Path(directory) / "test"
    source.write_text(code)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT), str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

print("Summon Heartbeat session: barrier publication, invalid rollback and recovery reset PASS")
