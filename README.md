# LameXP Next v0.8 — GitHub onboarding / Windows CI preparation

**Source prototype; no Windows build has yet been executed.** Builds must be confirmed by GitHub Actions after importing the project.

- Builds on v0.7 (C++20, Qt 6, FFmpeg/LAME process pipeline, metadata scanning and output naming).
- CI fix: deploy Qt plugins using `windeployqt` **before** the offscreen startup smoke test.
- The Windows CI artifact is only uploaded if the preceding Windows build, tests and GUI smoke test pass. Codec executables are not bundled.
- Tests rerun under Linux: four C++ core tests, three external audio smoke scripts. All passed in the local environment.
- The original upstream LameXP/MUtilities source tree is still not fully imported or migrated. Check copyright and license obligations before publishing binary distributions.

## Import into GitHub

Repository: https://github.com/3uhrmorgens/LameXP-Next

Unzip this archive, copy **the contents** of the extracted folder into a checkout of the GitHub repository (not the outer versioned directory), keeping its `.github/workflows/build.yml` file. Commit and push.

Do not commit `out/`, temporary binary test outputs, local codec executables or Qt runtime DLLs.

## Linux core verification

```sh
cmake --preset linux-core
cmake --build --preset linux-core
ctest --preset linux-core --output-on-failure
python3 tests/SmokePipeline.py
python3 tests/SmokeMetadata.py
python3 tests/SmokeStreaming.py
```

## Windows

The workflow `.github/workflows/build.yml` is designed for GitHub-hosted Windows 2022 / MSVC + Qt 6.8.3. Until it runs successfully, do not claim the Windows executable is tested.
