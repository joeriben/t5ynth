# Changelog

## v1.6.0-beta.1 — 2026-04-30

- **New: prompt-injection modes.** The Generation panel grows a six-button mode row above the Alpha slider. The classical A↔B crossfade is now one of six modes — *Linear*, *Fine*, *Layer*, *Kombi 1*, *Kombi 2*, *Kombi 3* — that select different ways prompt B is mixed into prompt A inside the diffusion pipeline. Linear preserves bit-for-bit compatibility with prior versions; the other five modes operate on the diffusion sampler steps and on individual DiT block cross-attention layers. See the in-app Manual §1 for the user-facing description and ARCHITECTURE.md §6.5 for the mechanics.
- **Fine mode** — early sampler steps see prompt A only; after a transition step the cross-attention conditioning swaps to a Fine-controlled blend of A and B. Drives a single intensity slider (0–1) coupled to both the transition point and the late-phase blend amount. Implemented as a `DiTWrapper.forward` kwargs swap on the inner sampler.
- **Layer mode** — a two-thumb range slider defines a B-zone over the 16 DiT blocks. Per-block forward_pre_hooks override each block's cross-attention context with a sigmoid top-hat blend of A and B. Useful for surgical "this prompt only on these layers" experiments.
- **Kombi 1 / 2 / 3** — preset combinations of step phase × layer band, each with a hardcoded layer range and a Fine-style intensity slider. Kombi 1 = surface (blocks 0..4), Kombi 2 = broad mid (blocks 4..12), Kombi 3 = narrow center (blocks 6..10). Hard layer mask (no edge softening) so slider=1 is genuinely 100 % B in the band's blocks.
- **Per-mode slider memory.** Fine and the three Kombi modes each remember their own intensity-slider value, so A/B-ing them by clicking buttons does not destroy the last-used position of any individual mode. Linear and Layer already had independent state.
- **Mode buttons trigger regeneration.** Clicking any of the six mode buttons fires a fresh inference with the newly selected mode, matching the existing slider/drift auto-regen UX.
- **Slider-scale fix.** The Fine/Kombi intensity slider now displays 0–1 (was 0.5–1.0) — internal mapping onto the audible region of `injection_transition_at` / `late_phase_alpha` is unchanged. Old presets reload their stored value into the corresponding mode slot, but their effective rendering may differ since the slider value is no longer remapped before being sent to the backend.

## v1.3.0-beta.1 - 2026-04-24

- Expanded the instrument from an early beta into a fuller text-to-sound workflow with independent wavetable extraction regions, shared P1/P2/P3 traversal controls, and a clearer wavetable start-point model.
- Added session persistence for the standalone app so working states survive quit/relaunch instead of behaving like disposable test sessions.
- Reworked the preset workflow with factory presets, `INIT`, explicit P1/P2/P3 locks, auto-trim support, and cleaner preset migration/loading behavior.
- Broadened the musical control surface with a microtuning system, non-Western scales, Shruti support, sampler-mode tuning support, and a more expressive generative sequencer.
- Extended modulation and motion design with drift random waveforms, additional drift targets, sample-and-hold LFO support, ghost sliders, and a denser but more legible sequencer layout.
- Moved the user-facing setup flow toward installer-first distribution on macOS, added a Windows installer path, and tightened model/preset placement around per-user defaults with system-wide scan fallbacks.
- Improved the guided onboarding and in-app documentation with the embedded manual overlay, clearer install guidance, and a more robust download/setup flow for model assets.
- Hardened backend startup, model discovery, and download handling so failures surface more clearly and model paths behave consistently across packaged installs.
- Stabilized playback and interaction around generate/regenerate, active-voice sampler behavior, modulation editing, shutdown, and repaint-heavy GUI paths.
- Kept the core dependency baseline unchanged across the beta line: `backend/requirements.txt` and the top-level CMake `FetchContent` pins for `nlohmann/json` and `signalsmith-stretch` did not change between `v1.0.0-beta.1`, `v1.0.0-beta.2`, and `v1.0.0-beta.3`.

## Notes

- `v1.3.0-beta.1` keeps the release on the beta line while the broader rollout (VST3/AU and additional public platforms) is still unfinished; the current tagged release asset remains the macOS installer only.
- The version number stays monotonic with the older internal `1.2.x` build line and avoids another downgrade in bundle / installer versioning.
