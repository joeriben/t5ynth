#!/usr/bin/env bash
set -euo pipefail

echo "T5ynth Linux preflight"
echo

if [[ -x /opt/T5ynth/T5ynth ]]; then
    echo "[ok] standalone binary: /opt/T5ynth/T5ynth"
else
    echo "[!!] missing standalone binary: /opt/T5ynth/T5ynth"
fi

if [[ -x /opt/T5ynth/backend/pipe_inference ]]; then
    echo "[ok] bundled backend: /opt/T5ynth/backend/pipe_inference"
else
    echo "[!!] missing bundled backend: /opt/T5ynth/backend/pipe_inference"
fi

if [[ -f /opt/T5ynth/backend/bundle.env ]]; then
    echo "[ok] bundle metadata: /opt/T5ynth/backend/bundle.env"
    # shellcheck disable=SC1091
    source /opt/T5ynth/backend/bundle.env
    echo "     bundle-id=${BUNDLE_ID:-unknown} arch=${BUNDLE_ARCH:-unknown} distro-family=${BUNDLE_DISTRO_FAMILY:-unknown}"
else
    echo "[warn] bundle metadata missing: /opt/T5ynth/backend/bundle.env"
fi

echo
if command -v nvidia-smi >/dev/null 2>&1; then
    echo "GPU:"
    nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv,noheader || true
else
    echo "[warn] nvidia-smi not found. T5ynth may still run, but CUDA availability is not confirmed."
fi

echo
echo "Runtime library check:"
ldd /opt/T5ynth/T5ynth || true
