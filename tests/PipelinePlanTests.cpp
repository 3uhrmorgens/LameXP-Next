#ifdef NDEBUG
#undef NDEBUG
#endif
#include "core/PipelinePlan.h"
#include <algorithm>
#include <cassert>
#include <string>
using namespace lamexp::core;
static bool has(const std::vector<std::string>& args, const std::string& s) {
    return std::find(args.begin(), args.end(), s) != args.end();
}
int main() {
    for (auto format : {OutputFormat::MP3, OutputFormat::FLAC, OutputFormat::Opus}) {
        PipelineOptions opt; opt.format = format;
        const auto plan = planPipeline(opt);
        assert(plan.steps.size() == 1 && plan.metadataPreserved);
        assert(has(plan.steps[0].args, kSource));
        assert(has(plan.steps[0].args, kOutput));
        assert(has(plan.steps[0].args, "-map_metadata"));
        assert(!has(plan.steps[0].args, kPcm));
    }
    PipelineOptions opt; opt.useNativeLame = true;
    auto plan = planPipeline(opt);
    assert(plan.steps.size() == 3 && plan.metadataPreserved);
    assert(plan.steps[0].executable == Backend::FFmpeg);
    assert(plan.steps[1].executable == Backend::NativeLame);
    assert(has(plan.steps[0].args, "pcm_s16le"));
    assert(has(plan.steps[0].args, "pipe:1"));
    assert(has(plan.steps[1].args, "-"));
    assert(has(plan.steps[1].args, kEncoded));
    assert(!has(plan.steps[0].args, kPcm));
    assert(plan.steps[2].executable == Backend::FFmpeg);
    assert(has(plan.steps[2].args, kEncoded));
    assert(has(plan.steps[2].args, "copy"));
    assert(has(plan.steps[2].args, kOutput));
    opt.hasAttachedCover = true;
    const auto coverPlan = planPipeline(opt);
    assert(has(coverPlan.steps[2].args, "attached_pic"));
    // User-configurable encoder parameters must reach real FFmpeg arguments.
    PipelineOptions custom;
    custom.format = OutputFormat::MP3;
    custom.mp3VbrQuality = 0;
    custom.outputChannels = 1;
    const auto mp3 = planPipeline(custom);
    assert(has(mp3.steps[0].args, "0"));
    assert(has(mp3.steps[0].args, "-ac"));
    assert(has(mp3.steps[0].args, "1"));
    custom.format = OutputFormat::Opus;
    custom.opusBitrateKbps = 192;
    custom.outputChannels = 2;
    const auto opus = planPipeline(custom);
    assert(has(opus.steps[0].args, "192k"));
    assert(has(opus.steps[0].args, "2"));
    custom.format = OutputFormat::FLAC;
    const auto flac = planPipeline(custom);
    assert(has(flac.steps[0].args, "flac"));
    // Extended MP3 rate control and sample rate.
    custom.format = OutputFormat::MP3;
    custom.mp3RateMode = RateMode::CBR;
    custom.mp3BitrateKbps = 320;
    custom.sampleRateHz = 44100;
    auto cbr = planPipeline(custom);
    assert(has(cbr.steps[0].args, "320k"));
    assert(has(cbr.steps[0].args, "-ar"));
    assert(has(cbr.steps[0].args, "44100"));
    assert(!has(cbr.steps[0].args, "-q:a"));
    custom.mp3RateMode = RateMode::ABR;
    custom.mp3BitrateKbps = 192;
    auto abr = planPipeline(custom);
    assert(has(abr.steps[0].args, "-abr"));
    assert(has(abr.steps[0].args, "192k"));
    custom.useNativeLame = true;
    auto nativeAbr = planPipeline(custom);
    assert(has(nativeAbr.steps[1].args, "--abr"));
    assert(has(nativeAbr.steps[1].args, "192"));
    assert(has(nativeAbr.steps[0].args, "44100"));
    custom.useNativeLame = false;
    opt.format = OutputFormat::Opus;
    plan = planPipeline(opt);
    assert(plan.steps.size() == 1);
}
