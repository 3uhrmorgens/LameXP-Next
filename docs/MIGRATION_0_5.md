# Migration 0.5 technical notes

## Native LAME metadata pipeline

FFmpeg decodes source audio to PCM WAV. Native LAME encodes from WAV to temporary `encoded.mp3`. FFmpeg then remuxes encoded MP3 stream using `-c:a copy` and source file metadata using `-map_metadata 1` into a hidden staged output. The existing JobManager only moves to final destination after every process exits successfully, otherwise it cleans temporary files.

For tagged MP3 input with video stream `attached_pic` in the first video position, an embedded JPEG cover is copied through image re-encoding (`-c:v mjpeg`). No tag rewriting via LAME binary is needed. The CLI test uses a generated MP3 with artist, album, title and cover. Additional format-specific tag mapping still required.

## Output folder pattern

A folder pattern is split on **literal `/`**, then each segment is rendered with the existing filename-template engine and sanitized for Windows. This prevents tag values from introducing path separators. Empty or parent traversal (`..`) path segments are rejected. No directory creation occurs in the preview. JobManager creates destination directories at enqueue time.

## Known technical debt

Metadata probing is synchronous; cover probing adds a second process per MP3 input. Output commit uses QFile::rename (non-overwrite check is not atomic with respect to unrelated writers). PCM streaming and Windows integration tests remain unimplemented.
