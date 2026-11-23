#!/usr/bin/env python3
"""Run actual FFmpeg encodes and verify resulting audio streams with FFprobe.

Uses exactly the encoder flags chosen by LameXP Next's current pipeline plan.
This is an external codec smoke test, not a full GUI automation test.
"""
import argparse
import json
import shutil
import subprocess
import tempfile
from pathlib import Path


def command(args):
    result = subprocess.run(args, text=True, capture_output=True, check=False, timeout=90)
    if result.returncode != 0:
        raise AssertionError(
            f"Encoder command failed (exit {result.returncode}): {args}\n{result.stderr[-3000:]}"
        )
    return result.stdout


def probe(ffprobe, path):
    data = command([
        ffprobe, "-v", "error", "-select_streams", "a:0",
        "-show_entries", "stream=codec_name,sample_rate,channels,bit_rate",
        "-of", "json", str(path)
    ])
    streams = json.loads(data)["streams"]
    assert len(streams) == 1, f"No audio stream in {path}"
    return streams[0]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ffmpeg", default=shutil.which("ffmpeg"))
    parser.add_argument("--ffprobe", default=shutil.which("ffprobe"))
    args = parser.parse_args()
    if not args.ffmpeg or not args.ffprobe:
        raise SystemExit("FFmpeg and FFprobe are required")
    with tempfile.TemporaryDirectory(prefix="lamexp-parameters-") as temp:
        root = Path(temp)
        source = root / "source.wav"
        command([args.ffmpeg, "-hide_banner", "-loglevel", "error", "-y",
                 "-f", "lavfi", "-i", "sine=frequency=997:sample_rate=48000:duration=6",
                 "-ac", "2", "-c:a", "pcm_s16le", str(source)])
        # Tests the effective encoder settings, including actual output
        # sample rate and channel layout. ABR bitrates are not exact constants.
        cases = [
            ("mp3-cbr", "mp3", ["-c:a", "libmp3lame", "-b:a", "192k", "-ar", "44100", "-ac", "2"], "mp3", 44100, 2),
            ("mp3-abr", "mp3", ["-c:a", "libmp3lame", "-abr", "1", "-b:a", "160k", "-ar", "32000", "-ac", "1"], "mp3", 32000, 1),
            ("mp3-vbr", "mp3", ["-c:a", "libmp3lame", "-q:a", "2", "-ar", "48000", "-ac", "2"], "mp3", 48000, 2),
            ("opus", "opus", ["-c:a", "libopus", "-b:a", "128k", "-ac", "2"], "opus", 48000, 2),
            ("flac", "flac", ["-c:a", "flac", "-ar", "32000", "-ac", "1"], "flac", 32000, 1),
        ]
        for name, ext, settings, codec, hz, channels in cases:
            output = root / f"{name}.{ext}"
            command([args.ffmpeg, "-hide_banner", "-nostdin", "-loglevel", "error",
                     "-y", "-i", str(source), "-map", "0:a:0", *settings, str(output)])
            data = probe(args.ffprobe, output)
            assert data["codec_name"] == codec, (name, data)
            assert int(data["sample_rate"]) == hz, (name, data)
            assert int(data["channels"]) == channels, (name, data)
            if name == "mp3-cbr":
                # Nominal MP3 CBR bitrate is reported per audio stream; allow muxer variance.
                bitrate = int(data["bit_rate"])
                assert 185000 <= bitrate <= 200000, (name, bitrate)
            print(f"PASS {name}: {codec}, {hz} Hz, {channels} channels; "
                  f"reported bitrate={data.get('bit_rate', 'n/a')}")


if __name__ == "__main__":
    main()
