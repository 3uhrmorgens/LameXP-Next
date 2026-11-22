#pragma once
#include "LameAdapter.h"
#include <string>
#include <vector>

namespace lamexp::core {
enum class OutputFormat { MP3, FLAC, Opus };
enum class Backend { FFmpeg, NativeLame };
// Tokens deliberately use placeholders, substituted by the Qt application layer.
// No executable contains shell command strings.
inline constexpr const char* kSource = "@SOURCE@";
inline constexpr const char* kPcm = "@PCM@";
inline constexpr const char* kOutput = "@OUTPUT@";
inline constexpr const char* kEncoded = "@ENCODED@";
struct ProcessStep {
    Backend executable;
    std::vector<std::string> args;
};
struct PipelinePlan {
    std::vector<ProcessStep> steps;
    bool metadataPreserved = false;
};
struct PipelineOptions {
    OutputFormat format = OutputFormat::MP3;
    bool useNativeLame = false;
    bool hasAttachedCover = false;
    LameSettings lame;
};
PipelinePlan planPipeline(const PipelineOptions& options);
}
