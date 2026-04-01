# Session 13 Handover — 2026-04-01 (spät abends)

## Thema: Device-Selector, Wavetable-Crash, UI-Reorganisation, DimExplorer-Design

### Was gemacht wurde

#### 1. Dual-Pipeline Device Selector (c2eaf50)
- Python `pipe_inference.py` lädt eine Pipeline pro verfügbarem Device (MPS + CPU auf Mac, CUDA + CPU auf PC)
- Neues Ready-Protokoll: `\x02` + uint16 Länge + JSON `{"devices": [...], "default": "..."}`
- JUCE `PipeInference` parst Device-Liste, neues `Request.device` Feld
- ComboBox in PromptPanel: Auto / MPS / CPU (CUDA auf PC)
- Info-Label zeigt Device: "3.2s | seed 123 | mps"
- Presets taggen `synth.device` — bei Import wird Device-Box entsprechend gesetzt
- Latent-Cache speichert auf CPU für device-agnostischen Zugriff

#### 2. Wavetable Race Condition Fix (173679a)
- **Crash**: SIGSEGV auf Audio-Thread — GUI Thread baut `mipFrames` während `processSample()` liest
- **Trigger**: Bracket-Drag in WaveformDisplay → `reextractWavetable()` → `generateMipLevels()` cleared `mipFrames`
- **Fix**: Double-Buffer mit `MipData mipSlots_[2]` + `std::atomic<int> activeSlot_`
  - GUI schreibt in inaktiven Slot, swappt atomar
  - Audio-Thread liest immer konsistente Daten
  - Shared-Mode Voices lesen via `sharedSource_->mipSlots_[sharedSource_->activeSlot_]`

#### 3. Pipe Inference Health Monitoring (173679a)
- `isChildAlive()` via `waitpid(WNOHANG)` — erkennt toten Subprocess
- `tryRestart()` — automatischer Neustart wenn Python stirbt
- `generate()` prüft vor jedem Request, startet bei Bedarf neu
- Fehler-Meldung im UI statt endloses "generating..."
- `backendDir_` wird gespeichert für Restart

#### 4. UI-Reorganisation (173679a, 5cd8c5a)
- **PresetPanel** aus Footer entfernt (nicht sichtbar, bleibt als Logic-Handler)
  - Import/Export-Funktionalität → wird ins JUCE-Standalone-Menü integriert (TODO)
- **DimensionExplorer** Mini-View in linker Spalte (unterhalb AxesPanel, 20% Höhe)
  - Klick öffnet Overlay
  - Explore-Button aus SynthPanel entfernt
- **StatusBar** bereinigt — keine Buttons, nur Status-Anzeige
- **Font-Size** Minimum auf 12px hochgesetzt, Budget auf 32 f-units

### Offene Punkte / Nächste Session

#### 1. DimensionExplorer — komplett neu aufbauen
Der aktuelle DimExplorer ist ein nutzloser 2D-Dot-Plot. Muss als **768-Kanal-Mischpult** neu gebaut werden:

**Design-Entscheidung (abgestimmt):**
- **768 vertikale Bars**, sortiert nach |A-B Differenz| (wichtigste links)
- **Farbe**: Richtung der Differenz (grün = A-Seite, orange = B-Seite)
- **Drag auf Bars**: setzt `dimension_offsets[dim_index]` — chirurgische Kontrolle
- **Kein Mode-Toggle**: automatisch A-B-Diff wenn Prompt B vorhanden, sonst absolute Aktivierung
- **Buttons**: "Anwenden + generieren" (grün), Undo (↩), Redo (↻), "Alle zurücksetzen" (orange)
- **Overlay**: schließt bei Klick außerhalb (kein separater Close-Button)
- **Mini-View**: in linker Spalte als Vorschau der Bars

**Pipeline fehlt noch:**
- `embedding_stats` vom Python-Backend (bereits berechnet in `_compute_stats()`) durch PipeInference ins JUCE bringen
- `PipeInference::Result` um Embedding-Stats erweitern
- Stats an DimensionExplorer durchreichen
- `dimension_offsets` zurück in den nächsten Generate-Request

#### 2. Standalone-Menü: "Settings" → "Menu"
- JUCE StandaloneFilterWindow hat ein "Settings"-Popup (Audio/MIDI, Save/Load State, Reset)
- Soll "Menu" heißen
- Import/Export Preset soll dort rein (statt toter PresetPanel)
- Model-Loading (SettingsPage) ist dort bereits als `extraSettingsPanel` eingebettet

#### 3. Overlay-Verhalten
- DimExplorer-Overlay schließt aktuell nur über Button — soll bei Klick außerhalb schließen
- Close/Reset Buttons sind im Overlay falsch positioniert → werden durch echte Funktions-Buttons ersetzt

### Geänderte Dateien

#### Session 13 Commits
- `c2eaf50` — dual-pipeline device selector
- `173679a` — wavetable race condition + UI reorg + inference monitoring
- `5cd8c5a` — revert StatusBar buttons, fix font size

#### Modifiziert
- `backend/pipe_inference.py` — dual pipeline, device routing, extended ready protocol
- `src/inference/PipeInference.h/cpp` — device in request, health monitoring, auto-restart
- `src/dsp/WavetableOscillator.h/cpp` — double-buffer MipData swap
- `src/gui/MainPanel.h/cpp` — DimExplorer in linker Spalte, PresetPanel hidden
- `src/gui/PromptPanel.h/cpp` — device ComboBox, font minimum 12px
- `src/gui/StatusBar.h/cpp` — bereinigt (nur Status)
- `src/gui/SynthPanel.h/cpp` — Explore-Button entfernt
- `src/gui/DimensionExplorer.h/cpp` — onClick callback
- `src/gui/PresetPanel.h/cpp` — importPreset/exportPreset public
- `src/PluginProcessor.h/cpp` — lastDevice, device in preset export

### Wichtige Erkenntnisse

1. **MPS vs CPU auf Apple Silicon** — Korrelation -0.54, fundamental unterschiedlicher Output. User MUSS Device wählen können, Auto-Detect reicht nicht. Presets sind device-spezifisch.

2. **Wavetable Double-Buffer** — `std::atomic<int>` Slot-Index ist die einfachste RT-sichere Lösung. Kein Mutex auf dem Audio-Thread. Shared-Mode Voices lesen via Pointer auf Master's aktiven Slot.

3. **Pipe Inference Subprocess kann sterben** — OOM, Python-Crash, etc. Ohne Health-Check hängt "generating..." ewig. `waitpid(WNOHANG)` + auto-restart löst das.

4. **DimensionExplorer Design** — "Relative" und "Absolute" aus dem Lab-Original sind beide nicht hilfreich. Die richtige View ist die **A-B Differenz sortiert nach Magnitude** — das zeigt dem User direkt welche Dims den Unterschied zwischen den Prompts ausmachen und ermöglicht gezieltes Feintuning.
