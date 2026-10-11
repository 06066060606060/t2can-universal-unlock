#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include "../text_format_pure.h"
static std::string legacyEscape(const std::string &in, JsonTextPolicy p) {
 if(p==JsonTextPolicy::Ble){
  std::string out=in;
  const auto replaceAll=[&](const std::string&a,const std::string&b){size_t pos=0;while((pos=out.find(a,pos))!=std::string::npos){out.replace(pos,a.size(),b);pos+=b.size();}};
  replaceAll("\\","\\\\");replaceAll("\"","\\\"");replaceAll("\n","\\n");replaceAll("\r","");return out;
 }
 std::string out;
 for (unsigned char c:in) {
  if(c=='\\'||c=='"'){out+='\\';out+=(char)c;}
  else if(c=='\n')out+="\\n";
  else if(c=='\r'){if(p!=JsonTextPolicy::Ble)out+="\\r";}
  else if(c=='\t'&&p!=JsonTextPolicy::Ble)out+="\\t";
  else if(p!=JsonTextPolicy::Research||c>=32)out+=(char)c;
 }
 return out;
}
static void legacyHex(const uint8_t*d,size_t n,char*out,size_t cap){
 if(!out||!cap)return;out[0]=0;if(!d)return;
 for(size_t i=0;i<n;++i){int wrote=snprintf(out,cap,"%s%02X",i?" ":"",d[i]);if(wrote<=0||(size_t)wrote>=cap)break;out+=wrote;cap-=wrote;}
}
int main(){
 for(auto p:{JsonTextPolicy::Wifi,JsonTextPolicy::Research,JsonTextPolicy::Ble})
  for(unsigned a=0;a<256;++a)for(unsigned b=0;b<256;++b){std::string in{(char)a,(char)b},out;appendJsonEscapedPure(out,in.data(),in.size(),p);assert(out==legacyEscape(in,p));}
 uint8_t bytes[256];for(unsigned i=0;i<256;++i)bytes[i]=i;
 for(size_t n=0;n<=256;++n)for(size_t cap=0;cap<=770;++cap){char a[772],b[772];memset(a,0x55,sizeof(a));memset(b,0x55,sizeof(b));legacyHex(bytes,n,a,cap);formatHexBytesPure(bytes,n,b,cap);assert(memcmp(a,b,sizeof(a))==0);}
 for(auto p:{HttpBoolPolicy::Strict01,HttpBoolPolicy::StrictAliases,HttpBoolPolicy::LegacyOne,HttpBoolPolicy::LegacyOneTrue,HttpBoolPolicy::LegacyAliases})
  for(const auto&s:std::vector<std::string>{"","0","1","true","false","on","off","TRUE"," 1","1 ","yes","2",std::string("1\0x",3)})
   for(bool initial:{false,true}){bool out=initial;bool ok=parseHttpBoolPure(s.data(),s.size(),out,p);bool aliases=p==HttpBoolPolicy::StrictAliases||p==HttpBoolPolicy::LegacyAliases;bool oneTrue=aliases||p==HttpBoolPolicy::LegacyOneTrue;bool yes=s=="1"||(oneTrue&&s=="true")||(aliases&&s=="on");bool no=s=="0"||(aliases&&(s=="false"||s=="off"));bool strict=p==HttpBoolPolicy::Strict01||p==HttpBoolPolicy::StrictAliases;assert(ok==(!strict||yes||no));assert(out==(ok?yes:initial));}
 formatHexBytesPure(nullptr,8,nullptr,0);
}
