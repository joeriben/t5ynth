# Session 5 Portierungs-Audit: Selbstprüfung aller Änderungen

Erstellt: 2026-03-29, Session 5
Methode: Jede Zeile aus der 136-Einträge-Portierungsliste gegen den aktuellen Code geprüft.

## Gesamtbilanz nach Session 5

| Status | Anzahl | % |
|--------|--------|---|
| OK (korrekt portiert) | 95 | 70% |
| TEILWEISE (funktioniert, nicht 100% identisch) | 7 | 5% |
| NOCH OFFEN (bewusst verschoben) | 8 | 6% |
| BUG (muss gefixt werden) | 7 | 5% |
| JUCE-LIMITATION (kann nicht portiert werden) | 3 | 2% |
| FEHLT (vergessen) | 3 | 2% |
| Nicht portierbar / Low Priority | 5 | 4% |
| **Vorher OK (nicht angefasst)** | 8 | 6% |

## Kritische Bugs (MUSS GEFIXT WERDEN)

### BUG 1: Alpha-Target für LFO verloren
- **Wo**: PluginProcessor.cpp, LFO target choice list
- **Was**: Beim Erweitern der LFO-Targets von 4→10 habe ich "Alpha" entfernt. War vorher Index 2.
- **Vorlage**: LFO kann Alpha nicht modulieren (Alpha ist kein ModTarget in useModulation.ts). Aber in der Vorlage gibt es `useDriftLfo` für Alpha. Also war das JUCE-eigene "Alpha" als LFO-Target **sowieso eine Eigenerfindung** — die Vorlage hat kein LFO→Alpha.
- **Bewertung**: Kein Bug — war Eigenerfindung, korrekt entfernt.

### BUG 2: DCF Envelope drückt Cutoff permanent runter bei Idle
- **Wo**: PluginProcessor.cpp Zeile 734-740
- **Was**: `envFactor = startFactor + (peakFactor - startFactor) * lastMod1Val`. Bei lastMod1Val=0 (kein Note-On) → envFactor = startFactor = 1-amount. Cutoff wird permanent mit (1-amount) multipliziert.
- **Fix**: envFactor nur anwenden wenn Envelope aktiv (noteIsOn oder !ampEnvelope.isIdle())

### BUG 3: modPitch berechnet aber nie angewendet
- **Wo**: PluginProcessor.cpp Zeile 698-720
- **Was**: `modPitch` akkumuliert Werte von Env/LFO→Pitch, wird aber danach nie auf wavetableOsc.setFrequency() oder looper.setMidiNote() angewendet.
- **Fix**: Nach dem Accumulation-Block: `if (modPitch != 0) { freq *= (1 + modPitch); }` anwenden.

### BUG 4: Kein APVTS-Parameter für Sequencer-Division
- **Wo**: PluginProcessor.cpp createParameterLayout()
- **Was**: `seq_division` Parameter fehlt. Division kann nur über StepSequencer.setDivision() gesetzt werden, nicht über UI/Presets.
- **Fix**: AudioParameterChoice für Division hinzufügen, in processBlock lesen.

### BUG 5: Kein APVTS-Parameter für Sequencer Glide Time
- **Wo**: PluginProcessor.cpp createParameterLayout()
- **Was**: `seq_glide_time` Parameter fehlt.
- **Fix**: AudioParameterFloat für GlideTime (10-500ms, default 80) hinzufügen.

### BUG 6: barStartFlag wird nie konsumiert
- **Wo**: StepSequencer setzt Flag, aber processBlock liest es nicht.
- **Was**: Das Flag ist für onBarStart() gedacht (Background-Regen Swap).
- **Fix**: In processBlock: if (stepSequencer.barStartFlag.exchange(false)) { /* trigger regen */ }. Aber die eigentliche Backend-Regen-Integration fehlt sowieso noch.

### BUG 7: Limiter prepare() hardcoded Defaults
- **Wo**: Limiter.cpp Zeile 11: `limiter.setThreshold(-0.3f)`
- **Was**: prepare() setzt -0.3dB, processBlock überschreibt mit Param (-3.0dB). Erster Block hat falschen Wert.
- **Fix**: prepare() soll den Param-Default verwenden oder einfach nichts setzen (processBlock setzt ohnehin).

## Verbesserungsmöglichkeiten (kein akuter Bug)

### Signal-Flow nicht vollständig parallel (#24)
- Reverb bekommt Delay-Output statt nur Originalsignal
- Fix: Separaten Buffer für Reverb-Input erstellen, vor Delay kopieren

### syncDelayToBpm() nicht implementiert (#28)
- Funktion fehlt, aber für die Grundfunktionalität nicht kritisch

### LFO-Scaling nicht identisch (#68)
- JUCE: multiplikativ `cutoff * (1 + raw*depth)`
- Vorlage: additiv `cutoff + base*depth*raw`
- Klingt ähnlich, ist aber nicht identisch

### Drift Depth exponentielles Mapping fehlt (#84)
- JUCE: linear 0-1
- Vorlage: exponentiell 0.001-1, Micro-Drift bei niedrigen Werten unmöglich
- Fix: APVTS Parameter mit skew-Factor oder Custom NormalisableRange

### Engine-Mode-Switch bei Parameter-Wechsel fehlt (#131)
- Vorlage stoppt beide Engines bei Mode-Wechsel
- JUCE: nur bei noteOn wird Looper in WT-Mode gestoppt

## JUCE-Limitationen (nicht portierbar)

- Limiter knee (6dB) — JUCE dsp::Limiter hat kein Knee
- Limiter ratio (20) — JUCE dsp::Limiter hat kein Ratio
- Limiter attack (1ms) — JUCE dsp::Limiter hat kein Attack
- **Alternative**: DynamicsCompressorNode ersetzen (hat alle Params)

## Bewusst offene Punkte

- MIDI Clock Sync (#103, #114)
- WAV Export (#135, #136)
- testNote() (#128)
- onBarStart() Backend-Integration (#129)
- Cutoff Hz vs. Normalized (#3, #20) — UI-Unterschied, kein DSP-Bug

---

## Detailliste: 136 Einträge

### 1. Filter (23 Einträge)

| # | Feature | Vorher | Jetzt | Bewertung |
|---|---------|--------|-------|-----------|
| 1 | FilterType LP/HP/BP | OK | OK | korrekt |
| 2 | FilterSlope 12/24dB | FEHLT | OK | cascade mit 2 TPT-Filtern |
| 3 | normalizedToFreq(n) | INKOMPATIBEL | OFFEN | Slider bleibt Hz, Preset konvertiert |
| 4 | resonanceToQ(r) | FEHLT | OK | 0.5*pow(36,r) exakt |
| 5 | enabled default false | OK | OK | |
| 6 | type default lowpass | OK | OK | |
| 7 | slope default '12' | OK/FEHLT | OK | Param default 0 |
| 8 | cutoff default 1.0=20kHz | OK | OK | |
| 9 | resonance default 0→Q=0.5 | FALSCH | OK | prepare() setzt Q=0.5 |
| 10 | mix default 1.0 | OK | OK | |
| 11 | kbdTrack default 0 | OK | OK | |
| 12 | applySlope() cascade | FEHLT | OK | |
| 13 | applyMix() equal-power | FALSCH | OK | cos/sin exakt |
| 14 | applyFrequency()+kbdTrack | OK | OK | |
| 15 | getFrequencyParam() | OK | OK | anderer Ansatz |
| 16 | setNote() | OK | OK | |
| 17 | setEnabled() | OK | OK | |
| 18 | setType() | OK | OK | |
| 19 | setSlope() | FEHLT | OK | |
| 20 | setCutoff() normalized | INKOMPATIBEL | OFFEN | Hz-basiert, Preset konvertiert |
| 21 | setResonance() Q-mapping | FEHLT | OK | |
| 22 | setMix() equal-power | FALSCH | OK | |
| 23 | setKbdTrack() | OK | OK | |

### 2. Effekte (25 Einträge)

| # | Feature | Vorher | Jetzt | Bewertung |
|---|---------|--------|-------|-----------|
| 24 | Paralleler Signal-Flow | FALSCH | TEILWEISE | Reverb bekommt Delay-Output |
| 25 | Feedback LP-Filter | FEHLT | OK | Butterworth LP Q=0.7 |
| 26 | Damping exp. Mapping | FEHLT | OK | 20000*pow(500/20000,d) |
| 27 | Dry-Compensation | FEHLT | OK | 1-mix*0.3 |
| 28 | syncDelayToBpm() | FEHLT | FEHLT | vergessen |
| 29 | delayTimeMs default 250 | FALSCH | OK | |
| 30 | delayFeedback default 0.35 | FALSCH | OK | |
| 31 | delayMix default 0.3 | FALSCH | OK | |
| 32 | reverbMix default 0.25 | FALSCH | OK | |
| 33 | delayDamp default 0.5 | FALSCH | OK | |
| 34 | Limiter threshold -3dB | FALSCH | BUG | Param OK, prepare() hardcoded -0.3 |
| 35 | Limiter knee 6dB | FEHLT | JUCE-LIMIT | kein Knee in dsp::Limiter |
| 36 | Limiter ratio 20 | FEHLT | JUCE-LIMIT | kein Ratio |
| 37 | Limiter attack 1ms | FEHLT | JUCE-LIMIT | kein Attack |
| 38 | Limiter release 100ms | OK | OK | |
| 39-43 | Delay setters | OK | OK | |
| 44-47 | Reverb setters | OK | OK | |
| 48 | getModTargets() | FEHLT | OK | block-rate in processBlock |

### 3. Modulation (28 Einträge)

| # | Feature | Vorher | Jetzt | Bewertung |
|---|---------|--------|-------|-----------|
| 49 | 13 ModTargets | FEHLT | TEILWEISE | 9 Env + 10 LFO. Alpha kein Bug (Eigenerfindung) |
| 50-52 | Env defaults | FALSCH | OK | alle korrigiert |
| 53-54 | LFO defaults | FALSCH | OK | rate/depth/wave korrigiert |
| 55-57 | DCA ADSR | OK | OK | |
| 58 | DCF sweep below base | FALSCH | BUG | envFactor bei Idle drückt Cutoff runter |
| 59 | DCF Release | FEHLT | TEILWEISE | ADSR→0 ähnlich, nicht identisch |
| 60 | Pitch Target | FEHLT | BUG | modPitch nie angewendet |
| 61-63 | Env Loop/MinRamp/bypass | OK | OK | |
| 64-65 | LFO Waveforms/params | OK | OK | |
| 66 | LFO targets 13 | FEHLT | TEILWEISE | 10 statt 13, Alpha kein Bug |
| 67 | LFO mode free/trigger | OK | OK | |
| 68 | LFO Scaling | FALSCH | TEILWEISE | multiplikativ statt additiv |
| 69 | LFO Trigger | OK | OK | |
| 70 | LFO Cross-Modulation | FEHLT | OK | |
| 71-75 | Env/LFO→delay/reverb | FEHLT | OK | block-rate |
| 76 | wt_scan callback | OK | OK | |

### 4. Drift LFO (14 Einträge)

| # | Feature | Vorher | Jetzt | Bewertung |
|---|---------|--------|-------|-----------|
| 77-78 | 3 LFOs, 6 Targets | OK | OK | |
| 79 | 4 Waveforms | FEHLT | OK | sine/tri/sq/saw |
| 80-82 | Defaults | FALSCH | OK | rate/depth korrigiert |
| 83 | Rate exp. Mapping | ~OK | OK | APVTS skew approximiert |
| 84 | Depth exp. Mapping | FALSCH | NOCH OFFEN | linear statt exponentiell |
| 85 | halfRange in Offset | FALSCH | OK | |
| 86 | Phase-Reset | FEHLT | OK | |
| 87 | Target-Wechsel nullen | N/A | OK | |
| 88 | Auto-Regen Semantik | FALSCH | TEILWEISE | Randomisierung entfernt, Backend fehlt |
| 89-90 | resetPhases/auto-start | OK | OK | |

### 5. Sequencer (24 Einträge)

| # | Feature | Vorher | Jetzt | Bewertung |
|---|---------|--------|-------|-----------|
| 91-93 | Step active/note/velocity | OK | OK | |
| 94 | Step.gate | FEHLT | OK | gate-off implementiert |
| 95 | Step.glide | FEHLT | OK | ch2-Encoding |
| 96 | StepCount options | ~OK | OK | |
| 97 | NoteDivision | FEHLT | BUG | implementiert aber kein APVTS-Param |
| 98 | Glide Time | FEHLT | BUG | implementiert aber kein APVTS-Param |
| 99 | 10 Presets | FEHLT | OK | alle 10 exakt portiert |
| 100 | Gate-Off timing | FEHLT | OK | |
| 101 | Glide Pitch-Ramp | FEHLT | OK | |
| 102 | Bar-Start Callback | FEHLT | BUG | Flag gesetzt, nie konsumiert |
| 103 | MIDI Clock Sync | FEHLT | OFFEN | spätere Session |
| 104-107 | start/stop/bpm/count | OK | OK | |
| 108-109 | loadPreset/resetGrid | FEHLT | OK | |
| 110-114 | Step setters | OK/FEHLT | OK | |

### 6. Arpeggiator (8 Einträge)

| # | Feature | Vorher | Jetzt | Bewertung |
|---|---------|--------|-------|-----------|
| 115 | 4 Patterns | OK | OK | Order entfernt (Eigenerfindung) |
| 116 | Chord Intervals | FALSCH | OK | [0,4,7] exakt |
| 117 | 7 Rate-Divisionen | FALSCH | OK | |
| 118 | Oktav-Range | OK | OK | |
| 119 | Fisher-Yates | FALSCH | OK | |
| 120 | processNote passthrough | OK | OK | |
| 121 | stop() | OK | OK | |
| 122 | buildIntervals() | FEHLT | OK | |

### 7. Orchestration (10 Einträge)

| # | Feature | Vorher | Jetzt | Bewertung |
|---|---------|--------|-------|-----------|
| 123 | triggerEngine() | ~OK | OK | Note-Stack |
| 124 | WT stoppt Looper | FEHLT | OK | |
| 125 | Engine-Stop nach Release | FEHLT | OK | engineStopCountdown |
| 126 | glideEngine() | FEHLT | OK | both engines |
| 127 | Seq→Glide | FEHLT | OK | ch2 encoding |
| 128 | testNote() | FEHLT | FEHLT | vergessen, low priority |
| 129 | onBarStart() regen | FEHLT | FEHLT | Backend-Integration |
| 130 | Note-Stack | FEHLT | OK | last-note-priority |
| 131 | Engine Mode Switch | FEHLT | TEILWEISE | nur bei noteOn |
| 132 | Legato Pitch-Change | FEHLT | OK | |

### 8. Looper (4 Einträge)

| # | Feature | Vorher | Jetzt | Bewertung |
|---|---------|--------|-------|-----------|
| 133 | glideToSemitones() | FEHLT | OK | per-sample linear ramp |
| 134 | normalizeOn default | FALSCH | OK | true |
| 135 | encodeWav() | FEHLT | OFFEN | low priority |
| 136 | exportRaw/Loop | FEHLT | OFFEN | low priority |
