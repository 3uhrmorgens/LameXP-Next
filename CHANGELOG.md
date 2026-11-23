# Changelog

Alle Versionen vor der ersten stabilen Veröffentlichung sind **Entwicklungs- und Teststände**. Änderungen werden erst nach erfolgreichem Build und – soweit relevant – Praxistest als bestätigt beschrieben.

## v0.9 – Portable Windows-x64-Teststand

- GitHub-Actions-Build unter Windows mit Qt 6 und MSVC 2022 bestätigt.
- Portable Paketierung mit FFmpeg und FFprobe vorbereitet und erfolgreich in CI getestet.
- Automatische Suche nach `codecs/ffmpeg.exe`.
- Windows-Praxistests: WAV → FLAC, MP3, Opus; FLAC → MP3; metadatenbasierte Benennung erfolgreich.
- GitHub-Actions-Upload-Pfad für das portable ZIP im Entwicklungsbranch korrigiert und in `main` übernommen.

## v0.9.1 – Bedienungsverbesserungen (in `main` übernommen)

- Drag & Drop aus dem Explorer; im Windows-Praxistest bestätigt.
- FFprobe-Prüfung auf Audiospuren; JPG-Ablehnung mit verständlicher Fehlermeldung getestet.
- MP3-VBR-Qualitätswahl, Opus-Zielbitrate sowie Mono-/Stereo-Ausgabe: Bedienung und Mono-/Stereo-Konvertierung im Praxistest bestätigt.
- [Pull Request #2](https://github.com/3uhrmorgens/LameXP-Next/pull/2) wurde in `main` zusammengeführt.

## In Entwicklung

- MP3 CBR/ABR, wählbare Samplerate und portable Konvertierungsprofile.
- FFprobe-gestützte Integrationstests der tatsächlichen Ausgabedateien.

## Frühere Prototypen v0.1–v0.8

- C++20-/Qt-6-/CMake-Grundgerüst, separate Codec-Planung und Konvertierungsjobs.
- Direkter LAME-Pfad mit PCM-Streaming, Metadatenübernahme sowie Dateinamen-/Ordnerregeln.
- Asynchrone FFprobe-Metadatenanalyse mit Cache.
- Erster erfolgreich ausgeführter Windows-x64-Build in v0.8.

Die frühe Entwicklung ist teilweise nur durch Quellcode-Prototypen und Testläufe dokumentiert; frühere Versionsnummern sind keine offiziellen Releases.
