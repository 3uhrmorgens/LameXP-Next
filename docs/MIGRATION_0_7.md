# v0.7 migration report — Windows x64 build gate & asynchronous metadata

## Implemented

* `MetadataScanner` is a Qt event-loop-driven, bounded **sequential** FFprobe analyzer. It is asynchronous (the UI is not blocked by `waitForFinished()`), but not a multithreaded worker pool.
* One FFprobe process collects tags and stream dispositions per source. A 10-second timer prevents indefinitely stuck individual probes, and a 2 MiB output cap limits subprocess JSON accumulation.
* Cache keys include absolute source filename, input size, last-modification time and ffprobe executable path. This is metadata caching, not content-hash verification; files modified with unchanged timestamp and size can give stale results.
* File naming, subfolders and MP3 cover detection now read cached scan results; the scan completes before planning or enqueueing. Warnings are shown for failed probes. On failures, naming falls back to existing logic; cover detection can be incomplete.
* Windows CI (GitHub Actions) compiles with MSVC 2022 / Qt 6.8.3, runs CTest, validates the PE AMD64 signature, performs an offscreen GUI launch smoke check, and publishes a Qt runtime archive. The job has **not been run in this environment**.
* Fix a cancellation race in the paired FFmpeg → LAME process: mark encoder complete before waiting for decoder termination.

## Important limitations

1. The GUI and Windows CI workflow could not be executed locally: the local environment is Linux and **Qt 6 development libraries are absent**. A passing core-only CMake build is not proof of Windows compatibility.
2. Codec executable binaries are deliberately **not included**; users must place their verified `ffmpeg.exe`, `ffprobe.exe`, and optionally `lame.exe` together and select FFmpeg in the UI.
3. The current application is still a separately written prototype. The entire SourceForge Release_421 upstream and MUtilities code have not been imported or ported.
4. The metadata scanner scans *one input file at a time* to limit external processes. Thread-pooled analysis with coalescing / cancellation can be added after an instrumented Windows run.
5. The output-file reservation behavior is not a full cross-process filesystem transaction; external simultaneous writers remain a theoretical race.
6. The encoder smoke scripts validate external codec commands on Linux. Windows Qt `QProcess` stream and cancellation semantics require integration tests.

## Windows CI execution

Push these sources to GitHub, enable Actions, and manually dispatch `.github/workflows/build.yml` or push a commit. Review the `windows-x64` job logs, first collect compile diagnostics, then run the offscreen startup check and download the produced artifact. If the initial CI job fails, the error log is the next precise task; do not label it a verified Windows build until the job actually succeeds.
