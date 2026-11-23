# Encoder profiles (v0.10 development)

This feature branch adds controls for MP3 VBR, ABR and CBR; selectable output sample rates; and persistent portable encoding profiles.

## MP3 encoder options

- **VBR**: `libmp3lame -q:a 0..9` (default: 2). The bitrate varies by input material.
- **ABR**: `libmp3lame -abr 1 -b:a <kbps>k`. Bitrate is a target **average**, not an exact per-frame or per-second rate.
- **CBR**: `libmp3lame -b:a <kbps>k` without the ABR switch or VBR quality flag. Constant MP3 frame bitrate.
- Separate direct `lame.exe` mode maps to `-V`, `--abr` or `--cbr -b`, but requires independently supplied and tested LAME binary.

Supported MP3 rate choices in GUI: 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256 and 320 kbit/s.

## Sample rate and channels

- MP3 and FLAC: preserve original/default sample rate, or request 32,000 / 44,100 / 48,000 Hz.
- Opus: the encoder selects its required internal rate (normally 48,000 Hz); its sample rate control is disabled rather than claiming user-selectable rates.
- Choose original channel layout, mono, or stereo with FFmpeg `-ac`.
- Downsampling or mixing to mono does not improve audio quality; settings change the output signal.

## Portable saved profiles

Profiles can be loaded, created and deleted in the GUI. Built-in profiles include MP3 VBR V2, MP3 CBR 320, MP3 ABR 192, Opus 128 and lossless FLAC. User-defined profiles are saved to `LameXPNext.ini` beside `LameXPNext.exe`. The folder must be writable for saving.

## Validation

- C++ pipeline tests assert argument choices for VBR, CBR, ABR, channels and sample rate.
- `tests/SmokeEncodingParameters.py` runs real FFmpeg encodes and probes the output codec, reported MP3 CBR rate, sample rate and channel count using FFprobe.
- This verifies encoder outputs independently of GUI widgets; full Windows GUI integration remains a separate human test.

### Run independently

```sh
python3 tests/SmokeEncodingParameters.py
```

Or point at the packaged Windows binaries:

```powershell
python tests/SmokeEncodingParameters.py --ffmpeg dist/codecs/ffmpeg.exe --ffprobe dist/codecs/ffprobe.exe
```

**Development status:** Do not merge or publish before green CI and review. Portable codec redistribution still requires the license/source audit documented in `docs/PORTABLE_0.9.md`.
