# Architecture decisions (ADR 0001)

Target: Windows 10/11 x64, C++20, Qt 6.8 LTS baseline, MSVC 2022, CMake/Ninja.

## Source-to-target mapping
| Original Release_421 | Target |
| --- | --- |
| Decoder_Abstract and Decoder_* | `audio/IDecoder`, codec adapter registry |
| Encoder_Abstract and Encoder_* | `audio/IEncoder`, capabilities, versioned executable adapters |
| Thread_Process / Thread_CPUObserver | `jobs/JobScheduler`, resource budgets, cancellation |
| Dialog_MainWindow + GUI forms | `ui/` views + app-layer view models |
| MUtilities | isolate and replace incrementally, preserving behavior and licensing |
| VS2019 .sln/.vcxproj | CMake with Qt AUTOGEN and CMakePresets |

## Target modules
- `domain`: immutable jobs, audio formats, quality settings, error types.
- `audio`: decoder/encoder capability registry, probing, tagging and validation.
- `jobs`: dependency-aware queue, bounded parallelism, cancellation, retries, atomic output.
- `platform`: Windows filesystem, file metadata, processor topology, power policy.
- `ui`: Qt views only; delegates actions to services and subscribes to state changes.
- `cli`: future headless front-end sharing application services.

## Migration sequence
1. Mirror original Release_421 at commit b04e5ea and fetch the exact MUtilities revision; create provenance manifest.
2. Build unmodified baseline on Windows, run golden-file tests for formats/tags/output consistency.
3. Introduce facade around existing encoding/decoding; preserve existing CLI parameter semantics.
4. Incrementally replace Qt4 dependencies, `QString`/Qt API incompatibilities, unsupported Windows APIs and MUtilities.
5. Swap entrypoint to Qt6, then port one codec at a time and run regression fixtures.
6. Add scheduler performance tuning, UI redesign, packaging and signing.

## Security / correctness invariants
- no user-provided shell strings; pass executable and arguments separately.
- validate every executable binary and version before launching, include provenance and checksums.
- never overwrite source or pre-existing destinations; generate unique temp output and atomic rename on success.
- handle Unicode, long paths, symlinks/reparse points, permissions, cancellation, files >4 GiB.
- support per-codec CPU thread caps to avoid nested oversubscription.
- preserve audio settings and tags; verify stream properties and detect silent/missing outputs.

## Benchmarks to add
- Batch of 100 small WAV inputs to MP3, FLAC, Opus.
- Single long WAV input (CPU-heavy encoder) with varying thread counts.
- Mixed job profiles on P/E-core Intel systems and AMD 8/16/32-core systems.
- Compare throughput, peak memory, per-file latency, failure rates, thermals/power where measurable.
