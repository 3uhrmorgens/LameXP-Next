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
    opt.format = OutputFormat::Opus;
    plan = planPipeline(opt);
    assert(plan.steps.size() == 1);
}
