#!/usr/bin/env python3
"""Step-by-step comparison: diffusers scheduler vs our implementation.
Run both with same DiT, same conditioning, same initial noise. Compare latents."""

import torch, json, math, numpy as np, sys
from pathlib import Path

sys.setrecursionlimit(50000)  # Workaround for torchsde

EXPORT_DIR = Path("~/Library/T5ynth/exported_models").expanduser()
MODEL_DIR = Path("~/Library/T5ynth/models/stable-audio-open-1.0").expanduser()

# Load models
print("Loading models...")
from diffusers import StableAudioPipeline
pipe = StableAudioPipeline.from_pretrained(str(MODEL_DIR), torch_dtype=torch.float32).to("cpu")

ts_dit = torch.jit.load(str(EXPORT_DIR / "dit.pt"), map_location="cpu"); ts_dit.eval()
ts_t5 = torch.jit.load(str(EXPORT_DIR / "t5_encoder.pt"), map_location="cpu"); ts_t5.eval()
ts_proj = torch.jit.load(str(EXPORT_DIR / "projection_model.pt"), map_location="cpu"); ts_proj.eval()
ts_vae = torch.jit.load(str(EXPORT_DIR / "vae_decoder.pt"), map_location="cpu"); ts_vae.eval()

# Conditioning (same for both)
import sentencepiece as spm
tok = spm.SentencePieceProcessor(); tok.Load(str(EXPORT_DIR / "spiece.model"))
ids = tok.Encode("a steady clean saw wave, c3")[:128] + [0] * (128 - len(tok.Encode("a steady clean saw wave, c3")[:128]))
token_ids = torch.tensor([ids], dtype=torch.long)
mask = (token_ids != 0).long()

with torch.no_grad():
    t5_out = ts_t5(token_ids, mask)
    emb = t5_out["last_hidden_state"] if isinstance(t5_out, dict) else t5_out[0]
    proj_out = ts_proj(emb, torch.tensor([0.0]), torch.tensor([3.0]))
    if isinstance(proj_out, dict):
        th, sh, eh = proj_out["text_hidden_states"], proj_out["seconds_start_hidden_states"], proj_out["seconds_end_hidden_states"]
    else:
        th, sh, eh = proj_out[0], proj_out[1], proj_out[2]

enc_h = torch.cat([th, sh, eh], dim=1)
glob_h = torch.cat([sh, eh], dim=2).squeeze(1)
attn_mask = torch.cat([mask.bool(), torch.ones(1, 2, dtype=torch.bool)], dim=1)
neg_enc = torch.zeros_like(enc_h)
neg_glob = torch.zeros_like(glob_h)

STEPS = 20; CFG = 7.0; SEED = 123456789

# ═══ Diffusers scheduler ═══
print("\n═══ Diffusers ═══")
sched = pipe.scheduler
sched.set_timesteps(STEPS)

torch.manual_seed(SEED)
latent_d = torch.randn(1, 64, 1024) * sched.init_noise_sigma
print(f"init_noise_sigma = {sched.init_noise_sigma}")
print(f"Initial latent RMS: {latent_d.pow(2).mean().sqrt():.4f}")

from diffusers.schedulers.scheduling_dpmsolver_sde import BrownianTreeNoiseSampler
noise_sampler = BrownianTreeNoiseSampler(latent_d, sigma_min=sched.config.sigma_min,
                                          sigma_max=sched.config.sigma_max, seed=SEED)

for i in range(STEPS):
    t = sched.timesteps[i]
    scaled = sched.scale_model_input(latent_d, t)
    with torch.no_grad():
        c = ts_dit(scaled, t.unsqueeze(0).float(), enc_h, glob_h, attn_mask)
        u = ts_dit(scaled, t.unsqueeze(0).float(), neg_enc, neg_glob, attn_mask)
    pred = u + CFG * (c - u)
    try:
        out = sched.step(pred, t, latent_d)
        latent_d = out.prev_sample
        rms = latent_d.pow(2).mean().sqrt().item()
        print(f"  step {i:2d}: rms={rms:.4f}")
    except RecursionError:
        print(f"  step {i:2d}: CRASH (torchsde)")
        break

# ═══ Our scheduler ═══
print("\n═══ Ours (SDE) ═══")
with open(EXPORT_DIR / "scheduler_config.json") as f: cfg = json.load(f)
s_min, s_max, s_data = cfg["sigma_min"], cfg["sigma_max"], cfg["sigma_data"]
sigmas = np.exp(np.linspace(np.log(s_max), np.log(s_min), STEPS))
sigmas = np.append(sigmas, 0.0)
timesteps = np.arctan(sigmas[:-1]) / (np.pi / 2)

torch.manual_seed(SEED)
latent_o = torch.randn(1, 64, 1024) * sigmas[0]
print(f"sigma_max = {sigmas[0]}")
print(f"Initial latent RMS: {latent_o.pow(2).mean().sqrt():.4f}")

prev_den = None; lo = 0
for i in range(STEPS):
    sig, sig_n = sigmas[i], sigmas[i+1]
    c_in = 1.0 / math.sqrt(sig**2 + s_data**2)
    scaled = latent_o * c_in
    t = torch.tensor([timesteps[i]], dtype=torch.float32)
    with torch.no_grad():
        c = ts_dit(scaled, t, enc_h, glob_h, attn_mask)
        u = ts_dit(scaled, t, neg_enc, neg_glob, attn_mask)
    pred = u + CFG * (c - u)
    c_skip = s_data**2 / (sig**2 + s_data**2)
    c_out = -(sig * s_data) / math.sqrt(sig**2 + s_data**2)
    den = c_skip * latent_o + c_out * pred
    noise = torch.randn_like(latent_o)
    last = (i == STEPS - 1)
    first = (i < 1 or lo < 1 or last)
    if first:
        lam_s = -math.log(sig)
        lam_t = -math.log(sig_n) if sig_n > 0 else 100.0
        h = lam_t - lam_s
        r = (sig_n/sig)*math.exp(-h) if sig_n > 0 else 0.0
        e2h = math.exp(-2*h)
        latent_o = r*latent_o + (1-e2h)*den + sig_n*math.sqrt(max(0, 1-e2h))*noise
    else:
        s0,s1,st = sigmas[i],sigmas[i-1],sigmas[i+1]
        lt = -math.log(st) if st>0 else 100; l0=-math.log(s0); l1=-math.log(s1)
        h=lt-l0; h0=l0-l1; r0=h0/h
        D0=den; D1=(1/r0)*(den-prev_den)
        r=(st/s0)*math.exp(-h) if st>0 else 0.0
        e2h=math.exp(-2*h)
        latent_o = r*latent_o + (1-e2h)*D0 + 0.5*(1-e2h)*D1 + st*math.sqrt(max(0,1-e2h))*noise
    prev_den=den
    if lo<2: lo+=1
    rms = latent_o.pow(2).mean().sqrt().item()
    print(f"  step {i:2d}: rms={rms:.4f}")

# Compare
print(f"\n═══ Comparison after step {min(i, STEPS-1)} ═══")
print(f"Diffusers latent RMS: {latent_d.pow(2).mean().sqrt():.4f}")
print(f"Ours      latent RMS: {latent_o.pow(2).mean().sqrt():.4f}")
d = (latent_d - latent_o).abs()
print(f"Latent diff: max={d.max():.4f}, mean={d.mean():.4f}")
corr = torch.corrcoef(torch.stack([latent_d.flatten(), latent_o.flatten()]))[0,1].item()
print(f"Latent correlation: {corr:.6f}")

# Decode both
with torch.no_grad():
    a_d = ts_vae(latent_d).squeeze(0).numpy()[:, :132300]
    a_o = ts_vae(latent_o).squeeze(0).numpy()[:, :132300]
import soundfile as sf
sf.write(str(Path("~/Desktop/sched_diffusers.wav").expanduser()), a_d.T, 44100)
sf.write(str(Path("~/Desktop/sched_ours.wav").expanduser()), a_o.T, 44100)
print("\nSaved sched_diffusers.wav and sched_ours.wav")
