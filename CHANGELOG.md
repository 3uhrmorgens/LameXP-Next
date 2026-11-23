# Changelog

Alle Versionen vor der ersten stabilen Veröffentlichung sind **Entwicklungs- und Teststände**. Änderungen werden erst nach erfolgreichem Build und – soweit relevant – Praxistest als bestätigt beschrieben.

## v0.9 – Portable Windows-x64-Teststand

- GitHub-Actions-Build unter Windows mit Qt 6 und MSVC 2022 bestätigt.
- Portable Paketierung mit FFmpeg und FFprobe vorbereitet und erfolgreich in CI getestet.
- Automatische Suche nach `codecs/ffmpeg.exe`.
- Windows-Praxistests: WAV → FLAC, MP3, Opus; FLAC → MP3; metadatenbasierte Benennung erfolgreich.
- Bekanntes Problem: Upload-Pfad des v0.9-ZIP im Workflow auf `main` muss noch korrigiert werden, damit neue Actions-Läufe stets ein Artefakt liefern.

## In Entwicklung (noch nicht Teil des bestätigten v0.9-Teststands)

- Drag & Drop, Prüfung tatsächlicher Audiospuren und verständliche Ablehnung ungeeigneter Dateien.
- UI-Anzeige und Auswahl von MP3-VBR-Qualität, Opus-Bitrate und Kanalmodus.
- Siehe [Pull Request #2](https://github.com/3uhrmorgens/LameXP-Next/pull/2).

## Frühere Prototypen v0.1–v0.8

- C++20-/Qt-6-/CMake-Grundgerüst, separate Codec-Planung und Konvertierungsjobs.
- Direkter LAME-Pfad mit PCM-Streaming, Metadatenübernahme sowie Dateinamen-/Ordnerregeln.
- Asynchrone FFprobe-Metadatenanalyse mit Cache.
- Erster erfolgreich ausgeführter Windows-x64-Build in v0.8.

Die frühe Entwicklung ist teilweise nur durch Quellcode-Prototypen und Testläufe dokumentiert; frühere Versionsnummern sind keine offiziellen Releases.
