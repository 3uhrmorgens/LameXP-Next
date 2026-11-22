#pragma once
#include <string>
#include <vector>

namespace lamexp::core {
enum class RateMode { VBR, ABR, CBR };
enum class ChannelMode { Automatic, JointStereo, ForceJointStereo, Stereo, DualChannel, Mono };

struct LameSettings {
    RateMode rateMode = RateMode::VBR;
    int qualityIndex = 5;      // Legacy LameXP GUI index: 0 -> -V 9, 9 -> -V 0
    int bitrateIndex = 8;      // Legacy LameXP bitrate index: 8 -> 128 kbps
    int algorithmIndex = 1;    // 0,1,2,3 -> -q 7,5,2,0
    int minBitrateKbps = 0;    // 0: unrestricted
    int maxBitrateKbps = 0;
    int sampleRateKhz = 0;     // 0: original input sample rate
    ChannelMode channelMode = ChannelMode::Automatic;
};

// No shell command construction: tokens passed individually to QProcess.
// This adapter accepts WAV files or WAV-encoded PCM from stdin ("-").
struct LameAdapter final {
    static bool supportsInput(const std::string& filename);
    static std::vector<std::string> arguments(const LameSettings& settings,
                                               const std::string& wavInput,
                                               const std::string& mp3Output);
    static int nearestBitrate(int kbps);
};
}
