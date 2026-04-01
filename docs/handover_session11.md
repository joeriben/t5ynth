# Session 11 Handover — 2026-04-01 (vormittags)

## Was gemacht wurde

### 1. Voice Chain gefixt (6891c1b)
- AudioLooper → SamplePlayer umbenannt
- Per-voice Filter (war vorher post-sum, jetzt pro Voice im renderSample)
- Debug-Bypass entfernt — echter Voice-Chain ist wieder aktiv
- Sampler/Wavetable-Mode über APVTS "engine_mode"

### 2. UI Polish (67e5bac)
- Engine-Mode Buttons (Sampler/Wavetable) steuern Visibility von Loop-Controls vs Scan
- Ghost-Indicators für inaktive Envelope/LFO-Targets (dimmed statt hidden)
- Envelope/Sequencer Fixes

### 3. Modulation Section Merge (383ed7a)
- Drift-LFOs in die Modulation Section integriert
- Random Seed Overflow gefixt (war int32, jetzt korrekt begrenzt)

## Architektur nach Session 11

```
Voice Chain (pro Voice):
  WavetableOscillator (Wavetable-Mode) ──┐
  SamplePlayer (Sampler-Mode) ───────────┤
                                         ├→ VCA (ampEnv × modulation) → SVF Filter → output
Modulation: ampEnv, mod1Env, mod2Env, LFO1, LFO2, Drift1, Drift2
```

## Commits
- `6891c1b` — fix voice chain, per-voice filter, rename AudioLooper
- `67e5bac` — UI polish, engine-mode visibility, ghost indicators
- `383ed7a` — merge drift into modulation, fix seed overflow
