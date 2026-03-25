"""
T5ynth Backend Configuration

Standalone config for the Stable Audio + Crossmodal Lab server.
All values overridable via environment variables.
"""

import os
from pathlib import Path

import torch

# --- Device Auto-Detection ---
if torch.cuda.is_available():
    _DEFAULT_DEVICE = "cuda"
    _DEFAULT_DTYPE = "float16"
elif hasattr(torch.backends, 'mps') and torch.backends.mps.is_available():
    _DEFAULT_DEVICE = "mps"
    _DEFAULT_DTYPE = "float32"
else:
    _DEFAULT_DEVICE = "cpu"
    _DEFAULT_DTYPE = "float32"

# --- Server ---
HOST = os.environ.get("T5YNTH_HOST", "127.0.0.1")  # Loopback only — no external access
PORT = int(os.environ.get("T5YNTH_PORT", "17803"))

# --- Model Storage ---
MODEL_DIR = Path(os.environ.get("T5YNTH_MODEL_DIR", str(Path.home() / "t5ynth" / "models")))

# --- Stable Audio ---
STABLE_AUDIO_ENABLED = os.environ.get("STABLE_AUDIO_ENABLED", "true").lower() == "true"
STABLE_AUDIO_MODEL_ID = os.environ.get("STABLE_AUDIO_MODEL_ID", "stabilityai/stable-audio-open-1.0")
STABLE_AUDIO_DEVICE = os.environ.get("STABLE_AUDIO_DEVICE", _DEFAULT_DEVICE)
STABLE_AUDIO_DTYPE = os.environ.get("STABLE_AUDIO_DTYPE", _DEFAULT_DTYPE)
STABLE_AUDIO_LAZY_LOAD = os.environ.get("STABLE_AUDIO_LAZY_LOAD", "true").lower() == "true"
STABLE_AUDIO_MAX_DURATION = 47.55  # seconds (model maximum)
STABLE_AUDIO_SAMPLE_RATE = 44100

# --- Crossmodal Lab ---
CROSS_AESTHETIC_ENABLED = os.environ.get("CROSS_AESTHETIC_ENABLED", "true").lower() == "true"

# --- Disabled backends (not needed for T5ynth MVP) ---
IMAGEBIND_ENABLED = os.environ.get("IMAGEBIND_ENABLED", "false").lower() == "true"
MMAUDIO_ENABLED = os.environ.get("MMAUDIO_ENABLED", "false").lower() == "true"
