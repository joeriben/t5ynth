# Session 17 Handover — BPM Sync for LFO / Drift / Delay (+ LFO mode UI rework)

**Status: not started.** This is a fresh feature ticket, written for a
cold session. Read it end-to-end before touching code; the UX
proposal interleaves with a code-level bug fix and an enum
re-shaping, and getting the order right matters.

---

## 1. The feature in one sentence

Add BPM-sync to **LFO 1/2/3**, **Drift LFO 1/2/3**, and **Delay Time**
so their rates / times can be quantised to musical divisions of a
clock, with a small inline UI selector that sits to the **left of
the existing "Rate" slider** (or, for Delay, to the left of the
"Time" slider).

The user wants two clock sources, selectable per parameter:

- **External clock** — host transport BPM (DAW playhead), when the
  DAW is actually playing.
- **Internal clock** — the existing `seqBpm` APVTS parameter that
  the in-app sequencer already uses (defined in `PluginProcessor.cpp`
  near line 2871; UI in `SequencerPanel.cpp:260-264`).

A third state is "off" → no sync, the rate / time slider keeps its
current free-running Hz / ms semantics.

The user mocked it as a small **clock-symbol button with a frame**,
visually similar to the **Delay-Vel** button they already have.
Three states cycle on click:

| State          | Visual                       | Behaviour                                  |
| -------------- | ---------------------------- | ------------------------------------------ |
| Off            | grey symbol, no fill         | Free-running rate / time (current default) |
| External clock | **orange fill** (background) | Sync to host BPM via `getPlayHead`         |
| Internal clock | **orange frame** (border)    | Sync to `seqBpm` APVTS                     |

When sync is on, the rate/time slider's value semantics shift from
Hz/ms to a **musical division** (1/1, 1/2, 1/4, 1/8, 1/16, 1/32,
plus dotted "·" and triplet "T" variants). The slider can either
show those division names directly (preferred) or remain a
continuous slider whose value is rounded to the nearest division at
read time.

---

## 2. Companion changes the user asked for in the same breath

### 2.1 LFO mode is currently a 2-entry choice (`Free / Trig`) — convert to a "1 cycle" button

Today the mode lives in `LfoMode` (`src/dsp/BlockParams.h:277-288`)
as a 2-entry choice:

```cpp
namespace LfoMode {
    enum : int { Free = 0, Trigger = 1 };
    static constexpr ChoiceEntry kEntries[] = {
        { "free",    "Free" },
        { "trigger", "Trig" }
    };
    ...
}
```

The UI is a `ComboBox` populated from this list (`SynthPanel.cpp:
171-173`). The user wants it as a **single toggle button** that
shows "F" or "T" (Free / Trig) and cycles on click — same visual
language as the proposed clock button. Then the **clock button sits
next to it**, so each LFO row has the layout:

```
[F/T] [⏱]  Rate  ───●─────  Free
```

The user explicitly called this "1 cycle button" — meaning a single
button that cycles through its states on click, NOT a dropdown.

### 2.2 LFO Free/Trig is broken — fix while you're there

The user reports "Free/Trig funktioniert nicht". The wiring goes:

- `PluginProcessor.cpp:122-124` reads `lfo1Mode/lfo2Mode/lfo3Mode`
  → `lfoNTrigMode` bools.
- These are passed into `voiceManager.setDroneNote(note, vel,
  lfo1TrigMode, ...)` at line 126 and into the per-block update
  path at `:1635`.
- Investigation needed inside `VoiceManager::setDroneNote` and the
  `SynthVoice` per-LFO trigger path. Verify that `Trig` mode
  actually re-phases the LFO on note-on; `Free` should let it run
  continuously across note boundaries.

The grep revealed `lfoNTrigMode` is read in two places, but I did
not chase whether the bool actually reaches the LFO instance.
Likely the bug is "param is read but never wired into
`Lfo::setMode` / `setRetriggerOnNote`". Start there.

### 2.3 Delay layout — the user is unsure where the sync button goes

The Delay panel header row currently has just `OFF / Stereo` (a
2-entry mode toggle that selects whether the delay is bypassed or
running in stereo). The user proposed extending this row to
**OFF / Free / Sync**, dropping the standalone `[F/T] [⏱]` pair
that LFO/Drift rows have, and instead embedding the sync state in
the existing mode selector:

| Old        | New              |
| ---------- | ---------------- |
| OFF        | OFF              |
| Stereo     | Free (= Stereo)  |
|            | Sync             |

Discuss with the user whether to also keep a separate Mono/Stereo
choice (the current `Stereo` button is already inconsistent: there
is no Mono mode — clicking OFF disables, clicking Stereo enables).
A more honest design might be: keep OFF / ON (the current
"OFF/Stereo" pair becomes "OFF/ON"), then add a separate Free/Sync
toggle and a separate Mono/Stereo toggle if any of those gain
multiple values. Confirm before refactoring.

The user's wording was tentative ("ggf. bei der Typ.Zeile, die eh
erst 2 Einträge hat") — this is the part to ask back about.

---

## 3. Code touchpoints

### 3.1 New APVTS parameters

For each of LFO 1/2/3, Drift 1/2/3, and Delay, add:

- `*ClockMode` — choice of `{ "off", "external", "internal" }`.
- `*Division` — choice of musical divisions (`{ "1/1", "1/2", "1/4",
  "1/4d", "1/4t", "1/8", ... }`).

Or — if the existing rate/time slider should itself snap to
divisions when sync is on — keep the existing slider param and add
only the `*ClockMode`. A separate `*Division` param is cleaner
because divisions are discrete; rate/time sliders are continuous in
Hz/ms.

I recommend separate params: the slider stays Hz/ms (used in
`*ClockMode == "off"`), the division choice is read in `external`
and `internal` modes. Existing presets (no `*ClockMode` field) load
as `off` → no behavioural change.

Param IDs to add to `src/PIDs.h` (or wherever PIDs live — grep for
`PID::lfo1Mode` to find the file):

```
lfo1ClockMode, lfo2ClockMode, lfo3ClockMode
lfo1Division,  lfo2Division,  lfo3Division
drift1ClockMode, drift2ClockMode, drift3ClockMode
drift1Division,  drift2Division,  drift3Division
delayClockMode
delayDivision
```

The choice tables go in `BlockParams.h` next to the existing
`LfoWave` / `LfoMode` / `DriftWave` namespaces, keyed
`ClockMode::kEntries` and `Division::kEntries`.

### 3.2 BPM source plumbing

- `processor.driftRegenBpm` already exists as a `std::atomic<float>`
  (read at `PromptPanel.cpp:1174`) — find where it gets written,
  this is likely the existing internal-clock BPM.
- Host BPM via `getPlayHead()->getCurrentPosition()` (deprecated in
  newer JUCE) or `getPlayHead()->getPosition()->getBpm()` — check
  which API the project's vendored JUCE supports. Cache it in
  `processBlock` once per block, expose as
  `processor.hostBpm.load(std::memory_order_relaxed)`.
- Both internal and external BPM should be available to LFO / Drift
  / Delay. Add a small helper:
  ```cpp
  float T5ynthProcessor::resolveSyncBpm(int clockMode) const {
      if (clockMode == ClockMode::Internal) return seqBpm.load();
      if (clockMode == ClockMode::External) return hostBpm.load();
      return 0.0f;  // off — caller falls back to free-running
  }
  ```

### 3.3 Sync rate computation

When `*ClockMode != off`, compute the effective rate from BPM and
division:

- LFO / Drift: `rateHz = (bpm / 60) * divisionFactor` where
  `divisionFactor = 1.0` for 1/4-note, `0.5` for 1/2-note, `2.0`
  for 1/8-note, etc. Look at `StepSequencer.h:60` for the
  existing `DIVISION_FACTORS` table — reuse it if possible.
- Delay Time: `delayMs = (60_000 / bpm) * (4.0 / divisionFactor)` —
  i.e. quarter-note ms divided by the division factor.

LFO / Drift rate is currently set per block via `lfo1.setRate(bp.lfo1Rate)`
at `PluginProcessor.cpp:978`. Insert a sync override before that:

```cpp
const int lfo1Clock = static_cast<int>(parameters.getRawParameterValue(PID::lfo1ClockMode)->load());
const float lfo1RateEff = (lfo1Clock == ClockMode::Off)
    ? bp.lfo1Rate
    : computeSyncRate(resolveSyncBpm(lfo1Clock),
                      static_cast<int>(parameters.getRawParameterValue(PID::lfo1Division)->load()));
lfo1.setRate(lfo1RateEff);
```

Delay time: `PluginProcessor.cpp:1751` reads `baseDelayTime`. Wrap
it the same way.

### 3.4 UI changes

Each LFO row in `SynthPanel.cpp` (around line 171) currently
renders:

```
[ Wave ] [ Mode ] [ Target ]   Rate  ──●──   Depth  ──●──
```

New layout:

```
[ Wave ] [F/T] [⏱] [ Target ]   Rate  ──●──   Depth  ──●──
                                 ^ shows "Hz" off, "1/4" external, "1/4" internal
```

The `[⏱]` button is a small `juce::TextButton` with a custom
`paintButton` override that draws the clock symbol and varies its
fill / border per state. Look at how the existing Delay-Vel button
is drawn — the user explicitly cited it as the visual reference.

Drift rows in `DriftPanel.cpp` get the same treatment minus the
`[F/T]` (Drift has no Free/Trig mode today; it doesn't need one
either). Just add the `[⏱]` clock button left of "Rate".

Delay: see §2.3 above. Either add a clock button next to "Time"
or extend the type row.

### 3.5 Preset / IPC compatibility

- Preset format (`.t5p`): all new APVTS params serialise
  automatically. Old presets without these fields default to
  `ClockMode::Off` → behaviour identical to v1.6.0-beta.1.
- IPC: none of these fields cross the Python boundary; they're all
  C++-side DSP params. No IPC changes.

---

## 4. Suggested implementation order

1. **Fix Free/Trig first.** It's a regression bug, independent of
   the new feature. Bug-fix commit on its own.
2. **Add clock-mode UI as a 1-cycle button** for LFO mode (replace
   the current ComboBox). This validates the visual pattern before
   introducing it everywhere.
3. **Wire host BPM** via `getPlayHead`. Land before the sync logic
   so it can be tested in isolation.
4. **Add `*ClockMode` + `*Division` APVTS params** for LFO 1/2/3,
   then UI clock button.
5. **Apply same pattern to Drift 1/2/3**.
6. **Apply to Delay Time.** Discuss the OFF/Free/Sync row with the
   user before refactoring the existing OFF/Stereo toggle.
7. **Document** in CHANGELOG, in-app Manual §1, and ARCHITECTURE.md
   (if BPM-sync infrastructure becomes architecturally meaningful).

---

## 5. Out of scope (don't touch)

- The seqBpm parameter itself (already correct).
- Sequencer step rate (already BPM-synced via the same `seqBpm`).
- Arpeggiator rate (already BPM-synced).
- The injection-mode work shipped in v1.6.0-beta.1 — done, do not
  revisit.

---

## 6. Open questions to ask the user before coding

1. **Delay's mode row** — extend the existing OFF/Stereo to
   OFF/Free/Sync, or add a separate clock button (consistent with
   LFO/Drift)? See §2.3.
2. **Division coverage** — minimum useful set is probably
   `{ 1/1, 1/2, 1/4, 1/8, 1/16, 1/32 }`. Add dotted (·) and
   triplet (T)? Add 1/64? Confirm before fixing the choice list.
3. **External-clock fallback** — when host transport is *not*
   playing, what should `external` do? Freeze? Fall through to
   internal? Free-run? Probably "freeze rate at last-known BPM"
   so a paused DAW doesn't desync the LFO from the listener's
   intuition.
4. **Slider semantics in sync mode** — does the existing
   continuous Rate slider become a stepped division-picker, or
   does the division have its own dedicated control? Recommend
   the latter (separate `*Division` param) — see §3.1.

Get these four answers before writing any APVTS layout.
