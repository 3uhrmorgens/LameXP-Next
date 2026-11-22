# Quellcode-Import und erster Windows-Build

Der vollständige lokale v0.8-Prototyp liegt als `LameXP-Next-0.8.zip` in der ChatGPT-Unterhaltung vor. GitHub enthält derzeit noch **nicht** alle C++-Quelldateien; deshalb läuft hier noch kein Windows-CI-Build.

## Import über GitHub-Weboberfläche

1. ZIP aus der ChatGPT-Unterhaltung herunterladen und auf dem Mac entpacken.
2. Das Repository [3uhrmorgens/LameXP-Next](https://github.com/3uhrmorgens/LameXP-Next) öffnen.
3. **Add file → Upload files** wählen.
4. Die **Inhalte** des entpackten Ordners hochladen; darauf achten, dass auch versteckte Dateien wie `.github/workflows/build.yml` enthalten sind. Für versteckte Dateien ist der Git-CLI-Weg zuverlässiger.
5. Den bestehenden `README.md` dabei nicht versehentlich mit einer irreführenden Statusbeschreibung überschreiben. Erst nach erfolgreichem Build auf einen bestätigten Release-Status umstellen.
6. Auf GitHub unter **Actions** den Workflow `LameXP Next - build and test` und anschließend die Windows-Job-Logs öffnen.

## Alternativ per Git (empfohlen für versteckte Dateien)

```sh
git clone https://github.com/3uhrmorgens/LameXP-Next.git
cd LameXP-Next
# Inhalte des entpackten v0.8-Ordners hier hinein kopieren,
# inklusive .github/, aber ohne out/ oder Compiler-Artefakte.
git add .
git commit -m "Import LameXP Next v0.8 prototype and Windows CI"
git push origin main
```

## Abnahmekriterien

- Windows Server 2022 Runner / MSVC 2022 / Qt 6.8.3: Konfiguration, Kompilierung und CTest erfolgreich
- erzeugte `LameXPNext.exe` hat PE-Machine-Architektur AMD64
- `windeployqt` ist vor dem GUI-Starttest ausgeführt worden
- GUI startet mit Qt-Offscreen-Plugin, ohne sofort abzustürzen
- Qt-only ZIP-Artefakt wird nur nach erfolgreichem Job erzeugt
- Codecprogramme sind nicht im Qt-only Paket enthalten

Die drei externen FFmpeg/LAME-Smoke-Tests im Ubuntu-Job prüfen Codec-Prozesse, ersetzen aber keinen Windows-End-to-End-Test.

## Lizenzhinweis

Vor Distribution von LameXP-Originalcode/FFmpeg/LAME/Qt-Binärdateien Copyright- und Lizenzverpflichtungen prüfen. Der bisherige Prototyp ist kein vollständiger Import von Release_421.
