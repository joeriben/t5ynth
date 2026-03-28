# T5ynth Development Log

## 2026-03-28 — Session 3: Audio Pipeline Debugging + Sequencer Fix

### Critical Bug Fixed: No Sound After Generation
- **Root cause**: Looper audio was multiplied by amp envelope, which defaults to 0 without MIDI note-on. Generated audio x 0 = silence.
- **Fix**: Auto-trigger envelopes when `loadGeneratedAudio()` is called.
- **Secondary issue**: SVT filter's `setType()` was called every processBlock, found to not reset state (JUCE source confirms it only sets a variable). Actual filter silence was caused by DryWetMixer behavior — bypassed for fallback sine.
- **Base64 decode**: Original code used `MemoryBlock::fromBase64Encoding()` which is JUCE-proprietary format, NOT RFC 4648. Python's `base64.b64encode()` produces standard base64. Switched to `juce::Base64::convertFromBase64()` which is RFC 4648 compliant.
- **HTTP timeout**: Backend lazy-loads Stable Audio model on first request (10-15s). Client timeout was 5s. Increased to 120s.

### Sequencer MIDI Pipeline
- Implemented `StepSequencer::processBlock()` — sample-accurate 16th-note stepping with MIDI note generation.
- Implemented `Arpeggiator::processBlock()` — 5 modes (Up/Down/UpDown/Random/Order), multi-octave cycling.
- **Critical bug**: `start()` was called every `processBlock`, resetting `samplesUntilNextStep` to 0 each block — sequencer ran at block rate (~86 Hz) instead of BPM. Fixed: only reset on actual start transition.
- Seq and Arp now run **in series** (Seq generates notes, Arp arpeggates them + external MIDI), not mutually exclusive.

### Waveform Display
- `WaveformDisplay::setWaveform()` was never called. Wired via 30Hz timer in SynthPanel polling a snapshot buffer in PluginProcessor. Peak-preserving downsample to 1024 display points.

### GUI Fixes
- Fixed aspect ratio (3:2) — window scales proportionally.
- All ENV/LFO/Filter/Drift sections always visible (dimmed at 30% alpha when inactive) — no collapsing, predictable layout height.
- Font sizes derived from panel's own available height / total content units — everything fits at any size.
- Minimum window: 1050x700.
- Settings page as overlay (auto-scans model at known paths, shows backend status).
- Settings button injected into JUCE standalone header next to Options.
- Master volume as rotary knob.
- MIDI monitor in sequencer panel.
- Seed field as numeric text input with Random toggle.
- Fallback sine oscillator for MIDI testing without generated audio.

### Backend Connection
- `BackendManager` status ("Running") and `BackendConnection::isConnected()` were independent — synced by calling `checkHealth()` when manager reports Running.

---

## 2026-03-26/27 — Sessions 1-2: Foundation + GUI Design Pass

### Project Scaffold
- JUCE standalone + VST3 plugin with full DSP chain (wavetable oscillator, audio looper, 3 ADSR envelopes, 2 LFOs, drift LFOs, SVT filter, delay, convolution reverb, limiter).
- Python backend (Flask) wrapping Stable Audio Open 1.0 via diffusers `StableAudioPipeline`.
- `BackendManager` launches Python server as child process, polls `/health`.
- `BackendConnection` HTTP client on dedicated thread, delivers results via `MessageManager::callAsync`.

### GUI
- 2-column layout: Col1 (25%) Generation (prompts, embedding controls, axes) | Col2 (75%) Engine/Filter/Modulation/Drift.
- Footer: Sequencer + FX (Delay/Reverb) + Master Volume.
- Section colors: Filter=cyan, Modulation=orange, Drift=purple.
- Linear sliders with value display and units (no rotary knobs for parameters).
- Color-coded semantic axes (pink/blue/green) + PCA axes (6 colors).

### DSP
- Full modulation routing in processBlock: envelopes/LFOs → DCA/Filter/Scan targets.
- Filter dry/wet mix via DryWetMixer.
- Envelope looping (re-enters Attack from Sustain).
- Drift LFO re-generation (randomize rate/depth on phase wrap).

### Model Loading
- Local model path support: `~/t5ynth/models/stable-audio-open-1.0/` (symlink to HuggingFace cache or ComfyUI directory).
- Fallback: HuggingFace auto-download.
- Requires HuggingFace pipeline format (`model_index.json`) — single `.safetensors` not usable because embedding manipulation needs separate access to text encoder + projection model.

---

## Architecture Notes

### Signal Flow
```
MIDI In → Sequencer → Arpeggiator → Note Processing
                                         ↓
              Wavetable Oscillator / Audio Looper / Fallback Sine
                                         ↓
                        Filter (SVT, modulated cutoff)
                                         ↓
                         Delay → Reverb → Master Vol → Limiter → Out
```

### Audio Generation Flow
```
User clicks Generate → PromptPanel builds GenerationRequest
  → BackendConnection POST /api/cross_aesthetic/synth (JSON)
  → Python backend: T5 encode → embedding manipulation → StableAudioPipeline
  → Response: base64-encoded WAV
  → JUCE: Base64 decode → WAV parse → AudioBuffer
  → loadGeneratedAudio(): Looper + Wavetable extract + Waveform snapshot
  → Auto-trigger envelopes → Sound plays immediately
```
