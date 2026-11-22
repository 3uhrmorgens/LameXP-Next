#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace lamexp::core {
using Tags = std::unordered_map<std::string, std::string>;
// Supports: {artist}, {albumartist}, {album}, {title}, {track}, {track:02},
// {disc}, {year}, {filename}. Missing tags use explicit non-empty fallbacks.
struct NamingResult { std::string stem; std::string error; };
NamingResult renderName(const std::string& pattern, const Tags& tags, const std::string& filenameStem);
std::string sanitizeWindowsComponent(const std::string& text);
// Renders a relative directory hierarchy separated by forward slashes.
// Each segment is sanitized independently and traversal is rejected.
struct FolderResult { std::vector<std::string> components; std::string error; };
FolderResult renderFolders(const std::string& pattern, const Tags& tags, const std::string& filenameStem);
std::string uniqueStem(const std::string& stem, std::unordered_set<std::string>& reservedFolded);
}
