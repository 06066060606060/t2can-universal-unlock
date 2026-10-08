from pathlib import Path
import subprocess, tempfile, textwrap

ROOT = Path(__file__).resolve().parents[1]
code = r'''
#include <stdint.h>
#include <string>
#include <sstream>
#include <iostream>

class String {
public:
  std::string v;
  String() = default;
  String(const char *s): v(s ? s : "") {}
  String(const std::string &s): v(s) {}
  String(unsigned long x){ v = std::to_string(x); }
  String(long x){ v = std::to_string(x); }
  String(unsigned int x){ v = std::to_string(x); }
  String(int x){ v = std::to_string(x); }
  String& operator+=(const char *s){ v += (s ? s : ""); return *this; }
  String& operator+=(char c){ v += c; return *this; }
  String& operator+=(unsigned long x){ v += std::to_string(x); return *this; }
  String& operator+=(long x){ v += std::to_string(x); return *this; }
  String& operator+=(unsigned int x){ v += std::to_string(x); return *this; }
  String& operator+=(int x){ v += std::to_string(x); return *this; }
  String& operator+=(const String &s){ v += s.v; return *this; }
  const char* c_str() const { return v.c_str(); }
};

#include "json_writer_arduino.h"

int main(){
  String s;
  JsonWriterArduino j(s);
  j.boolean("ok", true);
  j.u32("count", 42);
  j.i32("signed", -7);
  j.string("state", "OPEN");
  j.raw("fixed", String("1.25"));
  j.fixed("scaled", -123, 2);
  j.beginObject("nested");
  j.u32("value", 9);
  j.boolean("flag", false);
  j.endObject();
  j.beginArray("list");
  JsonWriterArduino child(s);
  child.u32("id", 1);
  child.finish();
  j.endArray();
  j.u32("tail", 10);
  j.finish();
  const std::string want = R"({"ok":true,"count":42,"signed":-7,"state":"OPEN","fixed":1.25,"scaled":-1.23,"nested":{"value":9,"flag":false},"list":[{"id":1}],"tail":10})";
  if (s.v != want) {
    std::cerr << s.v << " != " << want << "\n";
    return 1;
  }
  return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
    src = Path(td)/'t.cpp'
    exe = Path(td)/'t'
    src.write_text(code)
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-I',str(ROOT),str(src),'-o',str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('v3.6d9a2 JsonWriter host contract: PASS')
