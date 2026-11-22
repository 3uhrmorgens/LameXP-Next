# LameXP Release_421 → LameXP Next 0.2: migration notes

## Evidence examined
- https://sourceforge.net/p/lamexp/code/ci/Release_421/tree/src/Encoder_MP3.cpp
- https://sourceforge.net/p/lamexp/code/ci/Release_421/tree/src/Encoder_MP3.h
- https://sourceforge.net/p/lamexp/code/ci/Release_421/tree/src/
- https://lamexp.sourceforge.net/doc/Manual.html

The original has `MP3Encoder`, `AbstractEncoder`, a settings model, per-tool lookup, Qt QProcess and progress callbacks. This 0.2 prototype **does not vendor the upstream source**. Its argument mapping is independently written from documented behavior and reviewed against the public source. The full repository and the exact compatible MUtilities revision have not been imported.

## Preserved MP3 semantics in argument mapping
- Mode: VBR (`-V`), ABR (`--abr`), CBR (`--cbr -b`)
- VBR UI index mapping: 0..9 => LAME -V 9..0
- Bitrate UI indices: 32,40,48,56,64,80,96,112,128,160,192,224,256,320 kbit/s
- Algorithm quality: legacy indices 0..3 => `-q` 7,5,2,0
- Optional bitrate lower/upper bounds, sample-rate override and channel mode.

## NOT yet migrated
- LameXP legacy metadata, tag character sanitation, ID3 handling and cover art
- Input decoding to WAV/PCM, resampling/dithering/downmix pipeline
- Progress event parsing and timing, codec binary discovery/version verification
- MUtilities, rest of original encoder and decoder matrix, CUE/playlist support
- Integration testing with real `lame.exe`, verification of encoded audio, Qt x64 build

## Known correctness and security follow-ups
- Validate WAV PCM header/bit depth/channel count before passing to LAME.
- Replace direct output writes with same-volume temporary outputs and successful atomic finalization; clean partial files on cancellation.
- Reject reserved Windows filenames, symlinks/reparse point escapes and overlapping destinations under concurrent jobs.
- Preserve Unicode paths via Qt native QString interfaces rather than serializing paths through narrow `std::string` in Windows builds.
- Validate codec executable identity / version, sandbox or isolate untrusted multimedia input where practical.
- Define a stable codec contract and use a single job profile object per queued operation.

## Legal provenance
Original licensing: GPLv2-or-later + LameXP non-optional license addendum. No original files are bundled in this prototype. Review exact upstream license text, MUtilities license and codec redistribution license before packaging a derivative fork.
