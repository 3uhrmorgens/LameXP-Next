#include "PipelinePlan.h"
namespace lamexp::core {
PipelinePlan planPipeline(const PipelineOptions& opts) {
    PipelinePlan plan;
    if (opts.format == OutputFormat::MP3 && opts.useNativeLame) {
        // A bounded OS pipe sends WAV-encoded PCM directly from FFmpeg to LAME.
        // Metadata is NOT transparently preserved by a WAV intermediate.
        plan.steps.push_back({Backend::FFmpeg, {
            "-hide_banner", "-nostdin", "-loglevel", "error", "-n", "-i", kSource,
            "-map", "0:a:0", "-vn", "-sn", "-dn", "-map_metadata", "-1",
            "-c:a", "pcm_s16le", "-f", "wav", "pipe:1"
        }});
        auto lameArgs = LameAdapter::arguments(opts.lame, "-", "output.mp3");
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
    if (opts.format == OutputFormat::MP3) {
        args.insert(args.end(), {"-c:a", "libmp3lame", "-q:a", "2", "-id3v2_version", "3"});
    } else if (opts.format == OutputFormat::Opus) {
        args.insert(args.end(), {"-c:a", "libopus", "-b:a", "128k"});
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
