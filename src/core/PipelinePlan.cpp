#include "PipelinePlan.h"
#include <algorithm>
namespace lamexp::core {
PipelinePlan planPipeline(const PipelineOptions& opts) {
    PipelinePlan plan;
    const auto mp3Quality = std::clamp(opts.mp3VbrQuality, 0, 9);
    const auto opusRate = std::clamp(opts.opusBitrateKbps, 32, 320);
    const int channels = opts.outputChannels == 1 || opts.outputChannels == 2 ? opts.outputChannels : 0;
    const int sampleRate = opts.sampleRateHz;
    const int mp3Rate = LameAdapter::nearestBitrate(opts.mp3BitrateKbps);
    if (opts.format == OutputFormat::MP3 && opts.useNativeLame) {
        // A bounded OS pipe sends WAV-encoded PCM directly from FFmpeg to LAME.
        // Metadata is NOT transparently preserved by a WAV intermediate.
        plan.steps.push_back({Backend::FFmpeg, {
            "-hide_banner", "-nostdin", "-loglevel", "error", "-n", "-i", kSource,
            "-map", "0:a:0", "-vn", "-sn", "-dn", "-map_metadata", "-1",
            "-c:a", "pcm_s16le", "-f", "wav", "pipe:1"
        }});
        if (channels || sampleRate > 0) {
            auto& args = plan.steps.back().args;
            if (channels) args.insert(args.end() - 1, {"-ac", std::to_string(channels)});
            if (sampleRate > 0) args.insert(args.end() - 1, {"-ar", std::to_string(sampleRate)});
        }
        auto settings = opts.lame;
        settings.qualityIndex = 9 - mp3Quality; // legacy index maps 9 to V0, 0 to V9
        settings.rateMode = opts.mp3RateMode;
        if (opts.mp3RateMode != RateMode::VBR) {
            const auto kbps = mp3Rate;
            const int bitrates[] = {32,40,48,56,64,80,96,112,128,160,192,224,256,320};
            for (int i = 0; i < 14; ++i) if (bitrates[i] == kbps) settings.bitrateIndex = i;
        }
        if (channels == 1) settings.channelMode = ChannelMode::Mono;
        else if (channels == 2) settings.channelMode = ChannelMode::Stereo;
        auto lameArgs = LameAdapter::arguments(settings, "-", "output.mp3");
        lameArgs.back() = kEncoded;
        plan.steps.push_back({Backend::NativeLame, std::move(lameArgs)});
        // Lossless MP3 remux: transfer source tags and optional embedded cover.
        std::vector<std::string> mux{
            "-hide_banner", "-nostdin", "-loglevel", "error", "-n",
            "-i", kEncoded, "-i", kSource,
            "-map", "0:a:0", "-map_metadata", "1", "-c:a", "copy"
        };
        if (opts.hasAttachedCover) mux.insert(mux.end(), {
            "-map", "1:v:0", "-c:v", "mjpeg", "-disposition:v:0", "attached_pic"
        });
        mux.insert(mux.end(), {"-id3v2_version", "3", kOutput});
        plan.steps.push_back({Backend::FFmpeg, std::move(mux)});
        plan.metadataPreserved = true; // requested: actual fidelity is format-dependent
        return plan;
    }
    std::vector<std::string> args{
        "-hide_banner", "-nostdin", "-loglevel", "error", "-n", "-i", kSource,
        "-map", "0:a:0", "-sn", "-dn", "-map_metadata", "0"
    };
    if (channels) args.insert(args.end(), {"-ac", std::to_string(channels)});
    if (sampleRate > 0) args.insert(args.end(), {"-ar", std::to_string(sampleRate)});
    if (opts.format == OutputFormat::MP3) {
        args.insert(args.end(), {"-c:a", "libmp3lame"});
        if (opts.mp3RateMode == RateMode::VBR)
            args.insert(args.end(), {"-q:a", std::to_string(mp3Quality)});
        else if (opts.mp3RateMode == RateMode::CBR)
            args.insert(args.end(), {"-b:a", std::to_string(mp3Rate) + "k"});
        else
            args.insert(args.end(), {"-abr", "1", "-b:a", std::to_string(mp3Rate) + "k"});
        args.insert(args.end(), {"-id3v2_version", "3"});
    } else if (opts.format == OutputFormat::Opus) {
        args.insert(args.end(), {"-c:a", "libopus", "-b:a", std::to_string(opusRate) + "k"});
    } else {
        args.insert(args.end(), {"-c:a", "flac"});
    }
    if (opts.format == OutputFormat::MP3 && opts.hasAttachedCover) {
        args.insert(args.end(), {"-map", "0:v:0", "-c:v", "mjpeg", "-disposition:v:0", "attached_pic"});
    }
    args.push_back(kOutput);
    plan.steps.push_back({Backend::FFmpeg, std::move(args)});
    plan.metadataPreserved = true; // FFmpeg metadata map requested; individual tags may vary by container.
    return plan;
}
}
