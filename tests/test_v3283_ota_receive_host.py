"""Compile real OTA handlers: delayed payload, retained failures and safe retry.
Removing the per-client timeout aborts a 6-second gap; losing first failure,
partial received count or stale-state reset fails the independent scenarios.
"""
from pathlib import Path
import os, subprocess, tempfile
from ota_host_fixture import JSON_WRITER
ROOT=Path(__file__).resolve().parents[1]
src=(ROOT/'web_api.h').read_text()
ota=src[src.index('// ─── OTA update'):src.index('static uint8_t researchCaptureParseSlot')]
code=r'''
#include <cassert>
#include <cstring>
#include <cstdint>
#include <string>
#include <algorithm>
using String=std::string;
enum { UPLOAD_FILE_START, UPLOAD_FILE_WRITE, UPLOAD_FILE_END, UPLOAD_FILE_ABORTED };
struct HTTPUpload {int status=0;String filename="fw.bin";unsigned char*buf=nullptr;unsigned currentSize=0,totalSize=0;} upload;
struct Client {unsigned timeout=5000;void setTimeout(unsigned n){timeout=n;} } client;
String response;int responseCode=0;
struct Server {HTTPUpload& upload(){return ::upload;}Client& client(){return ::client;}void sendHeader(const char*,const char*){}void send(int code,const char*,const String&body){responseCode=code;response=body;} } server;
'''+JSON_WRITER+r'''
#define T2CAN_SERIAL_PRINTF(...) ((void)0)
#define T2CAN_SERIAL_PRINTLN(...) ((void)0)
#define UPDATE_SIZE_UNKNOWN 0
bool otaInProgress=false,otaSuccess=false,otaError=false,safe=true,stopped=false;
uint32_t otaBytes=0,otaTotal=0;char otaErrMsg[64]={};const char*otaStage="idle";uint8_t otaUpdateError=0;uint32_t otaAttempt=0;bool otaUploadEnded=false;
int beginCalls=0,writes=0,reboots=0;bool failBegin=false,failWrite=false,failEnd=false,beginWithoutError=false;
bool prepareCanForMaintenance(){stopped=safe;return safe;}
void delay(int){}
struct Updater {int error=0;bool begin(int){++beginCalls;assert(stopped);error=failBegin&&!beginWithoutError?4:0;return !failBegin;}const char*errorString(){return error==4?"Not Enough Space":error==1?"Flash Write Failed":error==8?"Bad Magic Byte":"Aborted";}uint8_t getError(){return error;}unsigned write(unsigned char*,unsigned n){++writes;assert(stopped);error=failWrite?1:0;return failWrite?0:n;}bool end(bool=false){error=failEnd?8:0;return !failEnd;}void abort(){error=12;} } Update;
void restartT2CanSafely(){assert(stopped);++reboots;}
'''+ota+r'''
void event(int status,unsigned current=0,unsigned total=0){upload.status=status;upload.currentSize=current;upload.totalSize=total;httpOtaUpload();}
void start(){client.timeout=5000;event(UPLOAD_FILE_START);}
void finish(){event(UPLOAD_FILE_END,0,otaBytes);httpOtaFinish();}
void expectFailure(const char*stage,unsigned code,unsigned bytes){assert(otaError&&!otaSuccess);assert(std::string(otaStage)==stage);assert(otaUpdateError==code);assert(otaBytes==bytes);assert(reboots==0);}
int main(){
 start(); // SDK resets client timeout before parsing; production START must override it.
 if(client.timeout<6000)event(UPLOAD_FILE_ABORTED);else {event(UPLOAD_FILE_WRITE,2);event(UPLOAD_FILE_END,0,2);}
 assert(otaSuccess && "OTA payload with a 6-second idle gap must reach success");
 assert(client.timeout==30000&&otaAttempt==1);assert(stopped&&beginCalls==1&&writes==1);
 // A reconnect/GET is read-only and must retain an aborted partial upload.
 start();event(UPLOAD_FILE_WRITE,4);event(UPLOAD_FILE_ABORTED,3,4);
 expectFailure("abort",12,7);assert(!otaInProgress);assert(stopped);
 const auto bytes=otaBytes;const auto code=otaUpdateError;const auto stage=std::string(otaStage);
 httpOtaStatus();assert(responseCode==200);assert(response.find("\"receivedBytes\":7")!=String::npos);
 assert(otaBytes==bytes&&otaUpdateError==code&&otaStage==stage&&otaAttempt==2);
 // Trailing multipart abort after END must revoke success without double count.
 start();event(UPLOAD_FILE_WRITE,5);event(UPLOAD_FILE_END,5,5);
 event(UPLOAD_FILE_ABORTED,5,5);expectFailure("abort",12,5);httpOtaFinish();assert(reboots==0);
 // Failed begin can have error 0 when the SDK cannot allocate its buffer.
 failBegin=true;beginWithoutError=true;start();
 expectFailure("begin",0,0);assert(std::string(otaErrMsg).find("failed")!=String::npos);
 beginWithoutError=false;failBegin=false;
 // Preserve first error even when the parser subsequently aborts.
 failBegin=true;start();event(UPLOAD_FILE_WRITE,2);event(UPLOAD_FILE_ABORTED,1,2);
 expectFailure("begin",4,3);assert(std::string(otaErrMsg)=="Not Enough Space");failBegin=false;
 failWrite=true;start();event(UPLOAD_FILE_WRITE,9);event(UPLOAD_FILE_ABORTED,0,9);
 expectFailure("write",1,9);assert(std::string(otaErrMsg)=="Flash Write Failed");failWrite=false;
 failEnd=true;start();event(UPLOAD_FILE_WRITE,5);finish();expectFailure("end",8,5);failEnd=false;
 safe=false;const int oldBegin=beginCalls,oldWrite=writes;start();event(UPLOAD_FILE_WRITE,2);finish();
 expectFailure("maintenance",0,2);assert(beginCalls==oldBegin&&writes==oldWrite);
 safe=true;start();assert(!otaError&&!otaSuccess&&otaTotal==0&&otaBytes==0&&otaUpdateError==0&&otaErrMsg[0]=='\0');
 event(UPLOAD_FILE_WRITE,6);finish();assert(otaSuccess&&!otaError&&otaBytes==6&&otaTotal==6&&reboots==1&&stopped);
 assert(response.find("\"ok\":true")!=String::npos);
}
'''
with tempfile.TemporaryDirectory(prefix='v3283-ota-host-') as d:
 p=Path(d)/'test.cpp';exe=Path(d)/'test';p.write_text(code)
 # Baseline does not have the read-only handler yet; compile a no-op fallback
 # solely to reach the receive behavior assertion, never to satisfy diagnostics.
 if 'static void httpOtaStatus()' not in ota:
  p.write_text(code.replace('void event(int status','void httpOtaStatus(){}\nvoid event(int status'))
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror',str(p),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('OTA production receive timeout, failure stages, partial bytes, read-only reconnect and retry PASS')
