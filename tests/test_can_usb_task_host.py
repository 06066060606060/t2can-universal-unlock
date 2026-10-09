"""Run the production USB worker against chunked, disconnectable offline CDC."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
code = r'''
#include <cassert>
#include <cstdint>
#include <string>
#include <deque>
#include <vector>
#include <algorithm>
#include "can_usb_logger_pure.h"
#define FW_VERSION "v3.28.0"
#define pdMS_TO_TICKS(x) (x)
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
constexpr int ESP_OK=0;
int canUsbMux=0;
bool canUsbCapturing=false,canSubsystemBusy=false;
uint32_t canUsbControllerEpoch=4,canUsbMcpOverflowTotal=2,now=0;
CanUsbQueue<512> canUsbQueue;
struct twai_status_info_t{uint32_t rx_missed_count=3,rx_overrun_count=1;};
int twai_get_status_info(twai_status_info_t*){return ESP_OK;}
uint32_t millis(){return now;}
bool canUsbRequestMode(bool){return true;}
struct Finished{};
int scenario=0,startCount=0,ctrlCount=0,steps=0,flushes=0;
bool disconnectedOnce=false,done=false;
std::vector<std::string> lines;
void injectFrames(){for(int i=0;i<3;++i){CanUsbFrame f{};f.id=0x100+i;f.dlc=1;f.data[0]=(uint8_t)i;f.timestampUs=i+1;assert(canUsbQueue.observe(f));}}
struct FakeSerial{
 bool connected=true;
 std::deque<char> input;
 std::string output,currentLine;
 operator bool()const{return connected;}
 void add(const std::string&s){for(char c:s)input.push_back(c);}
 int available()const{return (int)input.size();}
 int read(){int c=input.front();input.pop_front();return c;}
 int availableForWrite()const{return 3;}
 void flush(){++flushes;input.clear();}
 size_t write(const uint8_t*data,size_t n){
  for(size_t i=0;i<n;++i){
   const char c=(char)data[i];output+=c;currentLine+=c;
   if(c=='\n'){
    lines.push_back(currentLine);const std::string line=currentLine;currentLine.clear();
    if(line.rfind("@START,",0)==0){++startCount;injectFrames();if(scenario==1)now=2000;}
    if(line.rfind("@CTRL,",0)==0){++ctrlCount;if((scenario==0&&ctrlCount==1)||(scenario==1&&ctrlCount==2)||(scenario==2&&startCount==2))add("LOGGER STOP\n");}
    if(line.rfind("@STOP,",0)==0){done=true;}
   }
  }
  if(scenario==2&&startCount==1&&!disconnectedOnce&&currentLine.rfind("@CA",0)==0){
   disconnectedOnce=true;connected=false;add("LOGGER STA");
  }
  return n;
 }
}Serial;
void vTaskDelay(int ticks){
 now+=ticks;
 if(++steps>20000)assert(false&&"worker failed to finish");
 if(scenario==2&&!Serial.connected&&flushes){Serial.currentLine.clear();Serial.connected=true;Serial.add("LOGGER START PASSIVE\n");}
 if(done)throw Finished{};
}
'''
code += (ROOT / 'can_usb_logger_task.h').read_text().replace('#pragma once', '')
code += r'''
void reset(int value){scenario=value;startCount=ctrlCount=steps=flushes=0;disconnectedOnce=done=false;lines.clear();now=0;canUsbCapturing=false;canUsbQueue={};Serial={};Serial.add("LOGGER START PASSIVE\n");}
void run(){try{canUsbTask(nullptr);}catch(const Finished&){}assert(done);}
void verifyFinal(int expectedSession){
 size_t start=lines.size(),stop=lines.size(),lastCan=0;int count=0;
 const std::string startPrefix="@START,"+std::to_string(expectedSession)+",";
 const std::string canPrefix="@CAN,"+std::to_string(expectedSession)+",";
 for(size_t i=0;i<lines.size();++i){if(lines[i].rfind(startPrefix,0)==0)start=i;if(lines[i].rfind(canPrefix,0)==0){++count;lastCan=i;assert(i>start);}if(lines[i].rfind("@STOP,",0)==0)stop=i;}
 assert(count==3&&lastCan<stop);
 assert(lines[stop]=="@STOP,"+std::to_string(expectedSession)+",3,3,0,0\n");
 assert(!canUsbCapturing&&!canUsbQueue.active&&canUsbQueue.count==0);
}
int main(){
 reset(0);run();verifyFinal(1);
 assert(lines.front()=="\n"&&lines[1]=="@HELLO,1,v3.28.0\n");
 assert(lines[3]=="@CTRL,1,4,2,3,1,1\n");
 reset(1);run();verifyFinal(1); // STOP arrives between periodic CTRL and its STAT.
 assert(std::find(lines.begin(),lines.end(),"@STAT,1,3,3,0,3\n")!=lines.end());
 reset(2);run();verifyFinal(2);
 assert(flushes==1&&startCount==2&&disconnectedOnce);
 assert(Serial.output.find("@ERROR,session_busy")==std::string::npos);
 assert(Serial.output.find("@ERROR,unknown_command")==std::string::npos);
 // A truncated old frame is separated by a newline and a fresh HELLO.
 assert(Serial.output.find("\n@HELLO,1,v3.28.0\n",Serial.output.find("@CA"))!=std::string::npos);
}
'''
with tempfile.TemporaryDirectory(prefix='can-usb-task-') as directory:
    source = Path(directory) / 'test.cpp'
    executable = Path(directory) / 'test'
    source.write_text(code)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT), str(source), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print('USB task host: chunked START/CAN ordering, STOP drain, periodic report race and partial-disconnect restart passed')
