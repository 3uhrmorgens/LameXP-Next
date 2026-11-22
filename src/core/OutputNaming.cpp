#include "OutputNaming.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace lamexp::core {
namespace {
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){return char(std::tolower(c));});
    return s;
}
std::string trim(const std::string& s) {
    auto a=s.find_first_not_of(" \t\r\n");
    if(a==std::string::npos) return {};
    auto b=s.find_last_not_of(" \t\r\n"); return s.substr(a,b-a+1);
}
std::string fallback(const Tags& tags, const std::string& key, const std::string& filename) {
    const auto it=tags.find(key);
    if(it!=tags.end() && !trim(it->second).empty()) return trim(it->second);
    if(key=="title" || key=="filename") return filename;
    if(key=="artist" || key=="albumartist") return "Unknown Artist";
    if(key=="album") return "Unknown Album";
    if(key=="track" || key=="disc") return "0";
    return "Unknown Year";
}
}
std::string sanitizeWindowsComponent(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for(unsigned char c: text) {
        if(c<32 || c==127 || c=='<' || c=='>' || c==':' || c=='"' || c=='/' || c=='\\' || c=='|' || c=='?' || c=='*') out += '_';
        else out += char(c);
    }
    out=trim(out);
    while(!out.empty() && (out.back()=='.' || out.back()==' ')) out.pop_back();
    if(out.empty() || out=="." || out=="..") return "Untitled";
    auto base=lower(out.substr(0,out.find('.')));
    if(base=="con" || base=="prn" || base=="aux" || base=="nul" ||
       (base.size()==4 && ((base.substr(0,3)=="com" || base.substr(0,3)=="lpt") && base[3]>='1' && base[3]<='9'))) out="_"+out;
    // Limit UTF-8 safely, without splitting a code point.
    if(out.size()>180) {
        size_t n=180;
        while(n>0 && (static_cast<unsigned char>(out[n])&0xC0)==0x80) --n;
        out.resize(n);
        while(!out.empty() && (out.back()=='.' || out.back()==' ')) out.pop_back();
    }
    return out.empty()?"Untitled":out;
}
NamingResult renderName(const std::string& pattern, const Tags& tags, const std::string& filenameStem) {
    if(pattern.empty()) return {{},"Empty pattern"};
    std::string raw;
    for(size_t i=0;i<pattern.size();) {
        if(pattern[i]=='{') {
            auto end=pattern.find('}',i+1);
            if(end==std::string::npos) return {{},"Unclosed token"};
            auto key=lower(pattern.substr(i+1,end-i-1));
            const bool pad = key=="track:02" || key=="disc:02";
            if(pad) key=key.substr(0,key.find(':'));
            if(key!="artist" && key!="albumartist" && key!="album" && key!="title" &&
               key!="track" && key!="disc" && key!="year" && key!="filename") return {{},"Unknown token: "+key};
            auto value=fallback(tags,key,filenameStem);
            if(key=="track" || key=="disc") {
                auto slash=value.find('/'); if(slash!=std::string::npos) value=value.substr(0,slash);
                if(pad) {
                    try { size_t used=0; int n=std::stoi(value,&used); if(used==value.size() && n>=0 && n<=999) {
                        std::ostringstream os; os<<std::setfill('0')<<std::setw(2)<<n; value=os.str();
                    }} catch(...) { /* retain raw tag */ }
                }
            }
            raw += value; i=end+1;
        } else if(pattern[i]=='}') return {{},"Unexpected closing brace"};
        else raw+=pattern[i++];
    }
    return {sanitizeWindowsComponent(raw),{}};
}
FolderResult renderFolders(const std::string& pattern, const Tags& tags, const std::string& filenameStem) {
    if (pattern.empty()) return {{}, "Empty folder pattern"};
    if (pattern.size() > 2048) return {{}, "Folder pattern too long"};
    FolderResult result;
    size_t begin = 0;
    while (begin <= pattern.size()) {
        auto end = pattern.find('/', begin);
        auto component = pattern.substr(begin, end == std::string::npos ? end : end - begin);
        if (component.empty() || component == "." || component == "..")
            return {{}, "Empty or traversal directory component"};
        if (component.find('\\') != std::string::npos) return {{}, "Use / as directory separator"};
        auto rendered = renderName(component, tags, filenameStem);
        if (!rendered.error.empty()) return {{}, rendered.error};
        if (rendered.stem == "Untitled" && trim(component).empty()) return {{}, "Empty directory component"};
        result.components.push_back(std::move(rendered.stem));
        if (result.components.size() > 8) return {{}, "Too many subdirectories"};
        if (end == std::string::npos) break;
        begin = end + 1;
    }
    return result;
}
std::string uniqueStem(const std::string& stem, std::unordered_set<std::string>& reservedFolded) {
    // Callers also reserve the generated name against filesystem entries.
    auto candidate=stem;
    for(unsigned int n=2; !reservedFolded.insert(lower(candidate)).second; ++n) {
        candidate=stem+" ("+std::to_string(n)+")";
    }
    return candidate;
}
}
