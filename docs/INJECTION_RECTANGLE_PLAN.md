# Injection Kombi Preset Modes — Architecture Plan

Status: **implemented in v1.6.0-beta.1 (2026-04-30)**. Three Kombi modes
shipped — `kombi1`, `kombi2`, `kombi3` — together with a clarified Fine
slider scale (0–1) and per-mode slider memory. The earlier version of
this file proposed a 2D rectangle UI with Auto-Alpha; that approach
remains deferred (see §6) and may revisit if the Kombi probes reveal
musically richer territory than three discrete points can capture.

## 0. Empirical findings (post-implementation)

Listening sessions on 2026-04-30 with the SA Open Small model produced
the following observations and led to the shipped parameter tuples:

- **Sigmoid soft-mask was too gentle for narrow bands.** With
  `layer_split_smoothness=2`, a 4-block-wide range peaked at w≈0.534 in
  the centre and tapered to w≈0.441 at the edges. Even at slider=1.0
  (`late_blend = pure B`) the per-block contribution was bounded around
  50 %. Replaced with a **hard mask** (w=1 inside `[start, end)`, 0
  outside) so slider=1 reads as "100 % `late_blend` in the band's
  blocks", which matches the user's mental model of a preset's full
  strength.
- **High DiT blocks have low conditioning leverage.** The original
  Kombi 2 = [12, 16] was effectively inaudible — the four top blocks
  in SA Open Small carry mostly refinement work and reacting to the
  cross-attention override there barely changes the output. Kombi 2 was
  moved to **[4, 12] (broad mid)**, where the DiT does most of its
  conditioning work. The shift from [12, 16] to [4, 12] turned Kombi 2
  from "sounds like A" into the most distinct mode of the three.
- **Kombi 1 [0, 4] is structurally subtle.** Four narrow surface blocks
  out of sixteen, with the other twelve seeing pure A through every
  step, produces a fairly A-dominant result by design — "B as surface
  skin", as the original §2.1 framing put it. Listeners who expect a
  strong audible swing from clicking Kombi 1 should be aware that this
  is the design intent; if it proves too subtle in practice, candidate
  widenings are [0, 6] or [2, 6].
- **Kombi 3 = narrow center [6, 10].** The four highest-leverage blocks,
  same width as Kombi 1 but at a different vertical position. Probes
  whether *position-within-mid* matters as much as *band-width*.
  Hypothesis (post-listening, 2026-04-30): K3 may turn out to be the
  most aggressive of the three despite the narrowest band, because all
  four blocks sit in the dense-conditioning region.

## 0.1 Final shipped tuples

| Mode    | Layer band | Blocks | Mask | Slider mapping                            |
|---------|------------|--------|------|-------------------------------------------|
| Kombi 1 | [0, 4)     | 0..3   | hard | Fine-style 0–1, transition × late α       |
| Kombi 2 | [4, 12)    | 4..11  | hard | Fine-style 0–1, transition × late α       |
| Kombi 3 | [6, 10)    | 6..9   | hard | Fine-style 0–1, transition × late α       |

Each Kombi shares the Fine slider's coupled mapping
`transition_at = 0.5 − 0.45·t`, `late_α = t` with `t ∈ [0, 1]`.

## 1. Background — current state of the injection UI

The prompt-injection feature is currently research-mode local code in
[`PromptPanel`](../src/gui/PromptPanel.h:140), explicitly **not in APVTS**
(see the comment: "Temporary injection-mode test UI (research; not in APVTS)").

Three modes today, selected via radio-group buttons:

- `"linear"` — α-slider attached to APVTS, range −1..+1 (A↔B blend).
- `"late_step"` (Fine) — slider takes a hardcoded sweet-spot range
  0.5..1.0 and drives BOTH `injectionTransitionAt` AND `latePhaseAlpha`
  coupled. Mapping at
  [PromptPanel.cpp:900-904](../src/gui/PromptPanel.cpp:900):
  `transition_at = 0.5 − 0.45·t`, `late_phase_α = t`,
  with `t = (slider − 0.5)/0.5`.
- `"layer_split"` (Layer) — TwoValueHorizontal slider 0..16 sets the
  DiT block range `[b_start, b_end]`. The α component itself is read
  unmodified from APVTS (the last Linear-mode value), without a
  mode-specific heuristic.

## 2. Decision: Kombi 1 / Kombi 2 / Kombi 3 as additional preset modes

The path forward was the 80s Yamaha-organ pattern: add preset buttons
that load hardcoded musically-coherent combinations of step *and*
layer parameters, exposing only one slider (intensity) per mode.

- Six modes total (shipped): `Linear / Fine / Layer / Kombi 1 / Kombi 2 / Kombi 3`.
- Each Kombi internally fixes a step-axis sweet-spot AND a layer-axis
  range; the user controls only the intensity (Fine-style coupled
  slider).
- Each mode keeps its own slider position so A/B-ing by clicking
  buttons does not destroy state.
- Mode buttons trigger immediate regeneration.
- Drift continues to function unchanged on the active mode's
  intensity axis.

Labels stay neutral ("Kombi 1/2/3") to avoid pre-judging musical
character — descriptive renames are deferred until each Kombi proves
itself empirically.

### 2.1 Initial parameter tuples (theoretical first guesses, **superseded** — see §0.1)

The original plan called for two modes, both in the late-step half of
the conceptual rectangle:

**Kombi 1** — "B as surface skin" (late × low-layer):
- `b_start = 0`, `b_end = 4` (lowest 4 DiT blocks) — kept

**Kombi 2** — "B as gestalt filter" (late × high-layer):
- `b_start = 12`, `b_end = 16` (top 4 DiT blocks) — **dropped**, see §0
- empirical replacement: [4, 12] (broad mid)

A third mode (Kombi 3 = [6, 10] narrow center) was added during
implementation to occupy the unused button real estate and probe the
"narrow band at different vertical position" axis.

## 3. Architecture

### 3.1 Backend (Python)

In the Stable Audio Backend's injection-mode handler, add two new
mode strings: `"kombi1"`, `"kombi2"`. Each applies BOTH the
late-step parametrization AND the layer-split range simultaneously,
with the layer range hardcoded per mode and the late-step values
taken from the request.

This is a small extension of existing hooks — no new fields in the
IPC `Request` struct (the four relevant fields already exist:
`injectionTransitionAt`, `latePhaseAlpha`, `splitStart`, `splitEnd`).

### 3.2 UI (C++)

Two additional buttons in the existing radio group at
[`PromptPanel.cpp:148-154`](../src/gui/PromptPanel.cpp:148):

```cpp
juce::TextButton injModeKombi1 { "Kombi 1" };
juce::TextButton injModeKombi2 { "Kombi 2" };
// ...same setRadioGroupId(2027), styleModeBtn(...) treatment as the
//    existing three buttons.
```

The `applyModeToSlider()` function at
[`PromptPanel.cpp:800`](../src/gui/PromptPanel.cpp:800) gets two
additional branches that mirror the `late_step` branch (same range
0.5..1.0, same label format, same `lateMixAmount_` storage).

The `buildInferenceRequest()` function at
[`PromptPanel.cpp:853`](../src/gui/PromptPanel.cpp:853) sets the
appropriate `splitStart`/`splitEnd` overrides when the active mode
is one of the Kombis. The actual injection mode string sent to the
backend is `"kombi1"` or `"kombi2"`; the backend's handler reads
all four parameters and applies them together.

### 3.3 Drift integration

No changes. The existing drift architecture targets Alpha and the
mode-aware override logic in `buildInferenceRequest()` already works:
when drift modulates Alpha, the modulated value is passed via
`lateMixOverride` in Kombi modes, just as it does in Fine mode.

## 4. Empirical workflow

1. Implement backend additions with the theoretical default tuples.
2. Implement the two UI buttons and their `applyModeToSlider()` branches.
3. Build (`cmake --build build_clean --config Release`).
4. Listening session: A/B each Kombi against pure Fine and pure Layer
   with identical prompts and seeds.
5. Adjust the hardcoded parameter tuples per Kombi until each has a
   distinct musical character that justifies its existence.
6. Decide post-hoc on naming (keep "Kombi 1"/"Kombi 2" or rename
   descriptively).

If a Kombi cannot be made musically distinctive after a few iterations,
it is removed rather than shipped as decoration.

## 5. What stays unchanged

- Linear / Fine / Layer modes keep their current behavior and UI.
- The modulation system (envelopes, LFOs, drift LFOs, target
  dropdowns) is structurally untouched.
- APVTS layout gets no new parameters; the mode string remains
  research-mode local state in `PromptPanel`.
- Preset format gets two new valid `injectionMode` string values via
  the existing serialization path.
- IPC protocol: same four fields, two new valid mode-string values.

## 6. Deferred ideas (not in this plan)

The following were considered and explicitly excluded for now:

- **Rectangle UI**: a 2D canvas with draggable B-zone unifying step
  and layer axes into one geometry. Deferred because empirical
  validation of Step+Layer combinations has to happen in-synth (the
  test tool is not viable), and the Kombi approach probes the same
  musical question with a fraction of the refactor cost.
- **Auto-Alpha heuristic**: not needed without the rectangle, since
  each Kombi has its own hardcoded internals.
- **APVTS promotion of step/layer ranges**: not needed; ranges are
  per-mode hardcoded.
- **Drift target list reorder + version bump**: not needed; no new
  drift targets are introduced.
- **Early-step quadrants** of the conceptual rectangle: theoretically
  Matsch-affin (mid-trajectory hard-injection-disruption), so the
  two Kombis stay in the late-step half.

The rectangle path remains a possible future direction if the Kombi
listening sessions reveal that the late-step half of the (step ×
layer) plane is musically richer than two discrete points can
capture.

## 7. Related: Fine → "Kick-in" rename

The original ticket from the session handover (Fine → "Kick-in" or
similar label change) is independent of this plan and can land as a
separate commit. Pure UI label change, no behavioral effect.

## 8. Cross-references

- [`ARCHITECTURE.md`](ARCHITECTURE.md) §5 — modulation routing
  (unchanged by this plan).
- [`ADDING_A_MODULATION_TARGET.md`](ADDING_A_MODULATION_TARGET.md) —
  not invoked; no new targets.
- [`RESEARCH_IDEAS.md`](RESEARCH_IDEAS.md) — the matrix-UI sketch in
  "Possible UI Shape" remains superseded; the Kombi approach is the
  pragmatic descendant of the embedding-injection research line.
- [`IPC_PROTOCOL.md`](IPC_PROTOCOL.md) — two new valid `injectionMode`
  string values, no protocol-level changes.
