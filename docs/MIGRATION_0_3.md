# Migration 0.3 – Pipeline and atomic-ish output staging

- `PipelinePlan` (Qt-independent C++20) generates FFmpeg / LAME stages.
- Native LAME mode now accepts FFmpeg-decodable source formats: FFmpeg decodes the first audio stream to 16-bit PCM WAV in a temporary directory, LAME encodes to a hidden staged MP3 in the destination directory, and the result is renamed to its final name only on success.
- FFmpeg-only pipeline stages outputs similarly, carrying metadata mapping by FFmpeg (`-map_metadata 0`). Format-dependent tags are not guaranteed to survive.
- Intermediate WAV and failed/cancelled staged output are deleted. Active processes are killed on cancellation and marked cancelled to avoid committing results.
- No automatic overwrite of existing destination; collision check at enqueue and commit. Not a transactional file system protocol.
- Paths are carried as Qt QString tokens into QProcess argument arrays; no shell command line constructed.
- **Known issues:** direct LAME loses metadata; large intermediate WAV can exhaust disk; only first audio stream; no WAV size threshold handling; no codec binary verification; no progress bar; FFmpeg and LAME command-line availability differs across external builds.
- Core unit tests can be compiled on Linux, but Windows Qt6 GUI build and real end-to-end WAV/MP3 conversion have not been verified.
- This package does NOT vendor the original LameXP code or MUtilities. Integration requires separate source and license verification.
