# Session 14 Handover — 2026-04-16

## Thema: Sampler `Norm`, ReGenerate, laufende Voices, Mehrfach-Anschläge

### Kurzfazit

Das Problem ist derzeit **nicht sauber gelöst**. Der aktuelle Stand deutet darauf hin, dass im **Sampler** bei `ReGenerate` laufende Stimmen auf eine Weise beeinflusst werden, die wie wiederholte Teil-Neuansätze innerhalb derselben gehaltenen Note klingt.

Die Stretch-Hypothese ist nach User-Befund derzeit **nicht der Hauptverdächtige**:

- der Effekt tritt nicht nur einmal auf
- sondern 2, 3, 4 mal innerhalb derselben gehaltenen Note
- das passt deutlich besser zu **wiederholten Source-/Buffer-Updates** als zu einem einmaligen Priming-Artefakt

### Vom User klar festgestellt

- Problem betrifft **Sampler**, nicht Wavetable
- `Norm` wirkt bei leisen Samples nicht hinreichend / nicht proportional
- `P1 = P2` war in einem wichtigen Testfall gesetzt
- keine offensichtlichen versteckten Peaks / kein sichtbarer Rumble
- `ReGenerate` ist zentral beteiligt
- bei kurzem `Seq Gate` fällt das Problem kaum auf
- bei langen gehaltenen Noten klingt es wie **mehrfacher Anschlag / Echo / Unterbrechung**

### Bereits umgesetzte Änderungen

#### 1. Pre-stretch Env/DCA-bezogene Sampler-Norm

Implementiert in:

- `src/dsp/SamplePlayer.h`
- `src/dsp/SamplePlayer.cpp`
- `src/dsp/SynthVoice.h`
- `src/dsp/SynthVoice.cpp`

Idee:

- Referenzmessung auf dem **untransponierten** Samplerpfad
- mit **P1/P2/P3**
- mit synthetischer **Env/DCA-Referenz**
- daraus konstanter `sourceGain_`
- Gain wirkt **vor** Stretch

Status:

- Build grün
- Funktionalität vorhanden
- aber nicht als finale Lösung validiert

#### 2. Snapshot-Fix für laufende Sampler-Voices bei `ReGenerate`

Implementiert in:

- `src/dsp/SamplePlayer.h`
- `src/dsp/SamplePlayer.cpp`
- `src/dsp/VoiceManager.h`
- `src/dsp/VoiceManager.cpp`
- `src/PluginProcessor.cpp`

Änderung:

- aktive Sampler-Voices werden vor `loadGeneratedAudio()` / `reloadProcessedAudio()` mit `freezeSharedBuffer()` vom Master-Buffer entkoppelt
- nur **neue** Notes sollen das neue Material bekommen
- inaktive Voices bekommen weiter `shareBufferFrom(master)`

Technik:

- `SamplePlayer::freezeSharedBuffer()` kopiert den aktuell geteilten `playBuffer` lokal in die Voice
- `VoiceManager::freezeActiveSamplerVoices()` friert aktive Sampler-Stimmen vor dem Reload ein
- `VoiceManager::distributeSamplerBuffer()` bindet aktive Sampler-Stimmen nicht mehr neu an den Master

Status:

- Build grün
- Problem laut User **noch vorhanden**

### Wichtige Codepfade

#### ReGenerate / Distribution

- `src/PluginProcessor.cpp`
  - `loadGeneratedAudio(...)`
  - `reloadProcessedAudio(...)`
  - `processBlock(...)`

#### Sampler-Voice / Buffer-Sharing

- `src/dsp/VoiceManager.cpp`
  - `noteOn(...)`
  - `freezeActiveSamplerVoices()`
  - `distributeSamplerBuffer(...)`

#### Sampler-Playback

- `src/dsp/SamplePlayer.cpp`
  - `shareBufferFrom(...)`
  - `freezeSharedBuffer()`
  - `retrigger()`
  - `preparePlaybackBuffer()`
  - `processSample()`
  - `readRawSamples(...)`
  - `renderPitchedBlock(...)`
  - `advancePosition(...)`

### Aktueller Hauptverdacht

Nicht ein einzelner Stretch-Startfehler, sondern **wiederholte Re-Bindung / Re-Prepare / Source-Änderung während laufender Sampler-Voices**.

Relevant ist besonders:

- in `PluginProcessor::processBlock(...)` läuft pro Block:
  - `if (masterSampler.needsReprepare()) masterSampler.preparePlaybackBuffer();`
  - danach immer `voiceManager.distributeSamplerBuffer(masterSampler);`

Auch wenn `distributeSamplerBuffer()` aktive Voices inzwischen überspringt, muss genau geprüft werden:

- ob laufende Voices an anderer Stelle doch noch indirekt neu gekoppelt werden
- ob `preparePlaybackBuffer()` oder Parameteränderungen Zustände verändern, die aktive Stimmen trotzdem hörbar beeinflussen
- ob der User-Verdacht zutrifft, dass ein **nahtloser Buffer-Wechsel bei weiterlaufendem Playhead** die robustere Architektur wäre

### Nächster sinnvoller Schritt

Nicht weiter auf Verdacht an `Norm` oder Stretch drehen.

Stattdessen:

1. **Instrumentieren**, wann und warum `masterSampler.needsReprepare()` während gehaltenen Noten gesetzt wird.
2. **Loggen**, ob aktive Sampler-Voices nach `ReGenerate` oder UI-/Modulationsänderungen erneut `shareBufferFrom(...)`, `retrigger()` oder sonstige Reset-/Rebind-Pfade sehen.
3. Falls bestätigt:
   **Source-Swap ohne Reset** bauen:
   - Playhead der laufenden Voice bleibt erhalten
   - Buffer-Quelle wird dynamisch ersetzt
   - kurzer interner Crossfade verhindert Knackser

### Nicht weiter verfolgen ohne neue Evidenz

- “versteckte Peaks”
- DC-/Rumble-Hypothesen
- Stretch als alleinige Hauptursache
- Auto-Gen-Mixer-XFade als alleinige Erklärung

### Build-Status

Erfolgreich gebaut mit:

```bash
cmake --build /Users/joerissen/ai/t5ynth/build_clean --target T5ynth -j4
```

### Hinweis zum Worktree

Der Worktree ist **dirty**. Es gibt neben den Session-14-Änderungen auch andere lokale Modifikationen. Vor einem Commit die relevanten Dateien selektiv prüfen.
