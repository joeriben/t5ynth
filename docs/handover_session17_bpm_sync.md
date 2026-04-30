# Session 17 Handover — BPM Sync for LFO / Drift / Delay (+ LFO mode UI rework)

**Status: design decided, not yet implemented.** This is a fresh
feature ticket for a cold session. Read it end-to-end before
touching code; the UX proposal interleaves with a code-level bug
fix and an enum re-shaping, and getting the order right matters.

The user has answered the open design questions — see §6
("Resolved decisions"). Implementation order is **feature + UI
first; the Free/Trig bugfix is deferred** to a later commit.

---

## 1. The feature in one sentence

Add BPM-sync to **LFO 1/2/3**, **Drift LFO 1/2/3**, and **Delay
Time** so their rates / times can be quantised to musical divisions
of a clock, with a small inline UI selector that sits to the **left
of the existing "Rate" slider** (or, for Delay, to the left of the
"Time" slider).

**Two states**, not three:

| State | Visual                       | Behaviour                                            |
| ----- | ---------------------------- | ---------------------------------------------------- |
| Off   | grey clock symbol, no fill   | Free-running rate / time (current default)           |
| Sync  | **orange fill** (background) | Sync to BPM; division picked by the rate/time slider |

The clock source is **auto-resolved internally** by priority:

1. **Host transport playing** → use host BPM via `getPlayHead`.
   Cache it as `hostBpmLastSeen` each block.
2. **In-app sequencer running** (and host not playing) → use
   `seqBpm`. Starting the in-app sequencer is the explicit signal
   "I'm driving the tempo now."
3. **Host paused, in-app seq stopped, host BPM ever seen** →
   freeze at `hostBpmLastSeen`. Avoids jolting the rate when a
   DAW user briefly pauses.
4. **Standalone, host BPM never seen** → fall back to `seqBpm`.

This uses two unambiguous transport signals (host playing,
sequencer running) and avoids the slider-movement-as-override
heuristic, which would have collided with preset loads and
automation.

The single button cycles Off ↔ Sync on click. Visual reference is
the existing **Delay-Vel button**.

---

## 2. Companion changes the user asked for in the same breath

### 2.1 LFO mode: 2-entry choice (`Free / Trig`) → 1-cycle button

Today the mode lives in `LfoMode` (`src/dsp/BlockParams.h:277-288`)
as a 2-entry choice rendered as a `ComboBox`
(`SynthPanel.cpp:171-173`). The user wants a **single toggle
button** showing "F" or "T" (Free / Trig) that cycles on click —
same visual language as the clock button. Layout per LFO row:

```
[ Wave ] [F/T] [⏱] [ Target ]   Rate  ──●──   Depth  ──●──
```

### 2.2 LFO Free/Trig is broken — bugfix is **deferred**

The user reports "Free/Trig funktioniert nicht". Wiring trail:

- `PluginProcessor.cpp:122-124` reads `lfo1Mode/lfo2Mode/lfo3Mode`
  → `lfoNTrigMode` bools.
- These flow into `voiceManager.setDroneNote(...)` at line 126 and
  the per-block update at `:1635`.
- Suspected: the bool is read but never reaches `Lfo::setMode` /
  `setRetriggerOnNote` inside `SynthVoice` — start there.

**The user explicitly deferred this fix** — feature work first,
this becomes its own commit afterwards. Don't bundle it in.

### 2.3 Delay layout — separate clock button (resolved)

The Delay panel header row currently has just `OFF / Stereo`. The
user considered extending it to OFF/Free/Sync but decided
**against** — that row should stay reserved for future Stereo /
Ping-Pong / etc. modes. Instead, **Delay gets the same separate
clock button** as LFO/Drift, placed left of the "Time" slider.

Acknowledged tradeoff: this breaks the slider-row symmetry slightly
(Delay's Time row now has a button-prefix the other Delay sliders
don't). The user accepts this; the alternative (extending
OFF/Stereo) was judged worse.

---

## 3. Code touchpoints

### 3.1 New APVTS parameters

For each of LFO 1/2/3, Drift 1/2/3, and Delay, add:

- `*ClockMode` — choice of `{ "off", "sync" }`.
- `*Division` — choice of musical divisions (see §3.2).

Old presets without these fields default to `ClockMode::Off` →
behaviour identical to v1.6.0-beta.1.

Param IDs (in `namespace PID` in `src/dsp/BlockParams.h`):

```
lfo1ClockMode,   lfo2ClockMode,   lfo3ClockMode
lfo1ClockDivision, lfo2ClockDivision, lfo3ClockDivision
drift1ClockMode, drift2ClockMode, drift3ClockMode
drift1ClockDivision, drift2ClockDivision, drift3ClockDivision
delayClockMode
delayClockDivision
```

The `Clock` infix on the Division PIDs is intentional — disambiguates
from the existing `seqDivision` (sequencer step duration) which is
unrelated.

Choice tables live in `BlockParams.h` next to the existing
`LfoWave` / `LfoMode` / `DriftWave` namespaces, keyed
`ClockMode::kEntries` and `ClockDivision::kEntries`. The
`ClockDivision` namespace also exports a `kFactor[]` array
(events-per-whole-note) for use in §3.4.

### 3.2 Division set (resolved)

User chose: straight + triplets + quintuplets. **No dotted, no
1/64.**

```
Straight:    1/1, 1/2, 1/4, 1/8, 1/16, 1/32
Triplets:    1/2t, 1/4t, 1/8t, 1/16t
Quintuplets: 1/4q, 1/8q, 1/16q
```

Total: 13 entries.

Slider ordering must be **monotonic in rate** (slow → fast) so the
slider feels coherent. The factor column is "events per whole
note":

| Index | Division | Factor |
| ----- | -------- | ------ |
| 0     | 1/1      | 1      |
| 1     | 1/2      | 2      |
| 2     | 1/2t     | 3      |
| 3     | 1/4      | 4      |
| 4     | 1/4q     | 5      |
| 5     | 1/4t     | 6      |
| 6     | 1/8      | 8      |
| 7     | 1/8q     | 10     |
| 8     | 1/8t     | 12     |
| 9     | 1/16     | 16     |
| 10    | 1/16q    | 20     |
| 11    | 1/16t    | 24     |
| 12    | 1/32     | 32     |

Triplet convention: 3 in the space of 2 (= 1.5× the straight rate).
Quintuplet convention: 5 in the space of 4 (= 1.25× the straight
rate). These are the standard DAW conventions; flag if a different
reading is wanted before locking in the table.

### 3.3 BPM source plumbing

- `processor.driftRegenBpm` already exists as `std::atomic<float>`
  — find where it is written. Reuse if it already represents the
  effective BPM the rest of the engine uses.
- Cache **host BPM** and **host transport state** in `processBlock`
  via `getPlayHead()->getPosition()->getBpm()` (or the older
  `getCurrentPosition()` API — check which the vendored JUCE
  supports). New members:
  ```cpp
  std::atomic<float> hostBpmLastSeen { 0.0f };
  std::atomic<bool>  hostPlayingNow  { false };
  ```
- Each block:
  ```cpp
  bool playing = false;
  if (auto* ph = getPlayHead())
      if (auto pos = ph->getPosition())
          if (pos->getIsPlaying()) {
              playing = true;
              if (auto bpm = pos->getBpm()) hostBpmLastSeen.store((float) *bpm);
          }
  hostPlayingNow.store(playing);
  ```
- The in-app sequencer's run-state is already tracked somewhere
  (search for the sequencer Play/Stop button handler — likely an
  `std::atomic<bool>` on the processor or a query on the
  `StepSequencer` instance). Expose it as a `bool seqRunningNow()
  const` method on the processor if it isn't already easy to read.
- Resolution helper:
  ```cpp
  float T5ynthProcessor::resolveSyncBpm() const {
      if (hostPlayingNow.load())  return hostBpmLastSeen.load();
      if (seqRunningNow())        return seqBpm.load();
      const float h = hostBpmLastSeen.load(std::memory_order_relaxed);
      return (h > 0.0f) ? h : seqBpm.load();
  }
  ```
  Priority: live host > running in-app seq > frozen host > `seqBpm`
  fallback for never-played-host (= standalone).

### 3.4 Sync rate computation

When `*ClockMode == sync`, compute the effective rate from BPM and
division (using `factor` from §3.2):

- LFO / Drift: `rateHz = factor * (bpm / 60.0) / 4.0`.
  (Whole note at `bpm` lasts `4 * 60/bpm` seconds; `factor` events
  per whole note ⇒ rate = `factor / wholeNoteSeconds`.)
- Delay Time: `delayMs = (60000.0 / bpm) * (4.0 / factor)`.

Look at `StepSequencer.h:60` for any existing `DIVISION_FACTORS`
table — reuse if values match. If they don't, prefer adding a
shared header rather than duplicating numbers.

LFO / Drift rate is currently set per block via
`lfo1.setRate(bp.lfo1Rate)` at `PluginProcessor.cpp:978`. Insert a
sync override before that:

```cpp
const int lfo1Clock = static_cast<int>(parameters.getRawParameterValue(PID::lfo1ClockMode)->load());
const float lfo1RateEff = (lfo1Clock == ClockMode::Off)
    ? bp.lfo1Rate
    : computeSyncRate(resolveSyncBpm(),
                      static_cast<int>(parameters.getRawParameterValue(PID::lfo1Division)->load()));
lfo1.setRate(lfo1RateEff);
```

Delay time: `PluginProcessor.cpp:1751` reads `baseDelayTime`. Wrap
it the same way.

### 3.5 UI: stepped slider that swaps semantics

The user's mental model: **one** Rate / Time slider per row, which
switches its labels between Hz/ms (Off) and division names (Sync).
Implementation:

- **Two backing APVTS params** (`*Rate` continuous Hz/ms,
  `*Division` choice). JUCE/APVTS can't carry two semantics on one
  param.
- **One on-screen `juce::Slider`** whose attachment swaps when
  ClockMode changes. Simplest: build both attachments, place two
  sliders in the same screen rect, toggle `setVisible` based on
  ClockMode.
- In Sync mode the slider is stepped (`setRange(0, 12, 1)` for the
  13 divisions) and uses a custom `getTextFromValue` returning the
  division name (`"1/4"`, `"1/8t"`, `"1/16q"`, …).
- In Off mode it is the existing continuous Hz/ms slider with its
  current attachment.

Users perceive it as one control because the screen position never
changes; only the labels do.

### 3.6 UI changes — row layouts

LFO row in `SynthPanel.cpp` (~line 171), new layout:

```
[ Wave ] [F/T] [⏱] [ Target ]   Rate  ──●──   Depth  ──●──
                                 ^ shows "Hz" (Off) or division (Sync)
```

The `[⏱]` button is a small `juce::TextButton` with a custom
`paintButton` override that draws the clock symbol and varies its
fill per state (grey vs orange-fill). The Delay-Vel button is the
visual reference.

Drift rows in `DriftPanel.cpp`: same minus the `[F/T]` (Drift has
no Free/Trig mode and doesn't need one). Just add the `[⏱]` clock
button left of "Rate".

Delay row: add `[⏱]` left of "Time". (See §2.3 — the OFF/Stereo
row is left untouched.)

### 3.7 Preset / IPC compatibility

- Preset format (`.t5p`): all new APVTS params serialise
  automatically. Old presets without these fields default to
  `ClockMode::Off` → behaviour identical to v1.6.0-beta.1.
- IPC: none of these fields cross the Python boundary; they're all
  C++-side DSP params. No IPC changes.

---

## 4. Implementation order

User explicitly requested: **feature and frontend design first,
then the Free/Trig bugfix.**

1. **APVTS layout:** add `*ClockMode` + `*Division` for all 7
   targets (LFO 1/2/3, Drift 1/2/3, Delay). No behaviour yet —
   just wire them up so presets save/load.
2. **Frontend design pass** for the dual-semantics slider + the
   `[⏱]` clock button. Get the visuals right before the DSP wires
   up — easier to iterate on UI in isolation.
3. **Host BPM plumbing** in `processBlock`, with the
   freeze-on-pause semantics.
4. **Sync rate computation** for LFO 1/2/3 (one target proves the
   pattern).
5. **Apply to Drift 1/2/3.**
6. **Apply to Delay Time.**
7. **LfoMode → 1-cycle button** (UI rework, replaces the existing
   `ComboBox`).
8. **Free/Trig bugfix.** Separate commit, after the feature lands.
9. **Document** in CHANGELOG, in-app Manual §1, and
   ARCHITECTURE.md.

---

## 5. Out of scope (don't touch)

- The `seqBpm` parameter itself (already correct).
- Sequencer step rate (already BPM-synced via `seqBpm`).
- Arpeggiator rate (already BPM-synced).
- The injection-mode work shipped in v1.6.0-beta.1 — done, do not
  revisit.

---

## 6. Resolved decisions (was: open questions)

1. **Two states, not three.** Off / Sync. Clock source auto-resolved
   internally (host when transport playing, else `seqBpm`; freeze
   at last seen host BPM during pause).
2. **Delay layout:** separate clock button (not extending
   OFF/Stereo). Slider-symmetry break is accepted; the OFF/Stereo
   row stays reserved for future Stereo / Ping-Pong / etc. modes.
3. **Division set:** 1/1, 1/2, 1/4, 1/8, 1/16, 1/32 + triplets at
   1/2t–1/16t + quintuplets at 1/4q–1/16q. **No dotted, no 1/64.**
4. **External-clock fallback when transport paused:** priority is
   live host > running in-app sequencer > frozen `hostBpmLastSeen`
   > `seqBpm` (standalone, host never seen). Starting the in-app
   sequencer is the explicit signal that the user is now driving
   the tempo themselves; pausing the host alone keeps the LFO at
   the last seen host rate so a brief DAW pause doesn't jolt
   anything.
5. **Slider semantics:** one slider per row, swaps between Hz/ms
   labels (Off) and division names (Sync). Two backing APVTS
   params, one visible UI control. Stepped in Sync mode.

The only thing left to confirm before locking the table in §3.2 is
the triplet/quintuplet *convention* (3-in-2 / 5-in-4) — these are
the DAW defaults and likely correct, but flag if the user wants a
different reading.
