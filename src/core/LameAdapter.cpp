#include "LameAdapter.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <stdexcept>

namespace lamexp::core {
namespace {
constexpr std::array<int, 14> bitrates{32,40,48,56,64,80,96,112,128,160,192,224,256,320};
constexpr std::array<int, 10> vbrQualities{9,8,7,6,5,4,3,2,1,0};
constexpr std::array<int, 4> algorithmQualities{7,5,2,0};
int checkedIndex(int index, int count) {
    if (index < 0 || index >= count) throw std::invalid_argument("Invalid LAME quality/bitrate index");
    return index;
}
void append(std::vector<std::string>& args, const char* key, int value) {
    args.emplace_back(key); args.push_back(std::to_string(value));
}
}

bool LameAdapter::supportsInput(const std::string& filename) {
    const auto dot = filename.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string ext = filename.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".wav" || ext == ".wave";
}

int LameAdapter::nearestBitrate(int kbps) {
    const auto clamped = std::clamp(kbps, 32, 320);
    return *std::min_element(bitrates.begin(), bitrates.end(), [clamped](int a, int b) {
        return std::abs(a - clamped) < std::abs(b - clamped);
    });
}

std::vector<std::string> LameAdapter::arguments(const LameSettings& settings,
                                                 const std::string& wavInput,
                                                 const std::string& mp3Output) {
    if (wavInput != "-" && !supportsInput(wavInput)) throw std::invalid_argument("Direct LAME adapter requires WAV or stdin input");
    if (wavInput.empty() || mp3Output.empty()) throw std::invalid_argument("Missing input/output path");
    std::vector<std::string> args{"--nohist"};
    append(args, "-q", algorithmQualities.at(checkedIndex(settings.algorithmIndex, 4)));
    switch (settings.rateMode) {
        case RateMode::VBR:
            append(args, "-V", vbrQualities.at(checkedIndex(settings.qualityIndex, 10)));
            break;
        case RateMode::ABR:
            append(args, "--abr", bitrates.at(checkedIndex(settings.bitrateIndex, 14)));
            break;
        case RateMode::CBR:
            args.emplace_back("--cbr");
            append(args, "-b", bitrates.at(checkedIndex(settings.bitrateIndex, 14)));
            break;
    }
    if (settings.minBitrateKbps > 0 && settings.maxBitrateKbps > 0 &&
        settings.minBitrateKbps <= settings.maxBitrateKbps && settings.rateMode != RateMode::CBR) {
        append(args, "-b", nearestBitrate(settings.minBitrateKbps));
        append(args, "-B", nearestBitrate(settings.maxBitrateKbps));
    }
    if (settings.sampleRateKhz > 0) {
        // LAME accepts supported kHz values only; reject arbitrary input instead of guessing.
        constexpr std::array<int, 9> accepted{8, 11, 12, 16, 22, 24, 32, 44, 48};
        if (std::find(accepted.begin(), accepted.end(), settings.sampleRateKhz) == accepted.end())
            throw std::invalid_argument("Unsupported requested sample rate");
        append(args, "--resample", settings.sampleRateKhz);
    }
    switch (settings.channelMode) {
        case ChannelMode::Automatic: break;
        case ChannelMode::JointStereo: args.insert(args.end(), {"-m", "j"}); break;
        case ChannelMode::ForceJointStereo: args.insert(args.end(), {"-m", "f"}); break;
        case ChannelMode::Stereo: args.insert(args.end(), {"-m", "s"}); break;
        case ChannelMode::DualChannel: args.insert(args.end(), {"-m", "d"}); break;
        case ChannelMode::Mono: args.insert(args.end(), {"-m", "m"}); break;
    }
    args.push_back(wavInput);
    args.push_back(mp3Output);
    return args;
}
}
