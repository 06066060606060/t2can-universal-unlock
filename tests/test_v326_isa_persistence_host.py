"""Exercise standalone ISA Suppression transaction and legacy migration ownership."""

from pathlib import Path
import os
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
LOGIC = (ROOT / "vehicle_logic.h").read_text(encoding="utf-8")


def function(name: str) -> str:
    match = re.search(
        r"^static\s+[^;{}]*?\b" + name + r"\([^;{}]*?\)\s*\{",
        LOGIC,
        re.M,
    )
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
#include <string>

constexpr int pdTRUE=1,portMAX_DELAY=-1;
int canTxBarrierMutex=1,isaSuppressionControlMux=0;
bool held=false,busy=false,beginOk=true,putOk=true;
unsigned takeCalls=0,giveCalls=0,putCalls=0;
bool isaSuppressionEnabled=true;
volatile uint32_t isaSuppressionGeneration=0;
void portENTER_CRITICAL(int*){}
void portEXIT_CRITICAL(int*){}
int xSemaphoreTake(int,int){++takeCalls;if(busy)return 0;assert(!held);held=true;return 1;}
void xSemaphoreGive(int){assert(held);held=false;++giveCalls;}
struct Preferences{
 bool begin(const char*,bool){return beginOk;}
 size_t putBool(const char*,bool){assert(held);++putCalls;return putOk?1u:0u;}
 void end(){}
};
'''
code += function("isaSuppressionControlApply")
code += r'''
void reset(){held=busy=false;beginOk=putOk=true;takeCalls=giveCalls=putCalls=0;isaSuppressionEnabled=true;isaSuppressionGeneration=0;}
int main(){
 reset();busy=true;assert(!isaSuppressionControlApply(false));
 assert(isaSuppressionEnabled&&putCalls==0&&takeCalls==1&&giveCalls==0);
 reset();beginOk=false;assert(!isaSuppressionControlApply(false));
 assert(isaSuppressionEnabled&&putCalls==0&&takeCalls==1&&giveCalls==1&&!held);
 reset();putOk=false;assert(!isaSuppressionControlApply(false));
 assert(isaSuppressionEnabled&&putCalls==1&&takeCalls==1&&giveCalls==1&&!held);
 reset();assert(isaSuppressionControlApply(false));
 assert(!isaSuppressionEnabled&&isaSuppressionGeneration==1&&putCalls==1&&takeCalls==1&&giveCalls==1&&!held);
}
'''

with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / "isa_suppression.cpp"
    executable = Path(directory) / "isa_suppression"
    source.write_text(code, encoding="utf-8")
    subprocess.run(
        [
            os.environ.get("CXX", "c++"),
            "-std=c++17",
            "-Wall",
            "-Wextra",
            "-Werror",
            str(source),
            "-o",
            str(executable),
        ],
        check=True,
    )
    subprocess.run([str(executable)], check=True)

load = LOGIC[LOGIC.index("static void featureCfgLoad()") : LOGIC.index("static bool blinkerTxModePersist")]
save = LOGIC[LOGIC.index("static void featureCfgSave()") : LOGIC.index("static void summonCfgLoad()")]
assert 'isKey("isaSuppress")' in load
assert 'getBool("isaSuppress", false)' in load
assert 'getBool("tsl9isa", false)' in load
assert load.index('isKey("isaSuppress")') < load.index('getBool("tsl9isa", false)')
assert 'putBool("isaSuppress"' not in save

print("Standalone ISA Suppression NVS, migration, and TX-barrier transaction PASS")
