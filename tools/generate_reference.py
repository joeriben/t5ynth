#!/usr/bin/env python3
"""Generate reference audio using the REAL diffusers pipeline.
Patches the last scheduler step to avoid torchsde crash on CPU."""

import torch, math, sys
from pathlib import Path

MODEL_DIR = Path("~/Library/T5ynth/models/stable-audio-open-1.0").expanduser()

print("Loading pipeline...")
from diffusers import StableAudioPipeline
pipe = StableAudioPipeline.from_pretrained(str(MODEL_DIR), torch_dtype=torch.float32).to("cpu")

# Monkey-patch: skip noise sampling when sigma_t = 0 (final step)
original_step = pipe.scheduler.step.__func__
def patched_step(self, model_output, timestep, sample, **kwargs):
    step_index = self.step_index if hasattr(self, '_step_index') else self.index_for_timestep(timestep)
    sigma_next = self.sigmas[step_index + 1] if step_index + 1 < len(self.sigmas) else 0
    if sigma_next == 0:
        # Final step: return denoised (ODE for last step to avoid torchsde crash)
        from diffusers.utils.outputs import BaseOutput
        model_output = self.convert_model_output(model_output, sample=sample)
        self.model_outputs.append(model_output)
        self._step_index += 1
        class Out(BaseOutput):
            prev_sample: torch.Tensor
        return Out(prev_sample=model_output)
    return original_step(self, model_output, timestep, sample, **kwargs)

pipe.scheduler.step = patched_step.__get__(pipe.scheduler)

SEED = 123456789
gen = torch.Generator("cpu").manual_seed(SEED)

print("Generating: 'a steady clean saw wave, c3' (3.0s, 20 steps, CFG=7.0)")
result = pipe(
    prompt="a steady clean saw wave, c3",
    negative_prompt=None,
    audio_end_in_s=3.0,
    num_inference_steps=20,
    guidance_scale=7.0,
    generator=gen,
)

audio = result.audios[0].cpu().numpy()
print(f"Output: {audio.shape}, range=[{audio.min():.4f}, {audio.max():.4f}]")

import soundfile as sf
sr = 44100
trim = int(3.0 * sr)
if audio.shape[-1] > trim:
    audio = audio[..., :trim]
out = Path("~/Desktop/reference_pipeline.wav").expanduser()
sf.write(str(out), audio.T, sr)
print(f"Saved: {out}")
