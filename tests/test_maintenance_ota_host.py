"""Execute production OTA/reboot handlers with a simulated maintenance barrier."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
src=(ROOT/'web_api.h').read_text()
def extract(name):
 start=src.index('static void '+name+'() {');i=src.index('{',start)+1;depth=1
 while depth:
  depth+=(src[i]=='{')-(src[i]=='}');i+=1
 return src[start:i]
code=r'''
#include <cassert>
#include <cstring>
#include <string>
using String=std::string;
enum { UPLOAD_FILE_START, UPLOAD_FILE_WRITE, UPLOAD_FILE_END, UPLOAD_FILE_ABORTED };
struct HTTPUpload {int status=0;String filename="fw.bin";unsigned char*buf=nullptr;unsigned currentSize=2,totalSize=2;} upload;
struct Server {HTTPUpload& upload(){return ::upload;}void sendHeader(const char*,const char*){}void send(int,const char*,const String&){} } server;
#define T2CAN_SERIAL_PRINTF(...) ((void)0)
#define T2CAN_SERIAL_PRINTLN(...) ((void)0)
#define UPDATE_SIZE_UNKNOWN 0
bool otaInProgress=false,otaSuccess=false,otaError=false, safe=true,stopped=false;
unsigned otaBytes=0,otaTotal=0;char otaErrMsg[160]={};int beginCalls=0,writes=0,reboots=0;
bool prepareCanForMaintenance(){stopped=safe;return safe;}
void delay(int){}
struct Updater {bool begin(int){++beginCalls;assert(stopped);return true;}const char*errorString(){return "error";}unsigned write(unsigned char*,unsigned n){++writes;assert(stopped);return n;}bool end(bool=false){return true;}void abort(){} } Update;
void restartT2CanSafely(){assert(stopped);++reboots;}
struct Esp {void restart(){assert(stopped);++reboots;}} ESP;
'''+extract('httpOtaUpload')+extract('httpOtaFinish')+extract('httpRebootT2Can')+r'''
int main(){
 upload.status=UPLOAD_FILE_START;httpOtaUpload();assert(stopped&&beginCalls==1);
 upload.status=UPLOAD_FILE_WRITE;httpOtaUpload();assert(writes==1);
 upload.status=UPLOAD_FILE_END;httpOtaUpload();assert(otaSuccess&&stopped);httpOtaFinish();assert(reboots==1);
 safe=false;stopped=false;upload.status=UPLOAD_FILE_START;httpOtaUpload();assert(otaError&&beginCalls==1);
 upload.status=UPLOAD_FILE_WRITE;httpOtaUpload();assert(writes==1);httpOtaFinish();assert(reboots==1);
 safe=true;upload.status=UPLOAD_FILE_START;httpOtaUpload();upload.status=UPLOAD_FILE_ABORTED;httpOtaUpload();assert(stopped&&otaError);
 stopped=false;httpRebootT2Can();assert(stopped&&reboots==2);
}
'''
with tempfile.TemporaryDirectory(prefix='ota-maint-host-') as d:
 p=Path(d)/'test.cpp';exe=Path(d)/'test';p.write_text(code)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror',str(p),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('OTA begin/write/end/abort and reboot barrier PASS')
