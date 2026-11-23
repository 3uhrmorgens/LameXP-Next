# LameXP Next

**Portabler Audio-Konverter für Windows 10/11 (x64) – Entwicklungsprojekt auf Basis von C++20 und Qt 6.**

> **Aktueller Stand: v0.9.1 (Testversion).** Ein Windows-x64-Build und erste Konvertierungen wurden erfolgreich getestet. Die Anwendung ist noch keine abschließend getestete oder als offizielles Release freigegebene Version.

LameXP Next ist ein Modernisierungs- und Fork-Projekt, das sich am ursprünglichen [LameXP Release_421](https://sourceforge.net/p/lamexp/code/ci/Release_421/tree/) orientiert. Der Originalquellcode und MUtilities wurden noch **nicht vollständig portiert**. LameXP Next befindet sich daher weiterhin in Entwicklung.

## Funktionen (v0.9.1)

- Konvertierung zwischen mehreren Audioformaten, derzeit insbesondere **WAV, FLAC, MP3 und Opus**
- **Portables Windows-x64-Paket** mit Qt-Laufzeitkomponenten und FFmpeg/FFprobe
- Automatische Erkennung von `codecs/ffmpeg.exe` neben der Anwendung; manueller Codec-Pfad bleibt möglich
- MP3 über FFmpegs `libmp3lame`; optionaler direkter LAME-Modus erfordert separat `lame.exe`
- Begrenzte parallele Konvertierungsjobs
- Asynchrone Metadatenanalyse mit Cache
- Automatische Ausgabe-Dateinamen und optionale Unterordner aus Metadaten (z. B. `{artist}/{year} - {album}`)
- Metadaten-/Coverübernahme je nach Quell- und Zielformat
- Drag & Drop aus dem Windows-Explorer
- Prüfung auf Audiospuren mit verständlicher Ablehnung von Dateien ohne Audiospur (z. B. JPG)
- Einstellbare MP3-VBR-Qualität, Opus-Zielbitrate und Mono-/Stereo-Ausgabe

### Tatsächlich überprüft

Ein GitHub-Actions-Workflow hat bereits erfolgreich eine Windows-x64-Anwendung mit MSVC 2022/Qt 6 gebaut, gestartet und die Core-Tests ausgeführt. In einem Anwender-Praxistest wurden **WAV → FLAC, MP3 und Opus**, **FLAC → MP3**, die **metadatenbasierte Benennung der Ausgabedateien**, **Drag & Drop**, **JPG-Ablehnung**, die Auswahl von **MP3-Qualität und Opus-Bitrate** sowie die **Mono-/Stereo-Ausgabe** erfolgreich geprüft.

Das ist **keine** vollständige Regressionstest-Abdeckung und keine Garantie für sämtliche Formate, Dateien und Systeme.

## Download / Testen

- [GitHub Actions – Windows-Testbuilds](https://github.com/3uhrmorgens/LameXP-Next/actions)
- [Builds und Quellcode](https://github.com/3uhrmorgens/LameXP-Next)
- [Entwicklung und Pull Requests](https://github.com/3uhrmorgens/LameXP-Next/pulls)

Unter **Actions** einen erfolgreich abgeschlossenen Build öffnen und im Abschnitt **Artifacts** das Windows-Paket herunterladen, sofern ein solches Artefakt vorhanden ist. Für das Herunterladen von Actions-Artefakten kann eine GitHub-Anmeldung erforderlich sein. Der Upload-Schritt verwendet nun den korrekten Paketnamen; bitte bei einem konkreten Lauf prüfen, ob das Artefakt tatsächlich erstellt wurde.

Das ZIP auf einem Windows-10/11-x64-System entpacken und `LameXPNext.exe` starten. Der geplante Paketaufbau enthält `codecs/ffmpeg.exe` und `codecs/ffprobe.exe`. **Nicht aus Quellcode-Snapshots eine funktionsfähige EXE ableiten**; dafür den entsprechenden Build verwenden.

## In Entwicklung

- MP3-CBR/ABR-Bitraten, Sampleraten und portable, speicherbare Konvertierungsprofile
- Automatisierte FFprobe-Prüfung der tatsächlichen Ausgabedatei-Parameter
- Verbesserte Windows-End-to-End- und GUI-Tests

Die getesteten Funktionen aus [Pull Request #2](https://github.com/3uhrmorgens/LameXP-Next/pull/2) wurden in `main` übernommen. Neue Encoder-Funktionen werden gesondert entwickelt und getestet.

## Entwicklung / Architektur

- C++20, Qt 6 Widgets, CMake, MSVC 2022
- FFmpeg/FFprobe als externe Codec- bzw. Analyseprogramme
- Separate Planungs-/Adapter-Klassen und eine QProcess-Jobverwaltung
- PCM-Pipeline mit Streaming für den optionalen direkten LAME-Weg
- Automatisierte Windows- und Linux-Tests mit GitHub Actions

### Core-Tests unter Linux

```sh
cmake --preset linux-core
cmake --build --preset linux-core
ctest --preset linux-core --output-on-failure
python3 tests/SmokePipeline.py
python3 tests/SmokeMetadata.py
python3 tests/SmokeStreaming.py
```

Die externen Audio-Smoke-Tests benötigen FFmpeg, FFprobe und ggf. LAME auf dem Testsystem.

## Status, Lizenzen und Herkunft

**Entwicklungssoftware – keine offizielle stabile Veröffentlichung.** Vor öffentlicher Binärdistribution sind insbesondere die Original-LameXP-Lizenzbedingungen sowie die konkreten Lizenz- und Quellcodepflichten von **Qt, FFmpeg, LAME und weiteren Drittanbieterkomponenten** zu prüfen und zu dokumentieren. Nicht jede Codec-Binärdatei darf ohne weitere Bedingungen weiterverteilt werden.

Weitere Informationen: [Portable v0.9](docs/PORTABLE_0.9.md) · [Import und Build](docs/IMPORT_AND_BUILD.md).

---
*Die README wird mit bestätigten Entwicklungsständen aktualisiert. Noch nicht freigegebene Features werden getrennt ausgewiesen.*
