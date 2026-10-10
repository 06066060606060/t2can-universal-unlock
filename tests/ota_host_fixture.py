"""Minimal host JSON boundary used by production OTA handler fixtures."""
JSON_WRITER = r'''
struct JsonWriterArduino {String &s;bool first=true;explicit JsonWriterArduino(String &out):s(out){s="{";}void prefix(const char*k){if(!first)s+=",";first=false;s+='"';s+=k;s+="\":";}void boolean(const char*k,bool v){prefix(k);s+=v?"true":"false";}void u32(const char*k,uint32_t v){prefix(k);s+=std::to_string(v);}void string(const char*k,const char*v){prefix(k);s+='"';s+=v;s+='"';}void finish(){s+='}';}};
'''
