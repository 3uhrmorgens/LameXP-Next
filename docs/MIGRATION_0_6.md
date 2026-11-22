# LameXP Next 0.6 – streaming + automated builds

## Actual source changes

1. Native LAME jobs now use `ffmpeg -f wav pipe:1` and `lame ... - encoded.mp3`; two Qt `QProcess` instances are connected through `setStandardOutputProcess` (OS pipe, no user-level PCM buffering). The producer and consumer have separate stderr buffers, exit-state checks and joint cancellation. Failed codec stages never publish an output.
2. The already-existing post-encode FFmpeg stream-copy remux remains, retaining tags and optional cover art where supported, without MP3 audio re-encoding.
3. Default job concurrency is calculated by a platform-neutral `ResourcePolicy`; it conservatively starts with half of the detected logical CPUs, capped at eight. The user can override the limit to 1–32 in the UI. This is *not* adaptive load balancing or CPU usage telemetry.
4. Portable CMake core-only mode permits compilation/test without Qt; Windows x64 CI workflow installs Qt 6.8.3 and builds the GUI with MSVC on a GitHub-hosted runner, then runs CTest and packages the Qt runtime. Codec binaries are not downloaded or redistributed. The workflow file is provided, but has not run on GitHub in this environment.
5. Four C++ test executables (assertions explicitly enabled even in Release) and an external stream/metadata smoke test are included.

## Remaining constraints

- **A Windows Qt build was not executed locally** (MSVC and Qt 6 are absent). Workflow success remains unverified.
- The GUI's `ffprobe` metadata scanning is still synchronous, and must be migrated to a cancellable asynchronous cache before large-library usability claims.
- There is no measured CPU or throughput improvement yet; WAV temp storage is removed, but wall-clock gains are workload-specific.
- The full upstream LameXP/MUtilities sources are still not imported; this remains an independently written migration prototype.
- Finish/error/cancel race conditions in the Qt streaming handler require Windows stress testing, including invalid codec executables, full disks and user cancellation.
- Output rename uses Qt `QFile::rename`, not Win32 atomic move-with-no-replace against external writers.
- Windows binaries need redistributable codec licenses and a signed installer before production release.

## Local portable core + integration tests

```bash
cmake --preset linux-core
cmake --build --preset linux-core
ctest --preset linux-core
python3 tests/SmokeStreaming.py
python3 tests/SmokePipeline.py
python3 tests/SmokeMetadata.py
```

## Windows x64 local build

Open **x64 Native Tools Command Prompt for VS 2022** with Qt 6.8.3 MSVC2022 x64 and Ninja on PATH. Example:

```powershell
$env:CMAKE_PREFIX_PATH = 'C:\Qt\6.8.3\msvc2022_64'
cmake --preset win-msvc2022-x64
cmake --build --preset win-msvc2022-x64
ctest --preset win-msvc2022-x64
```

Build output: `out/build/win-msvc2022-x64/` (not `build/win-...`).

GitHub Actions instructions: Copy the project to a Git repository hosted on GitHub, push the branch, and inspect `.github/workflows/build.yml` runs; only after the Windows job is green should its build artifact be considered a verified Windows executable.
