#ifdef NDEBUG
#undef NDEBUG
#endif
#include "core/LameAdapter.h"
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>
using namespace lamexp::core;
static bool containsSequence(const std::vector<std::string>& a, std::initializer_list<std::string> expected) {
    return std::search(a.begin(), a.end(), expected.begin(), expected.end()) != a.end();
}
int main() {
    assert(LameAdapter::supportsInput("C:/Musik/TEST.WAV"));
    assert(!LameAdapter::supportsInput("test.mp3"));
    assert(LameAdapter::nearestBitrate(129) == 128);
    assert(LameAdapter::nearestBitrate(319) == 320);
    LameSettings s;
    auto args = LameAdapter::arguments(s, "track.wav", "output.mp3");
    assert(containsSequence(args, {"-V", "4"}));
    assert(containsSequence(args, {"-q", "5"}));
    assert(args.at(args.size() - 2) == "track.wav");
    args = LameAdapter::arguments(s, "-", "stream.mp3");
    assert(args.at(args.size() - 2) == "-");
    s.rateMode = RateMode::ABR; s.bitrateIndex = 13;
    args = LameAdapter::arguments(s, "track.wav", "output.mp3");
    assert(containsSequence(args, {"--abr", "320"}));
    s.rateMode = RateMode::CBR; s.bitrateIndex = 8;
    args = LameAdapter::arguments(s, "track.wav", "output.mp3");
    assert(containsSequence(args, {"--cbr", "-b", "128"}));
    s.rateMode = RateMode::VBR; s.minBitrateKbps = 96; s.maxBitrateKbps = 256;
    args = LameAdapter::arguments(s, "track.wav", "output.mp3");
    assert(containsSequence(args, {"-b", "96", "-B", "256"}));
    s.channelMode = ChannelMode::Mono;
    args = LameAdapter::arguments(s, "track.wav", "output.mp3");
    assert(containsSequence(args, {"-m", "m"}));
    s.qualityIndex = 99;
    bool thrown = false;
    try { (void)LameAdapter::arguments(s, "track.wav", "output.mp3"); }
    catch (const std::invalid_argument&) { thrown = true; }
    assert(thrown);
    thrown = false;
    try { (void)LameAdapter::arguments(LameSettings{}, "file.flac", "output.mp3"); }
    catch (const std::invalid_argument&) { thrown = true; }
    assert(thrown);
}
