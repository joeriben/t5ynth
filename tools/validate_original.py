#!/usr/bin/env python3
"""Run original StableAudioPipeline (not TorchScript) for comparison.

Usage:
    python tools/validate_original.py \
        --model-dir ~/Library/T5ynth/models/stable-audio-open-1.0 \
        --prompt "a steady clean saw wave, c3" \
        --seed 123456789
"""

import argparse
import math
import torch
import soundfile as sf
from pathlib import Path
from diffusers import StableAudioPipeline


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--model-dir", type=str, default="~/Library/T5ynth/models/stable-audio-open-1.0")
    parser.add_argument("--prompt", type=str, default="a steady clean saw wave, c3")
    parser.add_argument("--negative-prompt", type=str, default="")
    parser.add_argument("--duration", type=float, default=3.0)
    parser.add_argument("--steps", type=int, default=20)
    parser.add_argument("--cfg", type=float, default=7.0)
    parser.add_argument("--seed", type=int, default=123456789)
    parser.add_argument("--output", type=str, default="~/Desktop/original_validate.wav")
    args = parser.parse_args()

    model_dir = Path(args.model_dir).expanduser()
    output_path = Path(args.output).expanduser()

    print(f"Loading pipeline from {model_dir}...")
    pipe = StableAudioPipeline.from_pretrained(str(model_dir), torch_dtype=torch.float32)
    pipe = pipe.to("cpu")
    print("Pipeline loaded.")

    generator = torch.Generator("cpu").manual_seed(args.seed)

    # Force ODE variant to avoid torchsde Brownian tree crash
    pipe.scheduler.algorithm_type = "dpmsolver++"

    print(f"\nGenerating (ODE): '{args.prompt}' ({args.duration}s, {args.steps} steps, CFG={args.cfg}, seed={args.seed})")
    result = pipe(
        prompt=args.prompt,
        negative_prompt=args.negative_prompt if args.negative_prompt else None,
        audio_end_in_s=args.duration,
        num_inference_steps=args.steps,
        guidance_scale=args.cfg,
        generator=generator,
    )

    audio = result.audios[0]  # [channels, samples] or [samples]
    if audio.ndim == 1:
        audio = audio.unsqueeze(0)

    print(f"Output shape: {audio.shape}, range=[{audio.min():.3f}, {audio.max():.3f}]")

    # Convert to numpy
    audio_np = audio.cpu().numpy()
    sr = pipe.vae.config.sample_rate if hasattr(pipe.vae.config, 'sample_rate') else 44100

    # Trim
    requested = int(math.ceil(args.duration * sr))
    if audio_np.shape[-1] > requested:
        audio_np = audio_np[..., :requested]

    sf.write(str(output_path), audio_np.T, sr)
    print(f"Saved: {output_path} ({audio_np.shape[-1]} samples, {sr}Hz)")


if __name__ == "__main__":
    main()
