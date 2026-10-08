"""Execute production independent-country/map HTTP, atomic NVS and migration functions."""
from pathlib import Path
import os, re, subprocess, tempfile
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
#include <string>
#include <map>
#include "country_override_pure.h"
struct String : std::string {using std::string::string; int toInt() const {return std::stoi(*this);}};
struct Server {
 std::map<std::string,String> args; int status=0;
 bool hasArg(const char*k){return args.count(k);}
 String arg(const char*k){return args[k];}
 void send(int s,const char*,const String&){status=s;}
} server;
std::map<std::string,uint16_t> nvs;
bool nvsReadOpen=true,nvsWriteOpen=true,nvsWrite=true,lockOk=true,held=false;
int writes=0,countryOverrideMux=0,canTxBarrierMutex=1;
constexpr int portMAX_DELAY=1000,pdTRUE=1;
uint8_t activeVehicleProfile=2,activeVehicleTopology=3;
volatile uint8_t countryOverrideMode=0,countryOverrideMapMode=0;
volatile uint32_t countryOverrideCancelGeneration=1;
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
int xSemaphoreTake(int,int){if(!lockOk)return 0;assert(!held);held=true;return pdTRUE;}
void xSemaphoreGive(int){assert(held);held=false;}
struct Preferences {
 bool begin(const char*ns,bool read){assert(std::string(ns)=="countrylab");return read?nvsReadOpen:nvsWriteOpen;}
 bool isKey(const char*k){return nvs.count(k);}
 uint16_t getUShort(const char*k,uint16_t fallback){return nvs.count(k)?nvs[k]:fallback;}
 uint8_t getUChar(const char*k,uint8_t fallback){return nvs.count(k)?uint8_t(nvs[k]):fallback;}
 size_t putUShort(const char*k,uint16_t value){++writes;if(!nvsWrite)return 0;nvs[k]=value;return 2;}
 void end(){}
};
String countryOverrideStatsToJson(){return "{}";}
'''
for name in ('countryOverrideModeSnapshot', 'countryOverrideMapModeSnapshot',
             'countryOverrideSetModeWithBarrier', 'countryOverrideApplySelection',
             'countryOverrideCfgLoad'):
    code += extract('vehicle_logic.h', name)
code += extract('web_api.h', 'httpCountryOverrideUpdate')
code += r'''
void expect(uint8_t country,uint8_t map){assert(countryOverrideMode==country);assert(countryOverrideMapMode==map);}
void reboot(uint8_t country,uint8_t map){countryOverrideMode=255;countryOverrideMapMode=255;countryOverrideCfgLoad();expect(country,map);}
void request(const std::map<std::string,String>&args,int status){server.args=args;httpCountryOverrideUpdate();assert(server.status==status);}
int main(){
 // Missing keys and each released legacy preset keep their exact former effects.
 nvs.clear();reboot(0,0);
 assert(countryOverrideSetModeWithBarrier(0,0));expect(0,0);
 for(const auto& fixture: {std::pair<uint8_t,uint8_t>{0,0},{1,1},{2,2},{3,0},{4,0},{255,0}}){
  nvs={{"mode",fixture.first}};reboot(fixture.first<=3?fixture.first:0,fixture.second);
  assert(!nvs.count("selection"));
 }
 // Packed selection is authoritative over the older coupled preset.
 for(uint8_t country=0;country<4;++country)for(uint8_t map=0;map<3;++map){
  nvs={{"mode",2},{"selection",uint16_t(country|(uint16_t(map)<<8))}};reboot(country,map);
 }
 for(uint16_t bad: {uint16_t(4),uint16_t(255),uint16_t(0x0300),uint16_t(0x0104),uint16_t(0xFFFF)}){
  nvs={{"mode",2},{"selection",bad}};reboot(0,0);
 }
 nvsReadOpen=false;reboot(0,0);nvsReadOpen=true;
 // Exercise all independent valid HTTP combinations and actual NVS reload.
 for(int country=0;country<4;++country)for(int map=0;map<3;++map){
  request({{"countryMode",String(std::to_string(country).c_str())},{"mapMode",String(std::to_string(map).c_str())}},200);
  assert(nvs["selection"]==uint16_t(country|(map<<8)));reboot(country,map);
 }
 request({{"countryMode","0"}},200);expect(0,2);reboot(0,2);
 request({{"mapMode","1"}},200);expect(0,1);reboot(0,1);
 request({{"countryMode","3"}},200);expect(3,1);reboot(3,1);
 request({{"mapMode","0"}},200);expect(3,0);reboot(3,0);
 for(const auto*value:{"0","1","2","3"}){request({{"mode",value}},200);reboot(uint8_t(std::stoi(value)),0);}
 // Strict parser rejection must not write, publish or alter reboot state.
 const auto baseline=nvs;const int writesBefore=writes;
 for(const char*bad:{"4","255","-1","3x","3.0","03",""," 3","3 ","4294967296"}){
  request({{"countryMode",bad}},400);request({{"mode",bad}},400);expect(3,0);assert(nvs==baseline);
 }
 for(const char*bad:{"3","255","-1","2x","2.0","02",""," 2","2 ","4294967296"}){
  request({{"mapMode",bad}},400);expect(3,0);assert(nvs==baseline);
 }
 request({},400);request({{"countryMode","1"},{"mode","1"}},400);
 assert(writes==writesBefore);reboot(3,0);
 // NVS open/write failure retains both live selectors and old reboot selection.
 nvsWriteOpen=false;request({{"countryMode","1"},{"mapMode","2"}},503);expect(3,0);assert(nvs==baseline);reboot(3,0);
 nvsWriteOpen=true;nvsWrite=false;request({{"countryMode","1"},{"mapMode","2"}},503);expect(3,0);assert(nvs==baseline);reboot(3,0);nvsWrite=true;
 // Failed write before first migration also leaves the legacy reboot contract intact.
 nvs={{"mode",2}};reboot(2,2);nvsWrite=false;request({{"mapMode","0"}},503);expect(2,2);assert(!nvs.count("selection"));reboot(2,2);nvsWrite=true;
 const auto legacy=nvs;const int count=writes;
 // Real barrier-acquisition failure must refuse persistence as well as publication.
 const uint32_t beforeGeneration=countryOverrideCancelGeneration;lockOk=false;
 request({{"countryMode","1"},{"mapMode","0"}},503);expect(2,2);
 assert(nvs==legacy&&writes==count&&countryOverrideCancelGeneration==beforeGeneration&&!held);reboot(2,2);lockOk=true;
 canTxBarrierMutex=0;
 request({{"mapMode","0"}},503);expect(2,2);assert(nvs==legacy&&writes==count);reboot(2,2);canTxBarrierMutex=1;
 activeVehicleTopology=0;request({{"countryMode","1"}},409);assert(nvs==legacy&&writes==count);
 activeVehicleTopology=3;request({{"countryMode","1"},{"mapMode","0"}},200);expect(1,0);reboot(1,0);
 // Direct transaction boundary rejects invalid enums before persistent changes.
 const auto final=nvs;const int finalWrites=writes;
 assert(!countryOverrideApplySelection(4,0));assert(!countryOverrideApplySelection(0,3));
 assert(nvs==final&&writes==finalWrites);expect(1,0);
}
'''
with tempfile.TemporaryDirectory(prefix='country-map-api-host-') as directory:
    source=Path(directory)/'test.cpp'; binary=Path(directory)/'test'
    source.write_text(code)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-I',str(ROOT),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Country/Map production HTTP/NVS: 12 independent combinations, legacy migration, strict parsing, packed fail-closed and NVS runtime/reboot rollback PASS')
