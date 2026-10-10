"""Production NAG persistence fixture using IDF NVS fault seams."""
from pathlib import Path
import re
from test_v3282_profile_nvs_host import PREAMBLE
ROOT=Path(__file__).resolve().parents[1]
core=(ROOT/'can_core.h').read_text()
def extract(src,name):
 m=re.search(r'^static\s+[^;{}]*?\b'+name+r'\([^;{}]*?\)\s*\{',src,re.M);assert m,name
 i=src.index('{',m.start())+1;depth=1
 while depth:depth+=(src[i]=='{')-(src[i]=='}');i+=1
 return src[m.start():i]+'\n'
def make_code():
 code=PREAMBLE.replace('bool failProfileOpen=false;', 'bool failProfileOpen=false;bool failAnyOpen=false;bool failCommit=false;')
 code=code.replace('if(mode==NVS_READONLY&&std::string(ns)', 'if(failAnyOpen)return ESP_FAIL;\n if(mode==NVS_READONLY&&std::string(ns)')
 code=code.replace('return failWrite?ESP_FAIL:ESP_OK;', 'return failWrite||failCommit?ESP_FAIL:ESP_OK;')
 code=code.replace("#define ARDUINO 1", "").replace('#include "vehicle_profile.h"', '#include "vehicle_profile.h"\n#include "nvs_checked.h"')
 code+=r'''
#include <cstdlib>
#include "das_status_pure.h"
#include "tsl9_hands_on_0x399_pure.h"
#include "tsl9_input_scheduler_pure.h"
#include "nag_human_v4_pure.h"
#include "nag_mode_h_variant_pure.h"
#define T2CAN_SERIAL_DIAGNOSTICS 0
#define portENTER_CRITICAL(x) ((void)0)
#define portEXIT_CRITICAL(x) ((void)0)
struct TestPreferences {decltype(disk)&data=disk;bool&fail=failAnyOpen;bool&failWrite=::failWrite;}prefs;
bool activeProfileNagTorqueSupported(){return true;}
bool activeProfileNagTsl9Supported(){return true;}
void nagRxSelectorsRefreshFromConfig(){}
uint8_t nagHumanVariant=1;
NagHumanV4ConfigPure nagHumanV4Config=nagHumanV4DefaultConfigPure();
NagHumanV4StatePure nagHumanV4State={};
bool nagHumanV4Initialized=false;
static bool nagCfgSave();
'''
 code+=core[core.index('static const uint16_t NAG_TORQUE_RAW_MAX'):core.index('static portMUX_TYPE nagCfgMux')]
 for name in ('nagHumanRuntimeLoadDefaults','nagClampTorque','nagCfgSetCommonDefaults','nagCfgDefaultsModeA','nagCfgDefaultsModeB','nagCfgDefaultsModeC','nagCfgDefaultsModeH','nagCfgClampAll','nagCfgApplyActiveProfilePolicy','nagModePersistedIdSupported'):
  code+=extract(core,name)
 code+=core[core.index('// Versioned, single-key NVS commit.'):core.index('static void nagCfgLoad()')]
 code+='static NagScrollStoredSnapshot testScroll{false,60,0,false,60};\nstatic NagScrollStoredSnapshot nagScrollRuntimeSnapshot(){return testScroll;}\nstatic void nagScrollRuntimeRestore(const NagScrollStoredSnapshot&s){testScroll=s;}\n'
 code+=extract(core,'nagCfgLoad')+extract(core,'nagCfgSave')
 return code
