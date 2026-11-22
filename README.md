# LameXP Next

Modernisierungsprojekt für einen Audio-Konverter unter Windows 10/11 (x64).

> **Aktueller Status:** Entwicklungsprototyp. Der lokale v0.7-Quellcode ist noch nicht vollständig in dieses GitHub-Repository importiert. **Es gibt noch keinen erfolgreich geprüften Windows-x64-Build.**

## Projektziel

- C++20, CMake, Qt 6 (zunächst Widgets)
- Modulare Audioverarbeitung mit FFmpeg/ffprobe und LAME
- Direkte PCM-Streaming-Pipeline statt temporärer großer WAV-Dateien
- Sichere Metadatenübernahme inkl. Coverbilder
- Batch-Konvertierung und dateinamen-/ordnerbasierte Metadatenmuster
- Begrenzte Parallelisierung, asynchrone Metadatenanalyse
- Nachweisbar getestete Windows-x64-Artefakte mit GitHub Actions

## Nächster Schritt

1. Den **vollständigen v0.7-Projektstand** ohne `out/`, Cache-Dateien und lokale Binärdateien importieren.
2. Originalcode, MUtilities und Drittanbieter-Komponenten getrennt nach Herkunft und Lizenz prüfen.
3. CMake-Core-Tests und Audio-Smoke-Tests unter Linux ausführen.
4. MSVC 2022 + Qt 6.8.3 Build unter Windows ausführen, Buildfehler beheben.
5. Erst nach positivem CI-Ergebnis eine Windows-Testversion als Artefakt veröffentlichen.

## Lizenz und Herkunft

Technische Ausgangsbasis und Referenz: [LameXP Release_421](https://sourceforge.net/p/lamexp/code/ci/Release_421/tree/). Das bisherige neue Gerüst ist **kein** vollständiger Import des Originalprojekts. Vor Übernahme von fremdem Quellcode sind dessen Copyright- und Lizenztexte beizubehalten und die jeweiligen Bedingungen zu prüfen. Die Lizenz dieses neuen Repository-Inhalts muss vor einer öffentlichen Distribution verbindlich geklärt werden.

**Wichtig:** Die Anwesenheit dieses README bedeutet nicht, dass hier bereits eine kompilierbare Anwendung liegt.
