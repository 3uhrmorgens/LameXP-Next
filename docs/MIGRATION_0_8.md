# v0.8 migration report

Repository setup: GitHub connected; project README updated to describe incomplete import and Windows CI acceptance criteria.

CI bug fixed in source archive: Qt offscreen startup test now executes after `windeployqt` rather than before it, so it has the required platform plugin in the deployed `dist` directory.

Local validation: 4/4 C++ core tests and 3/3 FFmpeg/LAME external audio smoke checks passed in Linux. No MSVC/Qt Windows build has been executed yet.

Next: import full project into GitHub, run workflow and fix any actual Windows failures.
