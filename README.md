# T5ynth

**A text-to-sound synthesizer that navigates the T5 embedding space of a diffusion audio model.**

T5ynth is a JUCE-based synthesizer plugin (Standalone / VST3 / AU) that repurposes Stability AI's [Stable Audio Open](https://huggingface.co/stabilityai/stable-audio-open-1.0) model as an oscillator — not to generate finished music, but to produce raw sonic material for human-nonhuman artistic collaboration.

> *T5ynth is developed by [Prof. Dr. Benjamin Jörissen](https://github.com/joeriben) as part of the [AI4ArtsEd](https://github.com/joeriben/ucdcae-ai-lab) project at the UCDCAE AI Lab.*

---

## Context: Why This Exists

T5ynth emerged from [AI4ArtsEd](https://github.com/joeriben/ucdcae-ai-lab), a research project investigating alternative, non-standard uses of AI for educational purposes. AI4ArtsEd deliberately subverts consumerist user-subject positions and approaches AI from a critical, empowerment-oriented perspective.

### The Problem with Generative Audio AI

Text-to-audio models like Stable Audio are designed for a specific purpose: generating finished audio content from text prompts. Their intended use case is the substitution of creative labor — type a description, get a result. This positions the user as a consumer of AI output rather than an active creative agent.

### Open Source as Strategy

Stable Audio Open deserves credit: it was trained on ~486,000 Creative Commons-licensed recordings from Freesound and the Free Music Archive — not on copyrighted music. Stability AI conducted copyright verification and removed flagged content before training. This is genuinely better practice than much of the industry.

At the same time, "open source" in the AI industry operates as strategic marketing. Releasing a smaller, less capable model (Open) builds community and ecosystem around commercial products (Stable Audio Pro). The openness is real and useful, but it serves a business strategy. We should be honest about this dialectic rather than either dismissing or celebrating it uncritically.

### What T5ynth Does Differently

T5ynth takes this openly released model and implements a non-intended use: instead of generating finished audio, it treats the model's 768-dimensional T5 text embedding space as a navigable sonic terrain. The diffusion model becomes an oscillator — a sound source that a musician shapes, filters, modulates, and sequences like any other synthesizer component.

This inverts the appropriation relationship:

- **Intended use:** Human types prompt → AI produces finished content → Human consumes
- **T5ynth:** Human navigates embedding space → AI produces raw material → Human creates

The generated audio is not the output — it is the starting point. It requires human musicianship, sound design, and compositional decisions to become anything. T5ynth does not make music. It makes material for making music.

Whether this actually succeeds in reframing the human-AI relationship is an open question, not a solved one. That's the research.

---

## Features

- **Two Engine Modes:** Sampler (loop/one-shot/ping-pong) and Wavetable (pitch-synchronous frame extraction with mip-mapped band-limiting)
- **Embedding Space Navigation:** Direct manipulation of 768 T5 dimensions, 8 semantic axes (tonal/noisy, bright/dark, etc.), A/B prompt interpolation
- **Dimension Explorer:** Visualize and edit individual embedding dimensions, sorted by magnitude
- **Full Synthesizer Architecture:** ADSR envelopes (amplitude + 2 mod), 2 LFOs, 3 drift LFOs, state-variable filter (LP/HP/BP, 6-24dB), modulation matrix
- **Effects:** Delay with damping, convolution reverb (EMT 140 plate), limiter
- **Sequencer & Arpeggiator:** 16-step sequencer with per-step note/velocity/gate/glide, arpeggiator (up/down/updown/random)
- **Presets with Embedded Audio:** .t5p format stores parameters + generated audio + embeddings — instant recall without regeneration
- **Cross-Platform:** macOS (MPS acceleration), Linux (CUDA), CPU fallback

## Architecture

```
┌─────────────────────────────────────────────────────┐
│  JUCE Plugin (C++)                                  │
│  ┌─────────┐  ┌──────────┐  ┌────────┐  ┌───────┐ │
│  │ Sampler  │  │Wavetable │  │ Filter │  │Effects│ │
│  │ Player   │  │Oscillator│  │  SVF   │  │Dly+Rev│ │
│  └────┬─────┘  └────┬─────┘  └───┬────┘  └──┬────┘ │
│       └──────┬───────┘            │          │      │
│         Voice Manager (8 voices)──┴──────────┘      │
│              ↑                                      │
│    ┌─────────┴──────────┐                           │
│    │ loadGeneratedAudio │                           │
│    └─────────┬──────────┘                           │
│              │ Unix pipes (stdin/stdout)             │
├──────────────┼──────────────────────────────────────┤
│  Python Backend                                     │
│  ┌─────────┐ ┌──────────┐ ┌─────┐ ┌─────────────┐ │
│  │T5 Encode│→│ Embedding│→│ DiT │→│ VAE Decode  │ │
│  │ (768d)  │ │   Manip  │ │     │ │ 44.1kHz PCM │ │
│  └─────────┘ └──────────┘ └─────┘ └─────────────┘ │
└─────────────────────────────────────────────────────┘
```

The inference runs in a Python subprocess (BrownianTree SDE sampler requires torchsde). Audio transfers to JUCE via binary pipe protocol — no HTTP overhead.

---

## Building

### Requirements

- **CMake** >= 3.22
- **C++20** compiler (Clang, GCC, MSVC)
- **LibTorch** (pre-built, not pip torch) — [download here](https://pytorch.org/get-started/locally/)
- **libcurl**
- **Python** >= 3.10 with pip

### Steps

```bash
# Clone
git clone https://github.com/joeriben/t5ynth.git
cd t5ynth

# Python backend dependencies
python3 -m venv .venv
source .venv/bin/activate
pip install -r backend/requirements.txt

# Configure (adjust LibTorch path)
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/libtorch

# Build
cmake --build build

# Run standalone
./build/T5ynth_artefacts/Debug/Standalone/T5ynth.app/Contents/MacOS/T5ynth  # macOS
./build/T5ynth_artefacts/Debug/Standalone/T5ynth                            # Linux
```

### Model Download

T5ynth requires the Stable Audio Open 1.0 model (~11GB). On first launch, use the Settings panel to either:

1. **Auto-download:** Enter your [HuggingFace token](https://huggingface.co/settings/tokens) (requires accepting the [model license](https://huggingface.co/stabilityai/stable-audio-open-1.0)) and click Download
2. **Manual:** Place the model files in `~/Library/T5ynth/models/stable-audio-open-1.0/` (macOS) or `~/.local/share/T5ynth/models/stable-audio-open-1.0/` (Linux)

---

## License

T5ynth is licensed under the **GNU General Public License v3.0** — see [LICENSE](LICENSE).

This means you are free to use, modify, and redistribute T5ynth, provided that derivative works are also released under GPLv3 with source code available.

### Third-Party Components

- **Stable Audio Open 1.0** — [Stability AI Community License](https://stability.ai/community-license-agreement). The model is not included in this repository. Users download it separately and must accept its license. Powered by Stability AI.
- **JUCE Framework** — AGPLv3 (vendored in `JUCE/`)
- See [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md) for full details.

### Citation

If you use T5ynth in academic work:

```
Prof. Dr. Benjamin Jörissen / AI4ArtsEd — UCDCAE AI Lab
https://github.com/joeriben/t5ynth
```

---

*T5ynth is a research artifact. It is an argument in code form about what generative AI could be when we refuse the subject positions it was designed to produce.*
