# Session 9 Handover — 2026-03-31

## Was gemacht wurde

### 1. macOS Setup
- Repo geklont, JUCE 8.0.6 hinzugefuegt, LibTorch 2.11.0 ARM64 heruntergeladen
- Python venv mit Backend-Dependencies erstellt
- Standalone + AU erfolgreich gebaut (VST3 hat macOS-Signing-Problem, wird auf Fedora gebaut)
- libomp-Pfad in libtorch_cpu.dylib gefixt (`install_name_tool`)

### 2. Settings-Dialog
- JUCE Standalone "Options" Button umbenannt zu "Settings" (in `juce_StandaloneFilterWindow.h` Zeile 754)
- `StandalonePluginHolder` um `Component* extraSettingsPanel` erweitert (Zeile 443)
- `SettingsComponent` rendert das extraSettingsPanel unterhalb der Audio/MIDI-Einstellungen
- Dialog-Titel geaendert zu "T5ynth Settings" (Zeile 310)
- Model-Panel (`SettingsPage` in `SetupWizard.h/.cpp`) zeigt:
  - Model-Status + Pfad
  - Auto-Scan / Browse Buttons
  - HuggingFace Token Feld (persistent in `~/Library/T5ynth/settings.json`)
  - Download-Button mit Progress-Bar (HF API file listing + sequentielles Herunterladen)
  - Klare Anleitung (Read-Only Token, huggingface-cli Alternative, Browse)

### 3. Modell-Download
- Stable Audio Open 1.0 erfolgreich heruntergeladen nach `~/Library/T5ynth/models/stable-audio-open-1.0/`
- ACHTUNG: JUCE `userApplicationDataDirectory` auf macOS = `~/Library`, NICHT `~/Library/Application Support`
- Python config.py entsprechend angepasst

### 4. TorchScript Export
- Alle 4 Modelle erfolgreich exportiert (0.00 max diff):
  - `t5_encoder.pt` (438.9 MB)
  - `projection_model.pt` (1.6 MB)
  - `dit.pt` (4228.7 MB) — TraceSafeAttnProcessor funktioniert
  - `vae_decoder.pt` (624.8 MB)
  - Plus: spiece.model, scheduler_config.json, pca_components.pt, vae_meta.json
- Exportiert nach `~/Library/T5ynth/exported_models/`
- WICHTIG: transformers muss < 5.0.0 sein (5.x hat Breaking Change in Mask-Erstellung)

### 5. Backend-Fallback entfernt
- PromptPanel nutzt ausschliesslich native T5ynthInference
- Kein Python, kein HTTP, kein Flask mehr im Generation-Pfad
- BackendManager/BackendConnection Code ist noch in den Quellen, wird aber nicht mehr aufgerufen

## Was NICHT funktioniert

### Model Load Failed
Die App findet `dit.pt` im richtigen Verzeichnis, aber `T5ynthInference::loadModels()` wirft eine Exception. Der Fehler wird geloggt via `juce::Logger::writeToLog()` aber die Meldung ist in der GUI nicht sichtbar — nur "Model load failed" in der StatusBar.

**Naechster Schritt: Debugging**
1. Die Exception-Message aus `loadModels()` catch-Block sichtbar machen (z.B. in StatusBar oder Logfile)
2. Moegliche Ursachen:
   - LibTorch Version (2.11.0) inkompatibel mit dem PyTorch (2.11.0) der den Export gemacht hat — sollte passen
   - MPS vs CPU Device Mismatch — Modelle wurden auf CPU exportiert, LibTorch macOS ist CPU-only
   - Speicher: dit.pt ist 4.2 GB, muss komplett in RAM geladen werden
   - SentencePiece `spiece.model` Pfad/Format-Problem

### Kleinere UI-Issues
- "Backend: Not connected" in Settings ist jetzt irrelevant (Backend entfernt) — Text/Label entfernen
- UTF-8 Encoding: "—" wird als "â€"" angezeigt in PromptPanel infoLabel
- Progress-Bar (0%) bleibt sichtbar wenn Modell bereits gefunden
- Nach erfolgreichem Download Token/Buttons ausblenden funktioniert erst beim naechsten Dialog-Oeffnen

## Geaenderte Dateien

### Eigener Code
- `src/PluginEditor.cpp` + `.h` — registriert Model-Panel via StandalonePluginHolder::getInstance()
- `src/gui/MainPanel.cpp` + `.h` — Backend-Fallback entfernt, tryLoadInferenceModels sucht in ~/Library/T5ynth/exported_models
- `src/gui/SetupWizard.cpp` + `.h` — komplettes Model-Management (Download, Browse, Scan, Token)
- `src/gui/StatusBar.cpp` + `.h` — vereinfacht (nur Punkt + Text)
- `src/gui/PromptPanel.cpp` — nur noch native Inference, kein HTTP-Fallback
- `src/backend/BackendManager.cpp` — .venv Pfad hinzugefuegt (irrelevant da Backend nicht mehr genutzt)
- `backend/config.py` — macOS App Data Pfad korrigiert
- `backend/requirements.txt` — torchsde hinzugefuegt

### JUCE Modifikationen (NICHT im Git, da JUCE nicht getrackt wird)
- `JUCE/modules/juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h`:
  - Zeile 443: `Component* extraSettingsPanel = nullptr;` in StandalonePluginHolder
  - Zeile 571-573: SettingsComponent rendert extraSettingsPanel
  - Zeile 606-610: Layout fuer extraSettingsPanel
  - Zeile 634: Groessenberechnung inkl. Panel-Hoehe
  - Zeile 754: Button-Text "Settings" statt "Options"
  - Zeile 310: Dialog-Titel "T5ynth Settings"
- Diese Aenderungen muessen bei JUCE-Updates erneut angewendet werden!

## Architektur-Entscheidungen
- **Kein Python-Backend**: Die gesamte Inference laeuft nativ in C++ via LibTorch. Das Python-Backend war ein Legacy-Fallback der nie haette existieren sollen.
- **Modell-Speicherort macOS**: `~/Library/T5ynth/` (JUCE userApplicationDataDirectory)
- **Modell-Speicherort Linux**: `~/.local/share/T5ynth/` (noch nicht getestet)
- **HF Token**: Persistent in `~/Library/T5ynth/settings.json`, Read-Only reicht
- **Export**: `tools/export_to_torchscript.py` mit transformers < 5.0.0

## Commits
- `5c4845a` — feat(settings): integrate model download into JUCE settings dialog
- `3b15f95` — remove HTTP backend fallback — native LibTorch inference only
