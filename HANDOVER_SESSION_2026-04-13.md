# Handover — Session 2026-04-13

## Was committed und gepusht wurde

### Commit `6d50fe0` — WindowServer Crash Fix (closes #11)
- `setLookAndFeel(nullptr)` aus PluginEditor-Destruktor entfernt (primäre Crash-Ursache)
- `stopTimer()` in 6 Destruktoren: PromptPanel, SynthPanel, MainPanel, SequencerPanel, WaveformDisplay, SettingsPage
- `setLookAndFeel(nullptr)` aus SequencerPanel-Destruktor entfernt
- LnF-Deklarationsreihenfolge in SequencerPanel.h korrigiert (LnF vor Buttons)

### Commit `ad696e3` — Ghost Slider Visibility (closes #10)
- `SliderRow::paint()` → `paintOverChildren()` in GuiHelpers.h
- Ghost-Kreise rendern jetzt über dem Slider statt darunter

### Commit `1ad2822` — Microtuning Sampler (closes #9)
- `SamplePlayer::setTransposeRatio(double)` und `glideToRatio(double, float)` hinzugefügt
- SynthVoice berechnet `tunedHz(note)/tunedHz(60)` für Sampler-Modus
- Pre-existierender Double-Glide-Bug in renderPitchedBlock gefixt (Bypass-Check vor Block-Level-Advancement)

### Commit `0545c54` — CLAUDE.md
- Projekt-Regeln: Session-Start, Mandatory Process, JUCE Safety, Build, Quality, Agent Spawns

### Commit `1d0c52f` — Backend Error Reporting
- stderr des Backend-Prozesses wird nach `~/Library/Application Support/T5ynth/Logs/backend_stderr.log` umgeleitet
- `PipeInference::lastError_` speichert die echte Fehlermeldung
- UI zeigt den tatsächlichen Fehler statt generischem "Failed to start"

### Commit `7464c35` — CI/Installer Fixes
- CI Smoke-Test: akzeptiert "No model" als gültiges Ergebnis (CI hat keine Modelle)
- preinstall-Script: löscht alte T5ynth.app vor pkg-Installation (verhindert Debris von manuellen Installs)
- pkg-Version: nutzt Tag-Name nur bei Tag-Pushes, "0.0.0-dev" auf main
- CMakeLists VERSION 1.0.1 → 1.2.1 (Info.plist CFBundleVersion für Upgrade-Erkennung)

### Commit `8782668` — Installer Rename
- T5ynth-macOS.pkg → T5ynth-macOS-Installer.pkg

## Was NICHT committed ist

### Backend Code-Signing Fix (in build.yml, bereits gepusht)
- `codesign --force --deep --sign -` nach Backend-Injection in App-Bundle
- Ohne das blockiert macOS Gatekeeper den Backend-Subprocess auf End-User-Macs
- **Das ist die Ursache für "Backend: Failed to start" auf der Kollegin's Mac**

## Releases

- **v1.2.0** — gepusht und released, ABER: enthält nicht den Code-Signing-Fix für das Backend. Installer funktioniert auf End-User-Macs NICHT.
- **v1.2.1** — kein Tag gesetzt. Muss erst CPU-Problem gelöst werden.

## Offene kritische Punkte

### 1. CPU-Verbrauch 50% (BLOCKER für Release)
**macOS CPU Resource Report** (`T5ynth_2026-04-09-135620_MacBook-Pro.cpu_resource.diag`):
- 50% CPU Durchschnitt über 180 Sekunden
- Heaviest Stack: `TextEditor::drawContent → ShapedText → Shaper → CTFontCreateCopyWithAttributes`
- 25 von 28 Samples in JUCE's Text-Shaper
- Ursache: JUCE's TextEditor erstellt bei JEDEM Repaint den Font neu über CoreText — kein Caching
- Betroffene Components: `promptAEditor`, `promptBEditor`, `seedEditor` in PromptPanel
- **Geplanter Fix:** `setBufferedToImage(true)` auf den TextEditors — cached gerenderte Pixel, überspringt Shaper bei Parent-Repaints
- **Status: Nicht implementiert, nicht committed**

### 2. Detached Threads mit Use-after-free Risiko
- PromptPanel: Generation-Thread, Preload-Thread, Drift-Regen-Thread capturen `this` als Raw-Pointer
- MainPanel: Backend-Launch-Thread captured `this`
- SetupWizard: Download-Threads capturen `this`
- Bei Cmd-Q während laufender Generation: theoretisch Use-after-free
- **Fix:** `juce::Component::SafePointer<>` in callAsync-Lambdas
- **Status: Nicht implementiert**

### 3. Audio-Thread Allocations
- `SamplePlayer::primeStretcher()` allokiert 3 Vektoren auf dem Audio-Thread (bei jedem Note-On im Pitch-Shift-Modus)
- `SamplePlayer::preparePlaybackBuffer()` wird auf dem Audio-Thread aufgerufen (Buffer-Kopie + O(n²) Cross-Correlation)
- **Status: Bekannt, nicht gefixt**

### 4. exportJsonPreset() NULL-Pointer Crash (#12)
- SIGSEGV in exportJsonPreset() beim Preset-Speichern (Crash-Report vom 6. April)
- **Status: Nicht untersucht**

## Memory-System
- Komplett überarbeitet: 23 Feedback-Memories → 6 (feedback_00 bis feedback_05)
- feedback_00_product.md: "T5ynth ist ein Produkt für echte User" — höchste Priorität
- feedback_01_triage.md: Crashes blocken alles
- Veraltete Project-Memories gelöscht und aktualisiert

## Tickets
- #9 — Tuning Sampler (closed by 1ad2822)
- #10 — Ghost Slider Visibility (closed by ad696e3)
- #11 — WindowServer Crash (closed by 6d50fe0)
- #12 — exportJsonPreset NULL Crash (open)

## Zustand des Repos
- Branch: main, gepusht bis 7464c35
- v1.2.0 — released aber Backend-Launch auf End-User-Macs kaputt
- v1.2.1 — kein Tag, wartet auf CPU-Fix
- CI: grün auf main (7464c35)
