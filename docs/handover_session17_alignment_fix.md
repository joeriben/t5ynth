# Session 17 Sub-Handover — Fix BPM-sync UI Alignment

**Status: Step 2 frontend is wired but alignment is wrong.**
Open this with a fresh context. The previous session ran out of
budget while pattern-matching layout fixes that contradict the
project's existing column-alignment design.

The user's hint: *"Alle anderen Zeilen im frontend wenden ein
Prinzip an, und es ist bereits refaktoriert worden."*
That refactor is **commit 7cffb6e7** ("Fix responsive modulation
slider alignment") plus its follow-up **1d260ce7** ("Prevent
modulation label width feedback loops"). Read both diffs in full
before touching any layout code.

---

## 1. The principle you must internalise first

T5ynth's modulation rows (Envelopes, LFOs, Drift) share **one
left label column and one right label column** across the whole
modulation section. Alignment is enforced by:

1. Each `SliderRow` exposes
   `getNaturalLabelWidthForAvailableWidth(int totalWidth)` — the
   width its label *would* take given a column width, ignoring any
   forced override.
2. `SynthPanel::resized()`, **before** dispatching to per-section
   layout helpers, computes:
   - `leftLabelWidth = std::max({ ...natural widths of every row that
     lives in the left column... })`
   - `rightLabelWidth = std::max({ ...same for the right column... })`
3. Both forced widths are applied via `setForcedLabelWidth(...)` to
   **every** row in their respective column.
4. The per-section layout helpers (`layoutEnv`, `layoutLfo`,
   `layoutDrift`, ...) then call `layoutSliderRowPairBounds(row,
   leftRow, rightRow, gap)` and trust the rows to use the forced
   widths.

Critical invariant from commit 1d260ce7 (read the comment it added
at `GuiHelpers.h:259-263` *and* in `SynthPanel.cpp` immediately
above the `leftLabelWidth` block): the max **must** be derived from
*natural* widths. Computing from the *currently forced* width
creates a feedback loop where each resize grows the column.

The helper `modulationForcedLabelWidthFor` at the top of
`SynthPanel.cpp` (search for the function name) is the canonical
way to query a row's natural label width plus an optional
curve-button reservation. Reuse it.

This same pattern is **not** yet applied in `FxPanel` — Delay
currently uses pure pairwise `layoutSliderRowPairBounds` with no
cross-pair coordination. That is part of why FB and Mix don't sit
at identical X today, and the BPM-sync work made it more visible.

---

## 2. What's already in place (don't redo)

The Step 2 frontend code is correct *except* for column alignment:

- ✅ APVTS layout (Step 1, commit pending) — `ClockMode`,
  `ClockDivision` namespaces in `BlockParams.h`, 14 new PIDs and
  matching `AudioParameterChoice` entries in
  `PluginProcessor::createParameterLayout()`.
- ✅ `ClockButtonLnF` in `GuiHelpers.h` (clock-face Path icon,
  toggle-state-driven orange fill).
- ✅ `LfoSection` and `DriftSection` extended with `clockBtn`,
  `clockModeHidden`, `divisionRow`, plus their attachments. LFO
  also gained a `[F/T]` 1-cycle button (`modeBtn` + `modeHidden`)
  that replaces the old "Free⌄" ComboBox.
- ✅ `FxPanel` extended with `delayClockBtn`,
  `delayClockModeHidden`, `delayDivisionRow`, plus attachments.
- ✅ ClockMode-driven visibility swap: hidden ComboBox holds APVTS
  state, click cycles Off ↔ Sync, `onChange` toggles
  `setVisible` between `rateRow` (or `delayTimeRow`) and the
  division row.
- ✅ Division formatter (`1/4`, `1/8T`, `1/16Q`, ...) using
  `ClockDivision::kEntries[idx].label`.

⚠️ **Naïve forces I introduced and you must rip out before
applying the proper pattern**:

- `lfo.rateRow->setForcedLabelWidth(36)` and the matching
  `divisionRow` line in `initLfo` (`SynthPanel.cpp`).
- `drift.rateRow->setForcedLabelWidth(36)` and matching
  `divisionRow` line in `initDrift`.
- `setForcedLabelWidth(commonLabelW)` loop in `FxPanel::resized()`
  (line where the lambda iterates over all five delay rows).
- `delayTimeRow->getLabel().setText({}, ...)` and
  `delayDivisionRow->getLabel().setText({}, ...)` in `FxPanel`'s
  ctor — keep these (the row itself is correctly label-less; the
  clock button overlays the reserved slot).

`setForcedValueWidth` calls (56 / 64 px) on the rate+division
pairs are correct and should stay — they keep the value column
stable when the formatter swaps Hz/ms ↔ division name.

---

## 3. Concrete fixes you need to apply

### 3.1 SynthPanel — extend the existing modulation column to cover divisionRows

`SynthPanel::resized()` already has a block (search for the local
variable `leftLabelWidth`) that computes the shared column
widths. The lists currently miss the new `divisionRow` for every
LFO and every Drift.

For the **left** column (where `rateRow` lives):

1. Add one entry to the `std::max({...})` initialiser list — only
   one representative row is needed since all `divisionRow`s have
   the same label text "Rate" *and* the same forced value width
   (so the same natural label width). E.g.:
   ```cpp
   modulationForcedLabelWidthFor(*lfo1.divisionRow, modColumnWidth)
   ```
2. Add **every** `lfo*.divisionRow.get()` and
   `drift*.divisionRow.get()` to the `for (auto* row : { ... })
   row->setForcedLabelWidth(leftLabelWidth);` list.

The right column (`depthRow`) is unchanged — there is no
"divisionDepthRow".

### 3.2 FxPanel — introduce the same column system for Delay

This panel never received the refactor. Today
`FxPanel::resized()` lays the delay rows out as two independent
pairs, so FB and Mix have different label widths and don't align.
You need to add (right before the Delay layout block, the same
shape as `SynthPanel::resized()`'s modulation block):

```cpp
const int delayPairGap = 2;
const int delayColumnW = juce::jmax(0, (area.getWidth() - delayPairGap) / 2);

const int delayLeftLabelW = std::max({
    delayTimeRow->getNaturalLabelWidthForAvailableWidth(delayColumnW),
    delayDivisionRow->getNaturalLabelWidthForAvailableWidth(delayColumnW),
    delayDampRow->getNaturalLabelWidthForAvailableWidth(delayColumnW)
});
const int delayRightLabelW = std::max({
    delayFbRow->getNaturalLabelWidthForAvailableWidth(delayColumnW),
    delayMixRow->getNaturalLabelWidthForAvailableWidth(delayColumnW)
});

for (auto* r : { delayTimeRow.get(), delayDivisionRow.get(),
                 delayDampRow.get() })
    r->setForcedLabelWidth(delayLeftLabelW);
for (auto* r : { delayFbRow.get(), delayMixRow.get() })
    r->setForcedLabelWidth(delayRightLabelW);
```

Then keep the existing `layoutSliderRowPairBounds` calls — they
will now produce aligned slider tracks across all four delay
rows.

The `delayClockBtn` overlays the empty label slot at the start of
the Time row, so its width must equal `delayLeftLabelW`:

```cpp
delayClockBtn.setBounds(pair1[0].withWidth(delayLeftLabelW));
```

(Currently the code uses a separate `commonLabelW` computation —
replace it with `delayLeftLabelW`.)

### 3.3 SliderRow API extension — keep or revert?

The previous session added `setForcedValueWidth(int)` /
`clearForcedValueWidth()` to `SliderRow` (`GuiHelpers.h`). It is
**legitimate and used correctly** for the rate ↔ division swap:
both rows force the same value width so the slider's *right* edge
is stable when the value text widens or shrinks. Keep this API.

The `applyForcedLabelWidth` parameter inside `getLayoutProfile`
also gates `forcedValueWidth`. That's deliberate — natural
queries (`getNaturalLabelWidthForAvailableWidth`) bypass *both*
forces. Read the call sites to confirm before changing.

---

## 4. Verification

After applying §3.1 and §3.2:

1. Build: `cmake --build build_clean --config Release -j$(sysctl -n hw.ncpu)`.
2. Launch the standalone:
   `open build_clean/T5ynth_artefacts/Release/Standalone/T5ynth.app`.
3. Click the clock button on LFO 1. The slider track must NOT
   move horizontally. The value swap is the only visual change.
4. Same for Drift 1 and Delay.
5. The Delay row should align: Time slider track left edge =
   Damp slider track left edge; FB slider track = Mix slider
   track. The clock button sits exactly above the "Damp" label
   column.
6. Resize the plugin window. Alignment must hold across resizes
   (this is what the feedback-loop fix in 1d260ce7 protects
   against — verify by toggling the clock several times after
   resize to confirm no progressive drift).

If anything still drifts, the cause is almost certainly that some
row was missed from the `setForcedLabelWidth(...)` list. Re-read
the §1 invariant: *every row in the column gets the forced
width*.

---

## 5. Why the previous session got stuck

For the record (and to save the next session from repeating it):

- Used `setForcedLabelWidth(36)` as a magic number on individual
  rows, ignoring the centralised column-width computation.
- Used `setForcedLabelWidth(0)` on Delay rows to "drop the Time
  label", which made the Time slider start at column-X 0 while
  Damp started at column-X (its natural label width) — a
  guaranteed misalignment.
- Tried to fix the right-column FB/Mix mismatch by hardcoding all
  five delay rows to the same forced width — close to the right
  idea but bypassed the natural-width-max derivation, locked in a
  Damp-derived width that didn't reflect FB/Mix.
- Did not read commit 7cffb6e7 / 1d260ce7 before editing.

The lesson: when a layout pattern is already deployed across
several panels, **find it and extend it**. Don't synthesise a new
mechanism alongside.

---

## 6. Out of scope for this handover

- DSP wiring of BPM sync (Steps 3–6 of
  `handover_session17_bpm_sync.md`) — host BPM plumbing, the
  `seqRunningNow()` priority resolution, and the per-block sync
  rate computation. Do not start these before alignment is fixed
  and the user has signed off on the visual.
- The deferred Free/Trig regression bugfix (Step 8 in the parent
  handover).
- Anything outside `src/gui/SynthPanel.{h,cpp}` and
  `src/gui/FxPanel.{h,cpp}`. The APVTS work in
  `BlockParams.h` and `PluginProcessor.cpp` is complete and
  correct.
