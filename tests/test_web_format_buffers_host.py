"""Exercise production formatting adapters, nested labels and CSV boundaries."""
from pathlib import Path
import os, re, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
web=(ROOT/'web_api.h').read_text()
def function(name):
 m=re.search(r'^static[^;{}]*\b'+name+r'\([^;{}]*\)\s*\{',web,re.M);assert m,name
 e=web.index('{',m.start())+1;depth=1
 while depth:depth+=(web[e]=='{')-(web[e]=='}');e+=1
 return web[m.start():e]+'\n'
code=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
struct String:std::string{
 using std::string::string;
 String(const std::string&s):std::string(s){}
 void remove(size_t pos){erase(pos);}
 void concat(const char*s,size_t n){append(s,n);}
 String& operator+=(unsigned long n){append(std::to_string(n));return *this;}
 String& operator+=(long n){append(std::to_string(n));return *this;}
 using std::string::operator+=;
};
#include "text_format_arduino.h"
#include "json_writer_arduino.h"
struct Server{std::string bytes;std::vector<size_t> chunks;void sendContent(const char*p,size_t n){bytes.append(p,n);chunks.push_back(n);}}server;
static constexpr size_t RESEARCH_CAPTURE_CSV_CHUNK_BYTES=8192;
'''
for fn in ['wifiApJsonEscape','researchCaptureJsonEscape','lab3f8RawHex','canTxTraceRawHex','researchCaptureCsvHex','researchCaptureCsvFlush','researchCaptureCsvAppend']:code+=function(fn)
a=web.index('  jw.beginObject("labels");');b=web.index('  jw.u32("samples"',a)
code+='''int main(){
 const uint8_t bytes[]={0,15,16,127,128,239,254,255};
 assert(lab3f8RawHex(bytes,255)=="00 0F 10 7F 80 EF FE FF");
 assert(lab3f8RawHex(nullptr,8).empty());
 String binary("a\\0b",3);assert(wifiApJsonEscape(binary)==binary);
 assert(researchCaptureJsonEscape(nullptr).empty());
 for(size_t n=0;n<30;++n){char out[32];memset(out,0x55,sizeof(out));researchCaptureCsvHex(bytes,8,out,n);if(n)assert(strlen(out)<n);assert(out[n]==0x55);}
 for(size_t n:{size_t(1),size_t(8191),size_t(8192),size_t(8193)}){
  String chunk;chunk.reserve(8192);server.bytes.clear();server.chunks.clear();
  const std::string first(100,'a'),line(n,'b');
  researchCaptureCsvAppend(chunk,first.c_str());researchCaptureCsvAppend(chunk,line.c_str());
  assert(chunk.length()<=8192);researchCaptureCsvFlush(chunk);assert(server.bytes==first+line);
 }
 String j;JsonWriterArduino jw(j);jw.boolean("before",true);
 constexpr uint8_t RESEARCH_CAPTURE_LABEL_SLOTS=4;
 const char* labels[]={"a\\\"b","c\\\\d","e\\nf","g\\th"};
 const auto researchCaptureLabelSlotName=[](uint8_t i){return i==0?"A":i==1?"B":i==2?"C":"D";};
'''+web[a:b]+r'''
 jw.u32("samples",42);jw.finish();
 assert(j=="{\"before\":true,\"labels\":{\"A\":\"a\\\"b\",\"B\":\"c\\\\d\",\"C\":\"e\\nf\",\"D\":\"g\\th\"},\"samples\":42}");
}
'''
with tempfile.TemporaryDirectory() as d:
 source=Path(d)/'test.cpp';binary=Path(d)/'test';source.write_text(code)
 subprocess.run([os.environ.get('CXX','g++'),'-std=c++17','-O2','-I',str(ROOT),str(source),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
assert 'chunk.reserve(RESEARCH_CAPTURE_CSV_CHUNK_BYTES);' in web
assert 'j.reserve(1440)' not in web
print('Web adapters, nested JSON labels, CSV boundary behavior PASS')
