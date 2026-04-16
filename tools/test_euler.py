#!/usr/bin/env python3
"""Test if Euler scheduler (no SDE noise needed) works with Stable Audio."""

import torch, math
from pathlib import Path

MODEL_DIR = Path("~/Library/T5ynth/models/stable-audio-open-1.0").expanduser()
EXPORT_DIR = Path("~/Library/T5ynth/exported_models").expanduser()

print("Loading TorchScript models...")
import sentencepiece as spm
tok = spm.SentencePieceProcessor(); tok.Load(str(EXPORT_DIR / "spiece.model"))
ts_t5 = torch.jit.load(str(EXPORT_DIR / "t5_encoder.pt"), map_location="cpu"); ts_t5.eval()
ts_proj = torch.jit.load(str(EXPORT_DIR / "projection_model.pt"), map_location="cpu"); ts_proj.eval()
ts_dit = torch.jit.load(str(EXPORT_DIR / "dit.pt"), map_location="cpu"); ts_dit.eval()
ts_vae = torch.jit.load(str(EXPORT_DIR / "vae_decoder.pt"), map_location="cpu"); ts_vae.eval()

# Conditioning
ids = tok.Encode("a steady clean saw wave, c3")[:128]
ids += [0] * (128 - len(ids))
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
attn = torch.cat([mask.bool(), torch.ones(1, 2, dtype=torch.bool)], dim=1)
neg_enc = torch.zeros_like(enc_h)
neg_glob = torch.zeros_like(glob_h)

STEPS = 20; CFG = 7.0; SEED = 123456789
sigma_min, sigma_max, sigma_data = 0.3, 500.0, 1.0

# Euler discrete sigma schedule (same exponential schedule)
import numpy as np
sigmas = np.exp(np.linspace(np.log(sigma_max), np.log(sigma_min), STEPS))
sigmas = np.append(sigmas, 0.0)
timesteps = np.arctan(sigmas[:-1]) / (np.pi / 2)

torch.manual_seed(SEED)
latent = torch.randn(1, 64, 1024) * sigmas[0]

print(f"\nEuler discrete, {STEPS} steps")
with torch.no_grad():
    for i in range(STEPS):
        sig = sigmas[i]
        sig_next = sigmas[i + 1]

        # Scale input
        c_in = 1.0 / math.sqrt(sig**2 + sigma_data**2)
        scaled = latent * c_in

        t = torch.tensor([timesteps[i]], dtype=torch.float32)
        c = ts_dit(scaled, t, enc_h, glob_h, attn)
        u = ts_dit(scaled, t, neg_enc, neg_glob, attn)
        pred = u + CFG * (c - u)

        # v_prediction → denoised
        c_skip = sigma_data**2 / (sig**2 + sigma_data**2)
        c_out = -(sig * sigma_data) / math.sqrt(sig**2 + sigma_data**2)
        denoised = c_skip * latent + c_out * pred

        # Euler step: x_{t+1} = x_t + (denoised - x_t) * (sigma_next - sigma) / sigma
        # Simplified: x = denoised + (x - denoised) * sigma_next / sigma
        if sig > 0:
            latent = denoised + (latent - denoised) * (sig_next / sig)
        else:
            latent = denoised

        rms = latent.pow(2).mean().sqrt().item()
        print(f"  step {i:2d}: sigma {sig:.3f} → {sig_next:.3f}, RMS={rms:.4f}")

    audio = ts_vae(latent).squeeze(0).numpy()

print(f"\nOutput: {audio.shape}, range=[{audio.min():.4f}, {audio.max():.4f}]")
print(f"RMS: {np.sqrt((audio**2).mean()):.4f}")

import soundfile as sf
trim = int(3.0 * 44100)
audio = audio[:, :trim]
sf.write(str(Path("~/Desktop/euler_test.wav").expanduser()), audio.T, 44100)

# Compare with reference
ref, _ = sf.read('/Users/joerissen/Downloads/synth_raw_123456789-2.wav')
ours = audio.T
minlen = min(len(ref), len(ours))

def windowed_rms(x, win=4410):
    n = len(x) // win
    return [np.sqrt((x[i*win:(i+1)*win, 0]**2).mean()) for i in range(n)]

rms_r = windowed_rms(ref)
rms_o = windowed_rms(ours)
print('Ref  RMS windows: ' + ' '.join(f'{r:.3f}' for r in rms_r[:10]))
print('Ours RMS windows: ' + ' '.join(f'{r:.3f}' for r in rms_o[:10]))
print(f'Zero crossings ref: {np.sum(np.diff(np.sign(ref[:minlen,0])) != 0)}')
print(f'Zero crossings ours: {np.sum(np.diff(np.sign(ours[:minlen,0])) != 0)}')
print(f'\nSaved euler_test.wav')
