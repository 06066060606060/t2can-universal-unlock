"""Production profile bootstrap/load exercised with NVS read fault injection."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
PREAMBLE = r"""
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <set>
#include <vector>
#include <cstring>
#include <stdint.h>
std::map<std::string,std::map<std::string,unsigned>> disk;
bool failProfileOpen=false;
std::set<std::string> failKeys;
unsigned erases=0;
struct Preferences {
 std::string ns;
 bool begin(const char*n,bool ro){ns=n;if(ro&&ns=="v3profile"&&failProfileOpen)return false;if(ro&&!disk.count(ns))return false;if(!ro)disk[ns];return true;}
 unsigned get(const char*k,unsigned d){if(failKeys.count(ns+"/"+k))return d;auto&m=disk[ns];return m.count(k)?m[k]:d;}
 bool getBool(const char*k,bool d){return get(k,d)!=0;}
 uint8_t getUChar(const char*k,uint8_t d){return get(k,d);}
 size_t putBool(const char*k,bool v){disk[ns][k]=v;return 1;}
 size_t putUChar(const char*k,uint8_t v){disk[ns][k]=v;return 1;}
 void end(){}
};
using esp_err_t=int;
constexpr int ESP_OK=0;
int nvs_flash_init(){return ESP_OK;}
int nvs_flash_erase(){++erases;disk.clear();return ESP_OK;}

using nvs_handle_t=unsigned;
using nvs_type_t=unsigned;
constexpr nvs_type_t NVS_TYPE_U8=1,NVS_TYPE_U16=2,NVS_TYPE_U32=4,NVS_TYPE_BLOB=66;
constexpr int ESP_ERR_NVS_NOT_FOUND=1,ESP_ERR_NVS_TYPE_MISMATCH=2,ESP_ERR_NVS_INVALID_LENGTH=3,ESP_FAIL=-1;
enum nvs_open_mode_t{NVS_READONLY,NVS_READWRITE};
std::map<unsigned,std::string> handles;
unsigned nextHandle=0;
std::map<std::string,std::map<std::string,std::vector<uint8_t>>> blobs;
std::set<std::string> typedWrong;
bool failWrite=false;
bool blobDataReadError=false;
esp_err_t nvs_open(const char*ns,nvs_open_mode_t mode,nvs_handle_t*out){
 if(mode==NVS_READONLY&&std::string(ns)=="v3profile"&&failProfileOpen)return ESP_FAIL;
 if(!disk.count(ns)&&!blobs.count(ns)&&mode==NVS_READONLY)return ESP_ERR_NVS_NOT_FOUND;
 if(mode==NVS_READWRITE){disk[ns];}*out=++nextHandle;handles[*out]=ns;return ESP_OK;
}
void nvs_close(nvs_handle_t h){handles.erase(h);}
int rawGet(nvs_handle_t h,const char*k,unsigned&v){auto ns=handles[h];if(failKeys.count(ns+"/"+k))return ESP_FAIL;if(typedWrong.count(ns+"/"+k))return ESP_ERR_NVS_TYPE_MISMATCH;if(!disk[ns].count(k))return ESP_ERR_NVS_NOT_FOUND;v=disk[ns][k];return ESP_OK;}
int nvs_get_u8(nvs_handle_t h,const char*k,uint8_t*out){unsigned v=0;int r=rawGet(h,k,v);if(r==ESP_OK)*out=v;return r;}
int nvs_get_u16(nvs_handle_t h,const char*k,uint16_t*out){unsigned v=0;int r=rawGet(h,k,v);if(r==ESP_OK)*out=v;return r;}
int nvs_get_u32(nvs_handle_t h,const char*k,uint32_t*out){unsigned v=0;int r=rawGet(h,k,v);if(r==ESP_OK)*out=v;return r;}
int nvs_find_key(nvs_handle_t h,const char*k,nvs_type_t*t){auto ns=handles[h];if(failKeys.count(ns+"/"+k))return ESP_FAIL;if(blobs[ns].count(k)){*t=NVS_TYPE_BLOB;return ESP_OK;}if(disk[ns].count(k)){*t=NVS_TYPE_U8;return ESP_OK;}return ESP_ERR_NVS_NOT_FOUND;}
int nvs_get_blob(nvs_handle_t h,const char*k,void*out,size_t*len){auto ns=handles[h];if(failKeys.count(ns+"/"+k)||(out&&blobDataReadError))return ESP_FAIL;if(disk[ns].count(k))return ESP_ERR_NVS_TYPE_MISMATCH;if(!blobs[ns].count(k))return ESP_ERR_NVS_NOT_FOUND;auto&v=blobs[ns][k];if(out&&*len<v.size())return ESP_ERR_NVS_INVALID_LENGTH;*len=v.size();if(out)std::memcpy(out,v.data(),v.size());return ESP_OK;}
int nvs_set_blob(nvs_handle_t h,const char*k,const void*src,size_t len){if(failWrite)return ESP_FAIL;const auto*b=static_cast<const uint8_t*>(src);blobs[handles[h]][k]={b,b+len};return ESP_OK;}
int nvs_commit(nvs_handle_t){return failWrite?ESP_FAIL:ESP_OK;}

#define T2CAN_SERIAL_PRINTLN(x) ((void)0)
#define ARDUINO 1
#include "vehicle_profile.h"
"""
FOOTER = r"""void seed(){disk={{"v3profile",{{"univInit",1},{"profile",3},{"topology",3},{"turn",1}}},{"nag",{{"en",1},{"v",20}}},{"s3xyreg",{{"paired",1}}}};erases=0;failKeys.clear();failProfileOpen=false;vehicleProfileSetupMode=true;vehicleProfileNvsError=false;activeVehicleProfile=0;activeVehicleTopology=0;}
"""
BODY = r"""
#define CHECK(c) do {if(!(c)){std::fprintf(stderr,"assertion failed: %s line %d\n",#c,__LINE__);return 1;}}while(0)
int main(int argc,char**argv){
 if(argc!=2)return 2;
 std::string test=argv[1];seed();
 if(test=="open-error"){failProfileOpen=true;productionProfileBootProbe();CHECK(erases==0&&disk["nag"]["en"]==1&&disk["s3xyreg"]["paired"]==1&&vehicleProfileSetupMode);}
 else if(test=="two-read-errors"){failKeys={"v3profile/univInit","v3profile/profile"};productionProfileBootProbe();CHECK(erases==0&&disk["nag"]["en"]==1&&vehicleProfileSetupMode);}
 else if(test=="topology-error"){failKeys={"v3profile/topology"};productionProfileBootProbe();CHECK(erases==0&&vehicleProfileSetupMode&&activeVehicleTopology==0&&disk["v3profile"]["topology"]==3);}
 else if(test=="topology-type"){typedWrong={"v3profile/topology"};productionProfileBootProbe();CHECK(erases==0&&vehicleProfileSetupMode&&activeVehicleTopology==0);}
 else if(test=="healthy"){productionProfileBootProbe();CHECK(erases==0&&!vehicleProfileSetupMode&&activeVehicleProfile==3&&activeVehicleTopology==3&&disk["nag"]["en"]==1);}
 else if(test=="first-install"){disk.clear();productionProfileBootProbe();CHECK(erases==1&&vehicleProfileSetupMode&&disk["v3profile"]["univInit"]==1);}
 else if(test=="missing-topology"){disk["v3profile"].erase("topology");productionProfileBootProbe();CHECK(erases==0&&!vehicleProfileSetupMode&&activeVehicleTopology==2);}
 else if(test=="initialized-none"){disk["v3profile"]["profile"]=0;productionProfileBootProbe();CHECK(erases==0&&vehicleProfileSetupMode&&disk["nag"]["en"]==1);}
 else if(test=="missing-namespace"){
   CheckedPreferences p;CHECK(p.begin("absent",true)&&!p.exists());
   CHECK(p.getBool("bool",true)&&p.getUChar("u8",83)==83&&p.getUShort("u16",903)==903&&p.getUInt("u32",123456)==123456);
   CHECK(!p.isKey("none")&&p.getBytesLength("blob")==0&&p.ok());
   p.end();CHECK(p.ok()&&disk.count("absent")==0);
 }
 else if(test=="missing-key"){
   CheckedPreferences p;CHECK(p.begin("v3profile",true));
   uint32_t out=99;CHECK(!p.isKey("absent")&&p.getUInt("absent",77)==77&&p.getBytes("absent",&out,sizeof(out))==0&&out==99&&p.ok());
 }
 else if(test=="sticky-error"){
   CheckedPreferences p;CHECK(p.begin("v3profile",true));typedWrong={"v3profile/topology"};
   CHECK(p.getUChar("topology",0)==0&&!p.ok()&&p.error()==ESP_ERR_NVS_TYPE_MISMATCH);
   CHECK(p.getUInt("profile",99)==3&&!p.ok());p.end();CHECK(!p.ok());
   typedWrong.clear();CHECK(p.begin("v3profile",true)&&p.ok()&&p.getUShort("profile",99)==3);
 }
 else if(test=="blob-roundtrip"){
   const uint32_t desired=0x12345678u;uint32_t actual=0;bool found=true;
   CHECK(nvsCheckedReadBlob("absent","snap",&actual,sizeof(actual),found)&&!found&&actual==0);
   CHECK(nvsCheckedWriteBlob("atomic","snap",&desired,sizeof(desired)));
   CHECK(nvsCheckedReadBlob("atomic","snap",&actual,sizeof(actual),found)&&found&&actual==0x12345678u);
 }
 else if(test=="blob-size"||test=="blob-type"||test=="blob-read-error"){
   uint32_t desired=0x12345678u;CHECK(nvsCheckedWriteBlob("atomic","snap",&desired,sizeof(desired)));
   if(test=="blob-size")blobs["atomic"]["snap"].pop_back();
   if(test=="blob-type"){blobs["atomic"].erase("snap");disk["atomic"]["snap"]=1;}
   blobDataReadError=test=="blob-read-error";uint32_t actual=99;bool found=false;
   CHECK(!nvsCheckedReadBlob("atomic","snap",&actual,sizeof(actual),found)&&found&&actual==99);
 }
 else if(test=="blob-key-error"){
   uint32_t actual=99;bool found=false;failKeys={"v3profile/snap"};
   CHECK(!nvsCheckedReadBlob("v3profile","snap",&actual,sizeof(actual),found)&&actual==99);
 }
 else if(test=="blob-write-error"){
   uint32_t desired=123;failWrite=true;CHECK(!nvsCheckedWriteBlob("atomic","snap",&desired,sizeof(desired)));
   uint32_t actual=99;bool found=true;CHECK(nvsCheckedReadBlob("atomic","snap",&actual,sizeof(actual),found)&&!found&&actual==99);
 }
 else return 2;
 return 0;
}
"""

class ProfileNvsHostTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.scratch=tempfile.TemporaryDirectory(prefix="v3282-profile-nvs-")
        d=Path(cls.scratch.name)
        (d/"nvs.h").write_text("#pragma once\n")
        # Extract just the profile decision branch, retaining production statements.
        ino=(ROOT/(ROOT.name+".ino")).read_text()
        start=ino.index("      Preferences profileProbe;") if "      Preferences profileProbe;" in ino else ino.index("      CheckedPreferences profileProbe;")
        end=ino.index("\n    }\n  }",start)
        source=PREAMBLE+"void productionProfileBootProbe(){\n"+ino[start:end]+"\n}\n"+FOOTER+BODY
        (d/"probe.cpp").write_text(source)
        cls.exe=d/"probe"
        compiler=os.environ.get("CXX") or shutil.which("c++")
        built=subprocess.run([compiler,"-std=c++17","-Wall","-Wextra","-Werror","-Wno-unused-function","-Wno-unused-const-variable","-I",str(d),"-I",str(ROOT),str(d/"probe.cpp"),"-o",str(cls.exe)],capture_output=True,text=True)
        if built.returncode: raise RuntimeError("host profile compile failed:\n"+built.stderr)
    @classmethod
    def tearDownClass(cls): cls.scratch.cleanup()
    def run_case(self,name):
        result=subprocess.run([str(self.exe),name],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)
    def test_profile_open_error_preserves_entire_partition(self): self.run_case("open-error")
    def test_scalar_read_errors_cannot_trigger_first_install_erase(self): self.run_case("two-read-errors")
    def test_topology_io_error_inhibits_profile(self): self.run_case("topology-error")
    def test_topology_type_error_inhibits_profile(self): self.run_case("topology-type")
    def test_healthy_profile_restores_configured_routes(self): self.run_case("healthy")
    def test_true_first_install_migration_is_retained(self): self.run_case("first-install")
    def test_absent_topology_retains_v30_compatibility(self): self.run_case("missing-topology")
    def test_initialized_setup_profile_preserves_partition(self): self.run_case("initialized-none")
    def test_missing_namespace_retains_typed_defaults(self): self.run_case("missing-namespace")
    def test_missing_key_does_not_report_io_error(self): self.run_case("missing-key")
    def test_typed_error_stays_visible_until_next_begin(self): self.run_case("sticky-error")
    def test_atomic_blob_roundtrip_and_absence(self): self.run_case("blob-roundtrip")
    def test_blob_size_error_is_distinct_from_absence(self): self.run_case("blob-size")
    def test_blob_type_error_is_distinct_from_absence(self): self.run_case("blob-type")
    def test_blob_data_read_error_is_distinct_from_absence(self): self.run_case("blob-read-error")
    def test_blob_key_lookup_error_is_reported(self): self.run_case("blob-key-error")
    def test_blob_write_error_is_reported(self): self.run_case("blob-write-error")

if __name__=="__main__": unittest.main()
