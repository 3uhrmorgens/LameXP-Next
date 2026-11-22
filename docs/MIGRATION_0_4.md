# LameXP Next 0.4 – metadata naming migration

The new `core/OutputNaming` C++20 module is Qt-free, using UTF-8 strings. `MainWindow` probes tags via `ffprobe` JSON, mapping common tag aliases, and passes the tags through the naming engine. Filename sanitization and source-protection occur before `JobManager::enqueue`.

The UI shows a source/destination preview. It currently scans synchronously; replace with an asynchronous metadata index, cache keyed by file metadata and source path, and add editable preview policies before production.

**Preservation scope:** output *names* only. It does not yet preserve tags or artwork in the direct LAME pipeline. File and path protection are maintained by staged outputs and non-overwrite finalization.

**Validation:** Naming, pipeline, LAME adapter C++20 unit tests pass on Linux; external FFmpeg→LAME smoke test passes. Windows/Qt build not executed.
