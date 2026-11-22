# v0.9 portable Windows-x64 development build

## Scope
This is a **development/test artifact**, not a cleared public release. Windows Qt DLLs and the FFmpeg release Essentials binaries (`ffmpeg.exe`, `ffprobe.exe`) are staged in one ZIP. MP3 is supported through FFmpeg's `libmp3lame` encoder. **No standalone `lame.exe` is included** yet: the direct-LAME option requires a separately supplied binary and a compatibility test.

## Installation
Unzip the workflow artifact on Windows 10/11 x64, then run `LameXPNext.exe` from the extracted folder. The GUI automatically selects the adjacent `codecs/ffmpeg.exe` if present. Codec executable paths may still be chosen manually. `ffprobe.exe` is expected in the same codecs directory.

## Verification
The CI workflow performs:
- C++20 core tests on Linux and Windows
- External audio smoke tests on Linux
- Windows MSVC x64 compilation and Qt offscreen startup
- SHA256 verification of the fetched FFmpeg Essentials ZIP against the supplier's matching checksum
- `ffmpeg -version`, `ffprobe -version` and a real `libmp3lame` sample encode + probe

The dynamic supplier ZIP+checksum pair makes these builds **non-reproducible over time**. Before a public release, pin the full source version and archive SHA256 in an immutable lock file, confirm corresponding source distribution and relevant GPL/LGPL notices, and perform a full GUI-driven Windows end-to-end test.

## License and provenance
- FFmpeg licensing: https://ffmpeg.org/legal.html
- Supplier / build configuration: https://www.gyan.dev/ffmpeg/builds/
- LAME project: https://lame.sourceforge.io/
- Qt licensing: https://www.qt.io/licensing/

Redistributing binaries may require more than URLs. Audit the particular FFmpeg build's `--enable-gpl` / `--enable-nonfree` flags, bundled dependencies, matching source, copyright notices and license texts. The script captures archive SHA and any included LICENSE/COPYING/README files; this is NOT a substitute for a compliance review.
