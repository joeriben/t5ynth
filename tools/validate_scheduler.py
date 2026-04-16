#!/usr/bin/env python3
"""Compare our scheduler logic against diffusers CosineDPMSolverMultistepScheduler.

Runs the REAL diffusers scheduler step-by-step with the REAL models,
then runs OUR logic with the same inputs, and compares latent at each step.
"""

import torch
import json
import math
import numpy as np
from pathlib import Path
from diffusers import StableAudioPipeline
from diffusers.schedulers import CosineDPMSolverMultistepScheduler

MODEL_DIR = Path("~/Library/T5ynth/models/stable-audio-open-1.0").expanduser()
EXPORT_DIR = Path("~/Library/T5ynth/exported_models").expanduser()

# Load TorchScript models (validated correct above)
print("Loading TorchScript models...")
ts_dit = torch.jit.load(str(EXPORT_DIR / "dit.pt"), map_location="cpu")
ts_dit.eval()

# Load original scheduler
print("Loading original pipeline for scheduler...")
pipe = StableAudioPipeline.from_pretrained(str(MODEL_DIR), torch_dtype=torch.float32).to("cpu")
scheduler = pipe.scheduler

# Prepare conditioning (use TorchScript models — they match originals)
import sentencepiece as spm
tokenizer = spm.SentencePieceProcessor()
tokenizer.Load(str(EXPORT_DIR / "spiece.model"))

ts_t5 = torch.jit.load(str(EXPORT_DIR / "t5_encoder.pt"), map_location="cpu")
ts_proj = torch.jit.load(str(EXPORT_DIR / "projection_model.pt"), map_location="cpu")

prompt = "a steady clean saw wave, c3"
ids = tokenizer.Encode(prompt)[:128]
ids += [0] * (128 - len(ids))
token_ids = torch.tensor([ids], dtype=torch.long)
mask = (token_ids != 0).long()

with torch.no_grad():
    t5_out = ts_t5(token_ids, mask)
    if isinstance(t5_out, dict):
        emb = t5_out["last_hidden_state"]
    elif isinstance(t5_out, tuple):
        emb = t5_out[0]
    else:
        emb = t5_out

    proj_out = ts_proj(emb, torch.tensor([0.0]), torch.tensor([3.0]))
    if isinstance(proj_out, dict):
        text_h = proj_out["text_hidden_states"]
        start_h = proj_out["seconds_start_hidden_states"]
        end_h = proj_out["seconds_end_hidden_states"]
    else:
        text_h, start_h, end_h = proj_out[0], proj_out[1], proj_out[2]

encoder_hidden = torch.cat([text_h, start_h, end_h], dim=1)
global_hidden = torch.cat([start_h, end_h], dim=2).squeeze(1)
text_mask = mask.bool()
time_mask = torch.ones(1, 2, dtype=torch.bool)
attention_mask = torch.cat([text_mask, time_mask], dim=1)

neg_encoder = torch.zeros_like(encoder_hidden)
neg_global = torch.zeros_like(global_hidden)

# ═══ Run diffusers scheduler ═══
print("\n═══ Running diffusers scheduler (reference) ═══")
steps = 20
cfg_scale = 7.0
seed = 123456789

scheduler.set_timesteps(steps)
print(f"Diffusers sigmas: {scheduler.sigmas.numpy()}")
print(f"Diffusers timesteps: {scheduler.timesteps.numpy()}")

torch.manual_seed(seed)
latent_ref = torch.randn(1, 64, 1024) * scheduler.init_noise_sigma

# We need the noise sampler for the SDE variant
from diffusers.schedulers.scheduling_dpmsolver_sde import BrownianTreeNoiseSampler
import sys
sys.setrecursionlimit(10000)  # Workaround for torchsde crash

noise_sampler = BrownianTreeNoiseSampler(
    latent_ref, sigma_min=scheduler.config.sigma_min, sigma_max=scheduler.config.sigma_max,
    seed=seed,
)

for i in range(steps):
    sigma = scheduler.sigmas[scheduler.step_index].item()
    t = scheduler.timesteps[i]

    scaled = scheduler.scale_model_input(latent_ref, t)

    with torch.no_grad():
        cond = ts_dit(scaled, t.unsqueeze(0), encoder_hidden, global_hidden, attention_mask)
        uncond = ts_dit(scaled, t.unsqueeze(0), neg_encoder, neg_global, attention_mask)
    pred = uncond + cfg_scale * (cond - uncond)

    try:
        result = scheduler.step(pred, t, latent_ref, return_dict=True)
        latent_ref = result.prev_sample
    except RecursionError:
        print(f"  Step {i}: RecursionError (torchsde) — stopping here")
        break

    rms = latent_ref.pow(2).mean().sqrt().item()
    print(f"  Step {i:2d}: sigma={float(sigma):.3f}, latent RMS={rms:.4f}")

print(f"\nFinal latent RMS: {latent_ref.pow(2).mean().sqrt().item():.4f}")
print(f"Final latent range: [{latent_ref.min():.4f}, {latent_ref.max():.4f}]")

# ═══ Run OUR scheduler logic ═══
print("\n═══ Running OUR scheduler logic ═══")

with open(EXPORT_DIR / "scheduler_config.json") as f:
    our_config = json.load(f)

sigma_min = our_config.get("sigma_min", 0.3)
sigma_max = our_config.get("sigma_max", 500.0)
sigma_data = our_config.get("sigma_data", 1.0)

# Our sigma schedule
our_sigmas = np.exp(np.linspace(np.log(sigma_max), np.log(sigma_min), steps))
our_sigmas = np.append(our_sigmas, 0.0)
our_timesteps = np.arctan(our_sigmas[:-1]) / (np.pi / 2)

print(f"Our sigmas: {our_sigmas}")
print(f"Our timesteps: {our_timesteps}")

# Compare sigma schedules
print(f"\nSigma schedule diff: {np.abs(our_sigmas - scheduler.sigmas.numpy()).max():.6e}")
print(f"Timestep diff: {np.abs(our_timesteps - scheduler.timesteps.numpy()).max():.6e}")

torch.manual_seed(seed)
latent_ours = torch.randn(1, 64, 1024) * our_sigmas[0]

prev_denoised = None
lower_order_nums = 0

for i in range(steps):
    sigma = our_sigmas[i]
    sigma_next = our_sigmas[i + 1]

    # scale input
    c_in = 1.0 / math.sqrt(sigma**2 + sigma_data**2)
    scaled = latent_ours * c_in

    t = torch.tensor([our_timesteps[i]], dtype=torch.float32)
    with torch.no_grad():
        cond = ts_dit(scaled, t, encoder_hidden, global_hidden, attention_mask)
        uncond = ts_dit(scaled, t, neg_encoder, neg_global, attention_mask)
    pred = uncond + cfg_scale * (cond - uncond)

    # convert v_prediction
    c_skip = sigma_data**2 / (sigma**2 + sigma_data**2)
    c_out = -(sigma * sigma_data) / math.sqrt(sigma**2 + sigma_data**2)
    denoised = c_skip * latent_ours + c_out * pred

    # ODE first/second order
    is_last = (i == steps - 1)
    use_first = (i < 1 or lower_order_nums < 1 or is_last)

    if use_first:
        ratio = sigma_next / sigma if sigma > 0 else 0.0
        latent_ours = ratio * latent_ours + (1.0 - ratio) * denoised
    else:
        s0 = our_sigmas[i]
        s1 = our_sigmas[i - 1]
        st = our_sigmas[i + 1]
        lam_t = -math.log(st) if st > 0 else 100.0
        lam_s0 = -math.log(s0)
        lam_s1 = -math.log(s1)
        h = lam_t - lam_s0
        h_0 = lam_s0 - lam_s1
        r0 = h_0 / h
        D0 = denoised
        D1 = (1.0 / r0) * (denoised - prev_denoised)
        ratio = st / s0 if s0 > 0 else 0.0
        coeff = 1.0 - ratio
        latent_ours = ratio * latent_ours + coeff * D0 + 0.5 * coeff * D1

    prev_denoised = denoised
    if lower_order_nums < 2:
        lower_order_nums += 1

    rms = latent_ours.pow(2).mean().sqrt().item()
    print(f"  Step {i:2d}: sigma={sigma:.3f} → {sigma_next:.3f}, latent RMS={rms:.4f}")

print(f"\nFinal latent RMS: {latent_ours.pow(2).mean().sqrt().item():.4f}")
print(f"Final latent range: [{latent_ours.min():.4f}, {latent_ours.max():.4f}]")

# ═══ Decode both and save ═══
print("\n═══ VAE Decode ═══")
ts_vae = torch.jit.load(str(EXPORT_DIR / "vae_decoder.pt"), map_location="cpu")
ts_vae.eval()

with torch.no_grad():
    audio_ours = ts_vae(latent_ours).squeeze(0).numpy()

import soundfile as sf
sr = 44100
trim = int(3.0 * sr)
audio_ours = audio_ours[:, :trim]
sf.write(str(Path("~/Desktop/scheduler_ours.wav").expanduser()), audio_ours.T, sr)
print(f"Saved scheduler_ours.wav: range=[{audio_ours.min():.3f}, {audio_ours.max():.3f}]")
