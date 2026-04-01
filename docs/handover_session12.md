# Session 12 Handover — 2026-04-01 (nachmittags)

## Thema: Cross-Platform Determinismus + Inference-Optimierung + Wavetable Brackets

### Ausgangsproblem
Gleiche Parameter + Seed → unterschiedlicher Sound auf Mac (CPU) vs PC (RTX 6000).

### Ursachenanalyse
3 Divergenzquellen identifiziert:
1. **PRNG**: `torch.Generator("cpu")` vs `torch.Generator("cuda")` = verschiedene Algorithmen
2. **BrownianTree**: `torchsde/_brownian/brownian_interval.py:31` nutzt `torch.Generator(device)` intern
3. **Precision**: float16 (PC) vs float32 (Mac)

Das Original-Backend (`backend/services/stable_audio_backend.py` = Kopie aus github.com/joeriben/ucdcae-ai-lab) nutzt `pipe.to("cuda")` + `torch.Generator("cuda")`, pipe_inference.py nutzt CPU.

### Was gemacht wurde

#### 1. Deterministische Noise via numpy PCG64 (4d79ace)
- Monkey-Patch `torchsde._brownian.brownian_interval._randn` → numpy PCG64
- numpy PCG64 ist plattformübergreifend identisch (ARM, x86, beliebiges OS)
- Beide Dateien gepatcht: `pipe_inference.py` + `stable_audio_backend.py`
- Generator in stable_audio_backend auf CPU umgestellt
- Steps-Default vereinheitlicht: 100 → 20 (ausreichend für DPM-Solver++ SDE bei kurzen Samples)

#### 2. MPS Auto-Detect (02c6557)
- `pipe_inference.py` erkennt automatisch MPS > CUDA > CPU
- Apple Silicon: ~15s statt ~50s für 3s Audio (3.3x schneller)
- MPS-Output weicht von CPU/CUDA ab (float32-Arithmetik divergiert über 20 DiT-Steps)
- Akzeptiert: MPS-Presets und CPU/CUDA-Presets sind separate Kategorien
- Validiert: MPS Audio-Qualität ist gut (RMS 0.53, Range [-1.1, 1.1])

#### 3. Latent Caching + Interpolation (addfcbb)
Neues Pipe-Protokoll:
- `"cache_as": "name"` bei Generate → speichert pre-VAE Latent
- `"mode": "interpolate"` → LERP zwischen gecachten Latents + VAE-Decode (~2s statt 15-50s)
- `"mode": "decode_cached"` → Re-Decode eines gecachten Latents
- Basis für zukünftiges "Build Wavetable from Axis"

#### 4. Attention Slicing (4962e02)
- `pipe.enable_attention_slicing()` auf MPS/CPU
- ~20% weniger Peak-Memory, leicht schneller

#### 5. Wavetable Brackets (dde48de)
- `extractFramesFromBuffer()` akzeptiert jetzt `startFrac/endFrac`
- Im Wavetable-Mode steuern die Bracket-Handles die Extraction Region
- `reextractWavetable()` wird bei Bracket-Drag aufgerufen → sofortige Frame-Neuerstellung
- Label wechselt: "Loop interval" (Sampler) ↔ "Extraction region" (Wavetable)

#### 6. Axes Panel Vereinfachung (979cc1d)
Basierend auf Spektralanalyse (PC, RTX 6000): Mel-Cosine-Distanz bei 1s vs 10s.
- **PCA-Achsen kollabieren** bei 1-3s (pc1/pc2/pc10: Mel < 0.19)
- **Semantische Achsen sind effektiver** bei kurzer Dauer (Top 6: Mel > 0.80)
- UI: 3 Slots (statt 3 Semantic + 6 PCA), Dropdown aus 8 validierten Achsen:
  music/noise, acoustic/electronic, improvised/composed, refined/raw,
  solo/ensemble, sacred/secular, tonal/noisy, rhythmic/sustained

## MPS-Recherche (Background Agent)

- Ollama v0.19.0 "2x Speedup": MLX-basiert, nur LLM, irrelevant für diffusers
- torch.compile auf MPS: Prototyp, crasht auf DiT — nicht nutzbar
- float16 auf MPS: SDPA-Bug, Audio-Clipping — float32 bleibt korrekt
- MLX: Kein Audio-Diffusion-Support, kein DiT, Ground-Up-Rewrite nötig
- Core ML/ANE: Kein Audio-Modell-Support
- Einzige nutzbare Optimierung: attention_slicing (eingebaut)

## Nächste Schritte

1. **RAVE Minimoog-Modell**: Vorheriger Trainingsversuch gescheitert (falsche Hyperparameter). Prompts für PC formuliert: Training-Config finden + korrigierte Config erstellen.
2. **"Build Wavetable from Axis"**: Latent-Interpolation ist Python-seitig fertig, JUCE-Anbindung fehlt. Zurückgestellt bis Achsen-Effektivität geklärt (jetzt erledigt).
3. **Sampler testen**: Voice-Chain ist seit Session 11 reaktiviert — Sampler- und Wavetable-Mode müssen getestet werden.

## Geänderte Dateien

### Modifiziert
- `backend/pipe_inference.py` — numpy noise, MPS, latent cache, attention slicing
- `backend/services/stable_audio_backend.py` — numpy noise, CPU generator, steps=20
- `src/dsp/WavetableOscillator.h/cpp` — startFrac/endFrac Parameter
- `src/PluginProcessor.h/cpp` — reextractWavetable(), bracket→extraction region
- `src/gui/SynthPanel.cpp` — bracket callback für Wavetable re-extract
- `src/gui/AxesPanel.h/cpp` — 3 Slots × 8 effektive Achsen (kein PCA)

## Wichtige Erkenntnisse

1. **BrownianTree nutzt numpy PCG64 intern** für Seed-Ableitung (plattformunabhängig), aber `torch.Generator(device)` für die Noise-Generierung (device-abhängig). Der Monkey-Patch von `_randn` löst das.

2. **MPS divergiert fundamental** von CPU/CUDA — nicht nur Noise, sondern DiT-Forward-Pass-Arithmetik. Korrelation CPU↔MPS: -0.54. Nicht fixbar, aber akzeptabel (Audio-Qualität gleichwertig).

3. **PCA-Achsen sind unbrauchbar bei 1-3s**. Die 392K-Prompt-PCA fängt Langzeit-Varianz ein, die in kurzen Samples nicht zum Tragen kommt. Semantische Text-Achsen funktionieren besser weil das Modell gezielte Pole auch in 1s ausdrücken kann.

4. **>3 gleichzeitige Achsen** bringen bei kurzen Samples keinen Mehrwert.

## Commits (Session 12)
- `4d79ace` — deterministic noise (numpy PCG64)
- `02c6557` — MPS auto-detect
- `addfcbb` — latent caching + interpolation
- `4962e02` — attention slicing
- `dde48de` — wavetable bracket extraction
- `979cc1d` — axes panel simplification
