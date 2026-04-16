# Handover — Session 2026-04-11

## Was erledigt und committed wurde

### Commit `1ae5c05` — PID-Namespace + Cleanup (closes #3, #6, #8)
- `PID::` constexpr-Namespace in BlockParams.h: 122 Parameter-IDs als Single Source of Truth
- 582 String-Literale in 9 Dateien durch `PID::xxx` ersetzt
- `EnvPIDs`/`LfoPIDs`/`DriftPIDs`-Structs für Preset Save/Load
- LfoWave::kEntries Labels auf "Sin"/"Sq" (kurze Form), GUI nutzt kEntries
- Button "New" → "Init" umbenannt (StatusBar.h)
- INIT-Preset: LFO/Drift Targets auf "none" (---) gesetzt

### Commit `9b5067b` — Buffer-Preset (closes #4)
- Standalone quit: speichert State als `_buffer.t5p` (APVTS + Audio + Embeddings + Prompts + Axes)
- Standalone start: lädt `_buffer.t5p` falls vorhanden, sonst DEMO
- Pfad: `~/Library/Application Support/T5ynth/_buffer.t5p` — MUSS auf `/Library/Application Support/T5ynth/_buffer.t5p` umgestellt werden (siehe offene Punkte)

### Commit `9e0aaf2` — WT-Extraction + Scan→P1 (closes #5, #7)
- `wtExtractStartFrac_`/`wtExtractEndFrac_` in SamplePlayer (unabhängig von Sampler P2/P3)
- UI-Bracket-Handles routen mode-abhängig (WT → Extract, Sampler → P2/P3)
- Preset JSON: `engine.wtExtractStart`/`wtExtractEnd`, Fallback auf P2/P3
- "Scan" Mod-Target → P1 in Sampler-Mode (Drift + LFO Offset bei Retrigger)
- `startPosOffset_` in SamplePlayer, per-Block berechnet in processBlock

### Commit `cf78d2c` — Audio-Fixes + UX (pushed als v1.1.0)
- RMS-Normalisierung (-12 dBFS) statt Peak (0.95) — eliminiert >20dB Schwankungen
- Delay Time/FB: per-Sample Interpolation (5ms Ramp) statt Block-Sprünge
- Delay FB Modulation: additiv statt multiplikativ
- GenSeq Velocity: Range 0.7–1.0 (war 0.4–1.0), Ghost-Dämpfung 0.75× (war 0.4×)
- DriftTarget reorder: Emb. Noise + Magnitude direkt nach Alpha
- Start-Position Slider disabled bei SA small und AudioLDM2
- LICENSE aus SA small Required-Files und Downloads entfernt

## Was NICHT committed ist (unstaged changes)

### Modell-Pfad: `/Library/Application Support/T5ynth/models/`
- `getAppSupportModelDir()` in SetupWizard.cpp zeigt jetzt auf `/Library/Application Support/T5ynth/models/`
- Scan-Fallbacks: alte `~/Library/Application Support/` Location + HF-Cache
- Build ist durch, aber NICHT committed

## Offene kritische Punkte

### 1. macOS `.pkg`-Installer (HÖCHSTE PRIORITÄT)
`/Library/Application Support/` ist nur mit root-Rechten beschreibbar. Alle Audio-Apps (NI, KORG, iZotope etc.) lösen das über `.pkg`-Installer. T5ynth braucht einen.

Der Installer muss:
- `/Library/Application Support/T5ynth/models/` anlegen
- App nach `/Applications/` kopieren
- AU nach `~/Library/Audio/Plug-Ins/Components/` kopieren
- VST3 nach `~/Library/Audio/Plug-Ins/VST3/` kopieren
- CI-Build muss `.pkg` erzeugen (pkgbuild/productbuild)

Ohne Installer kann die App den Modell-Ordner nicht anlegen. Der aktuell im Settings-Dialog gebaute Auto-Scan/Copy-Mechanismus funktioniert nicht ohne den Ordner.

### 2. Buffer-Preset Pfad
`MainPanel.cpp:getBufferPresetFile()` nutzt noch `userApplicationDataDirectory` (= `~/Library/Application Support/T5ynth/`). Das ist der versteckte per-User-Ordner. Für den Buffer-Preset ist das akzeptabel (User muss den nie finden), aber sollte konsistent sein.

### 3. Preset-Ordner Pfad
`PresetFormat::getPresetsDirectory()` nutzt ebenfalls `userApplicationDataDirectory`. Gleiche Frage: sollen User-Presets nach `/Library/Application Support/T5ynth/presets/` oder bleiben sie im User-Ordner?

### 4. Ghost-Display für FX-Panel
FxPanel hat keinen Processor-Ref und keinen Timer — Ghosts für Delay/Reverb werden berechnet (PluginProcessor.cpp) aber nie im GUI angezeigt. Braucht Processor-Ref Durchreichen + Timer-Callback.

### 5. LFO → Delay Time Audio-Wirkung
Per-Sample Delay-Smoothing ist implementiert, aber noch nicht vom User getestet/bestätigt. Die Modulation ist subtiler als z.B. Filter-LFO — bei niedrigem Depth schwer hörbar.

## Zustand des Repos
- Branch: `main`, gepusht bis 89babf6
- Kein Release-Tag — v1.2.0 noch nicht getaggt
- CI-Build für v1.1.0 ist das letzte stabile Release

---

# Session 2 (Nachmittag 2026-04-11)

## Was erledigt und committed wurde

### Installer-Fixes (alle E2E-verifiziert mit installierter App)
- `84871cc` wandb-Mock `__spec__` fix — Backend startet
- `c1c622c` Per-user Model-Pfad Priorität
- `289b99d` UTF-8 Mojibake → ASCII
- `2888272` Realistische Gatekeeper Release Notes
- `298e47b` Kohärente UI-Status bei Backend-Fehler

### GenSeq
- `ddd18d9` 21 neue Skalen incl. non-Western (Bitmasks verifiziert, Pentatonic-Bug gefixt)
- `595dad1` Euclidean Accent Machine statt Velocity (binär: accent/normal/ghost)
- `e671076` Ghost Notes auf Rest-Positionen

### Tuning-System (NICHT FUNKTIONAL)
- `1495c37` + `1d83d73` + `81d41ac` Code kompiliert, crasht nicht, aber kein hörbarer Effekt
- TODO in Tuning.h, debugging steht aus

### Ghost-Slider
- `62e6754` Magnitude + Emb Noise
- `a1cddf8` Delay Time/FB/Mix + Reverb Mix

### Crash-Fixes
- `9cff7c3` setLookAndFeel(nullptr) entfernt
- `89babf6` FxPanel stopTimer() im Destruktor

### Generate-Button
- `5b03ce5` Guard + setEnabled wiederhergestellt (war fälschlich in 98d7596 + 86d0d10 entfernt)

## Offene Punkte

### Release v1.2.0
- RC-Tags gelöscht, kein neuer Tag
- PyInstaller-Bundle muss neu gebaut werden
- E2E-Test mit INSTALLIERTER App (nicht Dev-Build!)
- Erst nach E2E taggen

### Tuning-System
- Maqam (-50c) und Pelog (-60c) müssten hörbar sein
- Verdacht: Table wird gebaut aber nicht an Oszillator weitergegeben

### Drift ohne Sequencer
- Architektur-Bug: Drift-LFOs laufen nur bei aktivem Sequencer
- Generate ohne Sequencer + gleiche Settings = identischer Output
- War die eigentliche Ursache des "Button macht nichts"-Reports

## Schwere Fehler dieser Session
1. Zwei Crashes eingeführt (setLookAndFeel, FxPanel Timer) — macOS UI zerschossen
2. Generate-Button kaputt gemacht durch Entfernen des Pipe-Guards
3. Dev-Build statt installierte App getestet
4. v1.2.0-Tag ohne E2E gepusht (zurückgezogen)
5. Tuning implementiert ohne hörbare Verifikation
6. User wiederholt als Alpha-Tester missbraucht
