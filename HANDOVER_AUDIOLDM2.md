---
name: audioldm2-handover
description: AudioLDM2 integration status — generation works standalone but fails through pipe_inference.py with tensor shape mismatch
type: project
---

## Status (2026-04-07)

AudioLDM2 ist als Engine in T5ynth teilweise implementiert. **Python-Standalone-Test funktioniert, aber durch die App aufgerufen scheitert die Generation.**

## Was funktioniert

- **Modell-Erkennung**: `_model_format()` liest `model_index.json`, erkennt `AudioLDM2Pipeline` → Format `"audioldm2"`
- **Download**: SetupWizard lädt AudioLDM2 von HuggingFace (tokenfrei, public repo)
- **Lizenz-Dialog**: Bestätigungsdialog vor jedem Modell-Download (SA: Community License, AudioLDM2: CC-BY-NC-SA)
- **GPT2-Fix**: `model_index.json` wird nach Download automatisch gepatcht (GPT2Model → GPT2LMHeadModel, nötig für transformers ≥4.45)
- **UI-Slot**: PromptPanel Slot 2 erkennt AudioLDM2 per Pattern-Match, Defaults: steps=50, cfg=3.5
- **Standalone-Test bestanden**: `pipe(prompt="a bell ringing", ...)` auf CPU und MPS funktioniert einwandfrei, Output: (32000,) @ 16kHz
- **Resampling**: `_resample_audio()` konvertiert 16kHz→44.1kHz via scipy polyphase, mono→stereo

## Was NICHT funktioniert

**Fehler: `The size of tensor a (9) must match the size of tensor b (4) at non-singleton dimension 1`**

Dieser Fehler tritt NUR auf wenn AudioLDM2 durch pipe_inference.py aufgerufen wird — NICHT im Standalone-Test. Der Fehler kommt aus dem UNet-Forward-Pass und betrifft die Attention-Mask / Embedding-Dimensionen.

### Debugging-Erkenntnisse

1. AudioLDM2 hat ZWEI Embedding-Räume (anders als Stable Audio):
   - `prompt_embeds`: `[B, 4, 1024]` (projected CLAP+T5)
   - `generated_prompt_embeds`: `[B, 8, 768]` (GPT2 output)
2. Manuelle Embedding-Konstruktion (negative/positive) führt zu Shape-Mismatches
3. Auch der simplifizierte Text-Prompt-Pfad (`pipe(prompt=...)`) scheitert durch die App — Ursache unklar
4. Möglicherweise interferiert etwas im Ladefluss (preload, device-switching, oder Reihenfolge der Pipeline-Initialisierung)

### Versuchte Ansätze für Embedding-Manipulation

1. `encode_prompt(do_classifier_free_guidance=False)` + manuell negative Embeddings → Shape-Mismatch (9 vs 4)
2. `encode_prompt(do_classifier_free_guidance=True)` + positive Hälfte manipulieren + re-concat → gleicher Fehler
3. Reiner Text-Prompt ohne Embedding-Manipulation → funktioniert standalone, scheitert durch App

### Nächster Debug-Schritt

Den exakten Unterschied zwischen Standalone-Aufruf und App-Aufruf finden. Vermutlich:
- Prüfen ob `pipe_inference.py` stdout-Redirect (`sys.stdout = sys.stderr`) oder der `_patch_torchsde_for_determinism()` die AudioLDM2-Pipeline beeinflusst
- Prüfen ob die Preload-Sequenz (leere Audio-Antwort) den internen State der Pipeline korrumpiert
- Logging in `_generate_audioldm2` einbauen: Request-Inhalt, Pipeline-State, shapes vor dem `pipe()`-Call

## Geänderte Dateien

- `backend/pipe_inference.py`: AudioLDM2Wrapper, _load_audioldm2_pipeline, _generate_audioldm2, _resample_audio, Routing
- `src/gui/SetupWizard.cpp`: kKnownModels mit AudioLDM2 + Lizenz-Dialoge + GPT2-Patch + Token optional
- `src/gui/SetupWizard.h`: licenseAccepted_, selectedNeedsToken()
- `src/gui/PromptPanel.cpp`: AudioLDM2 model-specific defaults (steps=50, cfg=3.5)

## Wichtige Constraints

- **venv NICHT verändern** — der Standalone-Build (PyInstaller) ist fragil
- **SA Small darf nicht brechen** — AudioLDM2 ist experimentell, SA Small ist Produktion
- **PyInstaller-Bundle** (`pipe_inference.bak`): aktuell umbenannt, App nutzt Python-Script direkt. Bundle muss irgendwann neu gebaut werden.
- Keine neuen Dependencies nötig (diffusers, transformers, scipy reichen)

## Lizenz-Situation

- SA Open: Stability AI Community License (<$1M kommerziell frei)
- AudioLDM2: CC-BY-NC-SA 4.0 (strikt non-commercial, keine Schwelle)
- Bestätigungsdialog vor Download implementiert
- T5ynth provident keine Modelle, nur Software (GPLv3)
