# Session 10 Handover — 2026-04-01

## Was gemacht wurde

### 1. Inferenz: MPS-Fix → CPU funktioniert
- `detectBestDevice()` forciert CPU (MPS TorchScript auf macOS broken)
- Modelle laden jetzt erfolgreich, Generation laeuft
- Commit: `8a915a2`

### 2. Inferenz: C++ TorchScript Pipeline → GESCHEITERT
- DiffusionScheduler: SDE-Formeln, ODE-Formeln, Euler — alles getestet
- **Root Cause**: Stable Audio Open braucht `BrownianTreeNoiseSampler` (korreliertes SDE-Noise aus torchsde). Nur in Python verfuegbar.
- Ohne BrownianTree: unabhaengiges Noise → Rauschen/Glitches, ODE → Stille, Euler → Stille
- TorchScript-Modelle sind KORREKT exportiert (validiert: max_diff < 6e-06 vs Original)
- Attention Mask Fix: Text-Padding korrekt maskiert (war vorher all-ones)
- **Ergebnis**: C++ Native Inference deprecated, Python Pipe Inference als Ersatz

### 3. Python Pipe Inference (FUNKTIONIERT)
- `backend/pipe_inference.py`: stdin/stdout Loop, laedt diffusers Pipeline
- `src/inference/PipeInference.h/cpp`: POSIX fork/pipe/exec, bidirektionale Kommunikation
- Protokoll: `\x02` (ready) → JSON request auf stdin → `\x01` + Header + float32 PCM auf stdout
- Patched last step: ODE fuer sigma→0 (torchsde crasht auf CPU bei finalem Step)
- Kein HTTP, kein Flask, keine Ports — transparent fuer User
- ~50s Generierung auf CPU (20 Steps, CFG 7, 3s Audio)
- Audio-Qualitaet: RMS sustained 0.76, tonal (nicht Noise), klar erkennbare Prompts
- Integration: MainPanel startet Python-Subprocess, PromptPanel nutzt PipeInference

### 4. Debug Sampler (FUNKTIONIERT)
- Bypass in processBlock: direktes PCM-Playback mit Pitch-Transposition
- Eingebauter 8-Step-Sequencer (C-Dur Tonleiter, 120 BPM, 1/8 Noten)
- Beweist: Sample-Playback + Transposition funktioniert in 50 Zeilen
- `loadGeneratedAudio()` fuellt Debug-Buffer, Sequencer spielt es ab

### 5. AudioLooper/Voice Chain: KAPUTT (nicht gefixt)
- Looper-Daten sind korrekt geladen (Debug bestaetigt: hasAudio=1, Samples vorhanden)
- Aber Playback produziert statischen Ton statt Buffer-Inhalt
- Voice Branch wird erreicht (isWT=0, hasAudio=1, engineMode=0)
- Ursache UNKLAR — Debug-Sampler Bypass umgeht das Problem

### 6. Sequencer/Arpeggiator: Teilweise gefixt
- Neue APVTS Params: `arp_enabled`, `arp_mode`, `arp_gate`, `seq_gate`, `seq_preset`
- Arpeggiator: Gate-Mechanismus (samplesUntilGateOff), setBaseNote() ohne Timing-Reset
- StepSequencer::stop(): emittiert jetzt NoteOff (vorher lastPlayedNote vorzeitig geloescht)
- Arp-Disable: haengende Note wird aufgeraeumt
- SequencerPanel UI: 4-Zeilen-Layout (Transport/StepCount/Division/BPM, Preset/Gate/Glide, StepGrid, Arp)
- **ABER**: Der echte Sequencer ist noch nicht an den Debug-Sampler angebunden (Debug-Seq ist hardcoded in processBlock)
- Loop-Mode propagiert nicht zu Voice-Loopers (shareBufferFrom kopiert Mode nur bei Distribution)

### 7. Alpha-Fix
- `gen_alpha` Default von 0.5 auf 0.0 korrigiert (0 = 50/50 Mix, nicht 0.75)

## Was NICHT funktioniert

### AudioLooper Playback
- **Symptom**: Statischer Ton mit verschiedenen Obertoenen je Note
- **Kein Silence**: Voice IST aktiv, Sampler-Branch wird erreicht
- **Vermutung**: processSample() liefert falschen Output, oder Buffer-Inhalt stimmt nicht mit sharedPlayBuffer ueberein
- **Workaround**: Debug-Sampler Bypass in processBlock

### Sequencer Integration
- Der UI-Sequencer (SequencerPanel) steuert StepSequencer via APVTS
- StepSequencer generiert MIDI Events in processBlock
- ABER: processBlock hat `return` im Debug-Bypass BEVOR der Sequencer laeuft
- Der Debug-Sequencer (hardcoded C-Dur) ist unabhaengig vom UI

### Inferenz-Performance
- 50s fuer 3s Audio auf CPU ist zu langsam fuer interaktiven Einsatz
- GPU wuerde das auf ~5s reduzieren
- Moegliche Optimierungen: MPS Backend in Python (nicht C++), halbe Precision, Caching

## Architektur (aktuell)

```
JUCE Standalone
├── processBlock (Audio-Thread)
│   ├── Debug-Sampler Bypass (AKTIV — funktioniert)
│   │   ├── Hardcoded 8-Step C-Dur Sequencer
│   │   ├── Pitch-Transposition per Note
│   │   └── return; (bypassed alles darunter)
│   ├── StepSequencer → Arpeggiator → VoiceManager (BYPASSED)
│   ├── Voice Rendering → Filter → Effects (BYPASSED)
│   └── Master Volume → Limiter (BYPASSED)
│
├── PipeInference (Python Subprocess)
│   ├── POSIX fork/pipe/exec
│   ├── stdin: JSON Request
│   ├── stdout: Binary PCM Response
│   └── diffusers Pipeline + BrownianTree SDE
│
├── PromptPanel → triggerGeneration()
│   ├── PipeInference.generate() (bevorzugt)
│   └── T5ynthInference.generate() (Fallback, broken)
│
└── loadGeneratedAudio()
    ├── debugSampleBuf (Debug-Sampler)
    ├── masterLooper (AudioLooper — broken playback)
    └── waveformSnapshot (Display — funktioniert)
```

## Naechste Schritte (Prioritaet)

1. **AudioLooper fixen** — Warum liefert processSample() falschen Output? Der Debug-Sampler beweist dass die Daten korrekt sind. Der Looper-Code muss Zeile fuer Zeile gegen den Debug-Sampler verglichen werden.

2. **Debug-Bypass entfernen** — Wenn der Looper funktioniert, Debug-Sampler raus, echter Seq/Voice-Chain reaktivieren.

3. **Sequencer an MIDI anbinden** — UI-Sequencer generiert MIDI → VoiceManager statt hardcoded C-Dur.

4. **GPU-Inferenz** — pipe_inference.py: MPS Device auf macOS, CUDA auf Linux. Halbiert/drittelt Generierungszeit.

5. **Legacy-Code aufraeumen** — BackendManager/BackendConnection/Flask entfernen. T5ynthInference optional kompilieren.

## Geaenderte Dateien

### Neue Dateien
- `backend/pipe_inference.py` — Python stdin/stdout Inference Loop
- `src/inference/PipeInference.h/cpp` — POSIX Pipe Client
- `tools/validate_torchscript.py` — TorchScript Model Validation
- `tools/validate_export.py` — Export Layer-by-Layer Comparison

### Modifizierte Dateien
- `CMakeLists.txt` — PipeInference.cpp hinzugefuegt
- `src/PluginProcessor.h/cpp` — PipeInference Member, Debug-Sampler Bypass, neue APVTS Params
- `src/gui/MainPanel.h/cpp` — tryLoadInferenceModels → Pipe + Native Fallback
- `src/gui/PromptPanel.cpp` — PipeInference bevorzugt, Native Fallback
- `src/gui/SequencerPanel.h/cpp` — Komplettes UI Rebuild
- `src/inference/DiffusionScheduler.cpp` — ODE statt SDE (deprecated)
- `src/inference/T5ynthInference.cpp` — Attention Mask Fix, MPS disabled (deprecated)
- `src/sequencer/Arpeggiator.h/cpp` — Gate, setBaseNote Fix
- `src/sequencer/StepSequencer.cpp` — stop() NoteOff Fix
- `src/dsp/AudioLooper.cpp` — Debug Logging (noch drin)
- `src/dsp/SynthVoice.cpp` — Debug Logging (noch drin)

## Commits (Session 10)
- `8a915a2` — fix(inference): force CPU device + dynamic latent size + duration trim
- `feaf878` — feat: Python pipe inference + debug sampler + sequencer/arp rework

## Wichtige Erkenntnisse

1. **BrownianTree ist nicht optional**: Stable Audio Open produziert mit jedem anderen Noise-Typ Muell. Das ist kein Qualitaetsverlust sondern komplett unbrauchbar.

2. **JUCE ChildProcess hat kein stdin-Write**: Musste POSIX pipe/fork/exec verwenden.

3. **AudioLooper Bug ist NICHT in den Daten**: Buffer-Inhalt ist korrekt (Debug bestaetigt). Das Problem liegt in processSample() oder der Voice-Integration. Der Debug-Sampler mit identischer Logik (read position + speed ratio + interpolation) funktioniert.

4. **torchsde crasht auf CPU bei sigma→0**: Workaround: letzter Step als ODE (denoised = model_output). Aendert Ergebnis minimal.
