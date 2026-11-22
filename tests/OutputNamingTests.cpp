#ifdef NDEBUG
#undef NDEBUG
#endif
#include "core/OutputNaming.h"
#include <cassert>
#include <unordered_set>
using namespace lamexp::core;
int main() {
    Tags t{{"artist","Björk"},{"album","Debut"},{"track","3/12"},{"title","Human Behaviour"}};
    auto a=renderName("{artist} - {album} - {track:02} - {title}",t,"fallback");
    assert(a.error.empty() && a.stem=="Björk - Debut - 03 - Human Behaviour");
    assert(renderName("{unknown}",t,"x").error.size()>0);
    assert(renderName("{artist",t,"x").error.size()>0);
    assert(renderName("{title}",{},"Input").stem=="Input");
    assert(sanitizeWindowsComponent("CON.mp3")=="_CON.mp3");
    assert(sanitizeWindowsComponent("bad:/\\*?name. ")=="bad_____name");
    std::unordered_set<std::string> reserved{"track"};
    assert(uniqueStem("Track",reserved)=="Track (2)");
    assert(uniqueStem("Track",reserved)=="Track (3)");
    assert(renderName("{track:02}",{{"track","12/15"}},"x").stem=="12");
    auto folders=renderFolders("{artist}/{year} - {album}", t, "fallback");
    assert(folders.error.empty() && folders.components.size()==2);
    assert(folders.components[0]=="Björk" && folders.components[1]=="Unknown Year - Debut");
    assert(!renderFolders("../{artist}",t,"fallback").error.empty());
    assert(!renderFolders("/{artist}",t,"fallback").error.empty());
    assert(!renderFolders("{artist}/",t,"fallback").error.empty());
    assert(!renderFolders("{artist}\\{album}",t,"fallback").error.empty());
    assert(renderFolders("{artist}/{album}",{{"artist","CON"},{"album","A:B"}},"fallback").components[0]=="_CON");
}
