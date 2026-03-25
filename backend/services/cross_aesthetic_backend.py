"""
Crossmodal Lab Backend -- Latent Audio Synth

Replaces the old Strategy A/B/C approach with direct T5 embedding manipulation
for Stable Audio, analogous to the Surrealizer for images.

Operations:
- Interpolation: LERP between embedding A and B
- Extrapolation: alpha > 1.0 or < 0.0
- Magnitude: global scaling of embeddings
- Noise injection: Gaussian noise on embedding space
- Dimension shift: per-dimension offsets

Usage:
    backend = get_cross_aesthetic_backend()
    result = await backend.synth(
        prompt_a="ocean waves",
        prompt_b="piano melody",
        alpha=0.5,
    )
"""

import logging
import time
from pathlib import Path
from typing import Optional, Dict, Any, List, Tuple

logger = logging.getLogger(__name__)

# =============================================================================
# PCA Axes -- data-driven directions of maximum variance in T5 conditioning space
# Fitted on 392K prompt embeddings. Each component = 768d unit direction.
# =============================================================================

PCA_AXES_PATH = Path(__file__).resolve().parent.parent / "data" / "pca_components.pt"
_pca_components = None  # Lazy-loaded [N, 768] tensor

# Interpretive labels derived from corpus projection analysis (392K prompts).
# Each label pair names what the positive/negative pole of the PC represents.
PCA_LABELS: List[Dict[str, str]] = [
    {"pole_a": "Natural",       "pole_b": "Synthetic"},       # PC1: birds/wind vs plugins/devices
    {"pole_a": "Sonic",         "pole_b": "Physical"},         # PC2: metallic/synth vs boats/bags
    {"pole_a": "Musical",       "pole_b": "Elemental"},        # PC3: modular synth/kabuki vs wind/water
    {"pole_a": "Textured",      "pole_b": "Tonal"},            # PC4: crinkling/sanding vs bells/chants
    {"pole_a": "Social",        "pole_b": "Mechanical"},       # PC5: people/park vs engines/hinges
    {"pole_a": "Vocal",         "pole_b": "Atmospheric"},      # PC6: speech/voice vs wind/rain
    {"pole_a": "Machine",       "pole_b": "Biological"},       # PC7: motors/vehicles vs owls/birds
    {"pole_a": "Intimate",      "pole_b": "Crowd"},            # PC8: scratching/contact vs applause
    {"pole_a": "Material",      "pole_b": "Abstract"},         # PC9: wood/dice/chisel vs noise/tone
    {"pole_a": "Motion",        "pole_b": "Expression"},       # PC10: vehicles/speed vs shouts/applause
]


def _get_pca_components():
    """Lazy-load PCA component vectors."""
    global _pca_components
    if _pca_components is None:
        if PCA_AXES_PATH.exists():
            import torch
            _pca_components = torch.load(PCA_AXES_PATH, map_location="cpu")
            logger.info(f"[CROSSMODAL] Loaded PCA components: {_pca_components.shape}")
        else:
            logger.warning(f"[CROSSMODAL] PCA components not found at {PCA_AXES_PATH}")
    return _pca_components


# =============================================================================
# Semantic Axes -- 21 validated + 1 experimental
# Each axis = two text poles in T5 embedding space. LERP between them.
# d = cosine distance between encoded poles (higher = more distinct).
# =============================================================================
SEMANTIC_AXES: Dict[str, Dict[str, Any]] = {
    'tonal_noisy':          {'pole_a': 'sound tonal',       'pole_b': 'sound noisy',        'level': 'perceptual',   'd': 4.806},
    'rhythmic_sustained':   {'pole_a': 'sound rhythmic',    'pole_b': 'sound sustained',    'level': 'perceptual',   'd': 2.604},
    'ceremonial_everyday':  {'pole_a': 'ceremonial music',  'pole_b': 'everyday music',     'level': 'cultural',     'd': 2.391},
    'improvised_composed':  {'pole_a': 'improvised music',  'pole_b': 'composed music',     'level': 'cultural',     'd': 2.109},
    'acoustic_electronic':  {'pole_a': 'acoustic music',    'pole_b': 'electronic music',   'level': 'cultural',     'd': 2.013},
    'music_noise':          {'pole_a': 'music',             'pole_b': 'noise',              'level': 'critical',     'd': 2.058},
    'complex_simple':       {'pole_a': 'complex music',     'pole_b': 'simple music',       'level': 'critical',     'd': 1.914},
    'sacred_secular':       {'pole_a': 'sacred music',      'pole_b': 'secular music',      'level': 'cultural',     'd': 1.756},
    'refined_raw':          {'pole_a': 'refined music',     'pole_b': 'raw music',          'level': 'critical',     'd': 1.735},
    'solo_ensemble':        {'pole_a': 'solo music',        'pole_b': 'ensemble music',     'level': 'cultural',     'd': 1.593},
    'bright_dark':          {'pole_a': 'sound bright',      'pole_b': 'sound dark',         'level': 'perceptual',   'd': 1.281},
    'traditional_modern':   {'pole_a': 'traditional music', 'pole_b': 'modern music',       'level': 'cultural',     'd': 1.213},
    'beautiful_ugly':       {'pole_a': 'beautiful music',   'pole_b': 'ugly music',         'level': 'critical',     'd': 1.097},
    'loud_quiet':           {'pole_a': 'sound loud',        'pole_b': 'sound quiet',        'level': 'perceptual',   'd': 1.024},
    'professional_amateur': {'pole_a': 'professional music','pole_b': 'amateur music',      'level': 'cultural',     'd': 0.965},
    'smooth_harsh':         {'pole_a': 'sound smooth',      'pole_b': 'sound harsh',        'level': 'perceptual',   'd': 0.810},
    'fast_slow':            {'pole_a': 'sound fast',        'pole_b': 'sound slow',         'level': 'perceptual',   'd': 0.800},
    'close_distant':        {'pole_a': 'sound close',       'pole_b': 'sound distant',      'level': 'perceptual',   'd': 0.758},
    'dense_sparse':         {'pole_a': 'sound dense',       'pole_b': 'sound sparse',       'level': 'perceptual',   'd': 0.735},
    'vocal_instrumental':   {'pole_a': 'vocal music',       'pole_b': 'instrumental music', 'level': 'cultural',     'd': 0.673},
    'authentic_fusion':     {'pole_a': 'authentic music',   'pole_b': 'fusion music',       'level': 'critical',     'd': 0.649},
    'music_soundscape':     {'pole_a': 'music',             'pole_b': 'soundscape',         'level': 'experimental', 'd': None},
}

NEUTRAL_PROMPT = 'sound'


class CrossmodalLabBackend:
    """
    Crossmodal Lab backend for Latent Audio Synth.

    Manipulates Stable Audio's T5 conditioning space (768d) directly,
    enabling interpolation, extrapolation, magnitude scaling, noise injection,
    and per-dimension shifts between text embeddings.
    """

    def __init__(self):
        self._axis_cache: Dict[str, Any] = {}  # {axis_name: {emb_a, mask_a, emb_b, mask_b}}
        self._neutral_cache: Optional[Tuple[Any, Any]] = None  # (emb, mask)
        logger.info("[CROSSMODAL] Initialized CrossmodalLabBackend")

    async def synth(
        self,
        prompt_a: str,
        prompt_b: Optional[str] = None,
        alpha: float = 0.5,
        magnitude: float = 1.0,
        noise_sigma: float = 0.0,
        dimension_offsets: Optional[Dict[int, float]] = None,
        axes: Optional[Dict[str, float]] = None,
        duration_seconds: float = 1.0,
        start_position: float = 0.0,
        steps: int = 20,
        cfg_scale: float = 3.5,
        seed: int = -1,
    ) -> Optional[Dict[str, Any]]:
        """
        Latent Audio Synth: manipulate T5 embeddings and generate audio.

        Pipeline:
        1. Encode prompt_a (and optionally prompt_b) via T5
        2. Apply A/B interpolation, magnitude, noise
        3. Apply semantic axis deltas on top (if any)
        4. Apply per-dimension offsets
        5. Generate audio

        Args:
            prompt_a: Base text prompt
            prompt_b: Optional second prompt for interpolation/extrapolation
            alpha: Interpolation factor (0=A, 1=B, >1 or <0 = extrapolation)
            magnitude: Global embedding scale (0.1-5.0)
            noise_sigma: Gaussian noise strength (0=none, 0.5=moderate)
            dimension_offsets: Per-dimension offset values {dim_idx: offset}
            axes: Semantic axes {axis_name: -1.0 to 1.0}, 0 = no effect
            duration_seconds: Audio duration
            start_position: Position within a virtual sound (0.0-1.0). Controls
                seconds_start/seconds_total conditioning to suppress attack transients.
                0.0 = beginning (default), 0.5 = middle (sustained material).
            steps: Inference steps
            cfg_scale: Classifier-free guidance scale
            seed: Random seed (-1 = random)

        Returns:
            Dict with audio_bytes, embedding_stats, axis_contributions, generation_time_ms, seed
        """
        import torch
        import random as random_mod

        start_time = time.time()

        try:
            from services.stable_audio_backend import get_stable_audio_backend
            stable_audio = get_stable_audio_backend()

            # Step 1: Encode prompt(s)
            emb_a, mask_a = await stable_audio.encode_prompt(prompt_a)
            if emb_a is None:
                return None

            if prompt_b:
                emb_b, mask_b = await stable_audio.encode_prompt(prompt_b)
                if emb_b is None:
                    return None
            else:
                emb_b = None
                mask_b = None

            # Step 2: A/B interpolation, magnitude, noise (dimension_offsets handled later)
            result_emb = self._manipulate_embedding(
                emb_a, emb_b, alpha, magnitude, noise_sigma, None, seed
            )

            # Use mask from prompt_a (or combined mask if both prompts)
            result_mask = mask_a
            if mask_b is not None:
                result_mask = torch.maximum(mask_a, mask_b)

            # Step 3: Apply semantic axis deltas on top
            axis_contributions: List[Dict[str, Any]] = []
            if axes:
                result_emb, axis_contributions, result_mask = await self._apply_axes(
                    result_emb, result_mask, axes
                )

            # Step 4: Per-dimension offsets (last, so user can fine-tune everything)
            if dimension_offsets:
                for dim_idx, offset_value in dimension_offsets.items():
                    dim_idx = int(dim_idx)
                    if 0 <= dim_idx < result_emb.shape[-1]:
                        result_emb[:, :, dim_idx] += offset_value

            # Step 5: Compute embedding stats for visualization
            stats = self._compute_stats(result_emb, emb_a, emb_b)

            # Step 6: Generate audio
            duration_seconds = max(0.1, min(duration_seconds, 47.0))
            start_position = max(0.0, min(start_position, 1.0))

            # Compute conditioning: virtual sound length = duration / (1 - start_position)
            # so that the generated window starts at the requested position.
            # At start_position=0: seconds_start=0, seconds_end=duration (normal)
            # At start_position=0.5: model thinks it's in the middle -> sustained material
            if start_position > 0.0 and start_position < 1.0:
                virtual_total = duration_seconds / (1.0 - start_position)
                seconds_start = start_position * virtual_total
                seconds_end = seconds_start + duration_seconds
            else:
                seconds_start = 0.0
                seconds_end = duration_seconds

            if seed == -1:
                seed = random_mod.randint(0, 2**32 - 1)

            audio_bytes = await stable_audio.generate_from_embeddings(
                prompt_embeds=result_emb,
                attention_mask=result_mask,
                seconds_start=seconds_start,
                seconds_end=seconds_end,
                steps=steps,
                cfg_scale=cfg_scale,
                seed=seed,
            )

            if audio_bytes is None:
                return None

            elapsed_ms = int((time.time() - start_time) * 1000)

            logger.info(
                f"[CROSSMODAL] Synth complete: alpha={alpha}, magnitude={magnitude}, "
                f"noise={noise_sigma}, axes={len(axes) if axes else 0}, "
                f"duration={duration_seconds}s, start_pos={start_position:.0%}, time={elapsed_ms}ms"
            )

            return {
                "audio_bytes": audio_bytes,
                "embedding_stats": stats,
                "axis_contributions": axis_contributions,
                "generation_time_ms": elapsed_ms,
                "seed": seed,
            }

        except Exception as e:
            logger.error(f"[CROSSMODAL] Synth error: {e}")
            import traceback
            traceback.print_exc()
            return None

    async def _apply_axes(
        self,
        result_emb,
        result_mask,
        axes: Dict[str, float],
    ) -> Tuple[Any, List[Dict[str, Any]], Any]:
        """Apply semantic or PCA axis deltas on top of an existing embedding.

        Axes prefixed 'pc' (e.g. 'pc1', 'pc12') are PCA axes -- direct 768d
        directions from the fitted principal components. All others are
        semantic axes with text-pole encoding.
        """
        import torch

        # Split axes into semantic and PCA
        semantic_axes = {}
        pca_axes = {}
        for name, t in axes.items():
            if name.startswith("pc") and name[2:].isdigit():
                pca_axes[name] = t
            else:
                semantic_axes[name] = t

        # Handle semantic axes (requires neutral + pole caching)
        if semantic_axes:
            await self._ensure_neutral_cached()
            for axis_name in semantic_axes:
                await self._ensure_axis_cached(axis_name)

        neutral_emb, _ = self._neutral_cache if self._neutral_cache else (None, None)
        axis_deltas: Dict[str, Any] = {}

        for axis_name, t in semantic_axes.items():
            cache = self._axis_cache[axis_name]
            dir_a = cache['emb_a'] - neutral_emb
            dir_b = cache['emb_b'] - neutral_emb

            # t in [-1, 1]: 0 = no effect, +1 = full pole_a, -1 = full pole_b
            if t >= 0:
                delta = t * dir_a
            else:
                delta = (-t) * dir_b
            result_emb = result_emb + delta

            axis_deltas[axis_name] = delta.detach().squeeze(0).mean(dim=0)
            result_mask = torch.maximum(result_mask, cache['mask_a'])
            result_mask = torch.maximum(result_mask, cache['mask_b'])

        # Handle PCA axes (direct direction vectors, no encoding needed)
        pca = _get_pca_components()
        if pca is not None and pca_axes:
            pca_device = result_emb.device
            for axis_name, t in pca_axes.items():
                idx = int(axis_name[2:]) - 1  # "pc1" -> index 0
                if 0 <= idx < pca.shape[0]:
                    direction = pca[idx].to(pca_device)  # [768]
                    # Scale: t * direction broadcast across all sequence positions
                    delta = t * direction.unsqueeze(0).unsqueeze(0)  # [1, 1, 768]
                    result_emb = result_emb + delta
                    axis_deltas[axis_name] = (t * direction)

        return result_emb, self._compute_axis_contributions(axis_deltas), result_mask

    def _manipulate_embedding(
        self,
        emb_a,  # [1, seq, 768]
        emb_b,  # [1, seq, 768] or None
        alpha: float,
        magnitude: float,
        noise_sigma: float,
        dimension_offsets: Optional[Dict[int, float]],
        seed: int,
    ):
        """Apply embedding manipulation operations."""
        import torch

        # Interpolation / extrapolation
        if emb_b is not None:
            result = (1.0 - alpha) * emb_a + alpha * emb_b

            # Renormalize after extrapolation: preserve direction, clamp magnitude
            # to midpoint norm so the model stays in-distribution.
            # Only activates when alpha is outside [0, 1].
            if alpha < 0.0 or alpha > 1.0:
                midpoint = 0.5 * emb_a + 0.5 * emb_b
                ref_norm = midpoint.norm()
                result_norm = result.norm()
                if result_norm > 1e-8:
                    result = result * (ref_norm / result_norm)
        else:
            result = emb_a.clone()

        # Magnitude scaling
        if magnitude != 1.0:
            result = result * magnitude

        # Noise injection
        if noise_sigma > 0.0:
            generator = torch.Generator(device=result.device)
            if seed != -1:
                generator.manual_seed(seed + 1)  # Offset from generation seed
            noise = torch.randn_like(result, generator=generator) * noise_sigma
            result = result + noise

        # Per-dimension offsets
        if dimension_offsets:
            for dim_idx, offset_value in dimension_offsets.items():
                dim_idx = int(dim_idx)
                if 0 <= dim_idx < result.shape[-1]:
                    result[:, :, dim_idx] += offset_value

        return result

    # =====================================================================
    # Semantic Axes -- multi-axis synth
    # =====================================================================

    async def _ensure_neutral_cached(self):
        """Lazy-encode the neutral prompt on first use."""
        if self._neutral_cache is not None:
            return
        from services.stable_audio_backend import get_stable_audio_backend
        sa = get_stable_audio_backend()
        emb, mask = await sa.encode_prompt(NEUTRAL_PROMPT)
        if emb is None:
            raise RuntimeError("Failed to encode neutral prompt")
        self._neutral_cache = (emb, mask)

    async def _ensure_axis_cached(self, axis_name: str):
        """Lazy-encode one axis's poles on first use."""
        if axis_name in self._axis_cache:
            return
        axis = SEMANTIC_AXES.get(axis_name)
        if not axis:
            raise ValueError(f"Unknown axis: {axis_name}")

        from services.stable_audio_backend import get_stable_audio_backend
        sa = get_stable_audio_backend()

        emb_a, mask_a = await sa.encode_prompt(axis['pole_a'])
        emb_b, mask_b = await sa.encode_prompt(axis['pole_b'])
        if emb_a is None or emb_b is None:
            raise RuntimeError(f"Failed to encode axis {axis_name}")

        self._axis_cache[axis_name] = {
            'emb_a': emb_a, 'mask_a': mask_a,
            'emb_b': emb_b, 'mask_b': mask_b,
        }
        logger.info(f"[CROSSMODAL] Cached axis: {axis_name}")

    async def multi_axis_synth(
        self,
        axes: Dict[str, float],
        base_prompt: Optional[str] = None,
        dimension_offsets: Optional[Dict[int, float]] = None,
        duration_seconds: float = 1.0,
        start_position: float = 0.0,
        steps: int = 20,
        cfg_scale: float = 3.5,
        seed: int = -1,
    ) -> Optional[Dict[str, Any]]:
        """
        Multi-axis semantic synth.

        Computes: result = base + sum(t_i * (pole_a_i - neutral) + (1-t_i) * (pole_b_i - neutral))
        where base = encode(base_prompt) if given, else neutral embedding.
        Axis directions are always relative to neutral, so they act as semantic modifiers.

        Args:
            axes: {axis_name: -1.0 to 1.0} -- 0 = no effect, +1 = full pole_a, -1 = full pole_b
            base_prompt: Optional text prompt as starting point (if None, uses neutral "sound")
            dimension_offsets: Per-dimension offset values {dim_idx: offset}
            duration_seconds: Audio duration
            start_position: Position within a virtual sound (0.0-1.0). Suppresses attack transients.
            steps: Inference steps
            cfg_scale: CFG scale
            seed: Random seed (-1 = random)

        Returns:
            Dict with audio_bytes, embedding_stats, axis_contributions, generation_time_ms, seed
        """
        import torch
        import random as random_mod

        start_time = time.time()

        try:
            from services.stable_audio_backend import get_stable_audio_backend
            stable_audio = get_stable_audio_backend()

            # Split axes into semantic and PCA
            semantic_axes = {}
            pca_axes_local = {}
            for name, t in axes.items():
                if name.startswith("pc") and name[2:].isdigit():
                    pca_axes_local[name] = t
                else:
                    semantic_axes[name] = t

            # Cache neutral + requested semantic axes
            await self._ensure_neutral_cached()
            for axis_name in semantic_axes:
                await self._ensure_axis_cached(axis_name)

            neutral_emb, neutral_mask = self._neutral_cache

            # Base embedding: user prompt if given, else neutral
            if base_prompt and base_prompt.strip():
                base_emb, base_mask = await stable_audio.encode_prompt(base_prompt.strip())
                if base_emb is None:
                    return None
                result = base_emb.clone()
                result_mask = base_mask.clone()
                logger.info(f"[CROSSMODAL] Multi-axis base: \"{base_prompt.strip()[:50]}\"")
            else:
                result = neutral_emb.clone()
                result_mask = neutral_mask.clone()

            # Track per-dimension contributions for drawbar coloring
            axis_deltas: Dict[str, Any] = {}

            # Apply semantic axes
            for axis_name, t in semantic_axes.items():
                cache = self._axis_cache[axis_name]
                emb_a = cache['emb_a']
                emb_b = cache['emb_b']

                dir_a = emb_a - neutral_emb
                dir_b = emb_b - neutral_emb

                if t >= 0:
                    delta = t * dir_a
                else:
                    delta = (-t) * dir_b
                result = result + delta

                axis_deltas[axis_name] = delta.detach().squeeze(0).mean(dim=0)
                result_mask = torch.maximum(result_mask, cache['mask_a'])
                result_mask = torch.maximum(result_mask, cache['mask_b'])

            # Apply PCA axes
            pca = _get_pca_components()
            if pca is not None and pca_axes_local:
                for axis_name, t in pca_axes_local.items():
                    idx = int(axis_name[2:]) - 1
                    if 0 <= idx < pca.shape[0]:
                        direction = pca[idx].to(result.device)
                        delta = t * direction.unsqueeze(0).unsqueeze(0)
                        result = result + delta
                        axis_deltas[axis_name] = (t * direction)

            # Apply dimension offsets on top
            if dimension_offsets:
                for dim_idx, offset_value in dimension_offsets.items():
                    dim_idx = int(dim_idx)
                    if 0 <= dim_idx < result.shape[-1]:
                        result[:, :, dim_idx] += offset_value

            # Compute axis contributions for drawbar coloring
            axis_contributions = self._compute_axis_contributions(axis_deltas)

            # Compute embedding stats
            stats = self._compute_stats(result)

            # Generate audio
            duration_seconds = max(0.1, min(duration_seconds, 47.0))
            start_position = max(0.0, min(start_position, 1.0))

            if start_position > 0.0 and start_position < 1.0:
                virtual_total = duration_seconds / (1.0 - start_position)
                seconds_start = start_position * virtual_total
                seconds_end = seconds_start + duration_seconds
            else:
                seconds_start = 0.0
                seconds_end = duration_seconds

            if seed == -1:
                seed = random_mod.randint(0, 2**32 - 1)

            audio_bytes = await stable_audio.generate_from_embeddings(
                prompt_embeds=result,
                attention_mask=result_mask,
                seconds_start=seconds_start,
                seconds_end=seconds_end,
                steps=steps,
                cfg_scale=cfg_scale,
                seed=seed,
            )

            if audio_bytes is None:
                return None

            elapsed_ms = int((time.time() - start_time) * 1000)

            logger.info(
                f"[CROSSMODAL] Multi-axis synth: {len(axes)} axes, "
                f"duration={duration_seconds}s, time={elapsed_ms}ms"
            )

            return {
                "audio_bytes": audio_bytes,
                "embedding_stats": stats,
                "axis_contributions": axis_contributions,
                "generation_time_ms": elapsed_ms,
                "seed": seed,
            }

        except Exception as e:
            logger.error(f"[CROSSMODAL] Multi-axis synth error: {e}")
            import traceback
            traceback.print_exc()
            return None

    def _compute_axis_contributions(
        self, axis_deltas: Dict[str, Any]
    ) -> List[Dict[str, Any]]:
        """
        Compute per-dimension axis contribution for drawbar coloring.

        Returns list of 768 entries:
        [{"dim": 0, "top_axis": "tonal_noisy", "contribution": 0.42,
          "all": {"tonal_noisy": 0.42, "bright_dark": -0.11}}, ...]
        """
        import torch

        if not axis_deltas:
            return []

        n_dims = 768
        contributions = []

        for d in range(n_dims):
            all_contribs = {}
            max_abs = 0.0
            top_axis = ""

            for axis_name, delta_tensor in axis_deltas.items():
                val = float(delta_tensor[d].item())
                all_contribs[axis_name] = round(val, 4)
                if abs(val) > max_abs:
                    max_abs = abs(val)
                    top_axis = axis_name

            contributions.append({
                "dim": d,
                "top_axis": top_axis,
                "contribution": round(max_abs, 4),
                "all": all_contribs,
            })

        return contributions

    def get_available_axes(self) -> List[Dict[str, Any]]:
        """Return axis metadata for frontend dropdown population."""
        result = [
            {
                "name": name,
                "pole_a": axis["pole_a"],
                "pole_b": axis["pole_b"],
                "level": axis["level"],
                "d": axis["d"],
                "type": "semantic",
            }
            for name, axis in SEMANTIC_AXES.items()
        ]

        # Add PCA axes if available
        pca = _get_pca_components()
        if pca is not None:
            for i in range(pca.shape[0]):
                if i < len(PCA_LABELS):
                    label = PCA_LABELS[i]
                    pole_a = label["pole_a"]
                    pole_b = label["pole_b"]
                else:
                    pole_a = f"PC{i + 1}+"
                    pole_b = f"PC{i + 1}-"
                result.append({
                    "name": f"pc{i + 1}",
                    "pole_a": pole_a,
                    "pole_b": pole_b,
                    "level": "pca",
                    "d": None,
                    "type": "pca",
                })

        return result

    def _compute_stats(self, embedding, emb_a=None, emb_b=None) -> Dict[str, Any]:
        """Compute embedding statistics for frontend visualization.

        When emb_b is provided, all_activations are sorted by prompt A/B difference
        (feature probing). Otherwise falls back to activation magnitude sorting.
        """
        import torch

        emb = embedding.detach()
        mean_val = emb.mean().item()
        std_val = emb.std().item()

        # Top dimensions by absolute mean activation (backward compat)
        dim_means_abs = emb.squeeze(0).mean(dim=0).abs()  # [768]
        top_k = min(10, dim_means_abs.shape[0])
        top_vals, top_indices = dim_means_abs.topk(top_k)

        # All 768 activations with diff-based or magnitude-based sorting
        dim_means_signed = emb.squeeze(0).mean(dim=0)  # [768], signed

        if emb_b is not None and emb_a is not None:
            # Feature probing: sort by prompt A/B difference
            diff = emb_a.detach().squeeze(0).mean(dim=0) - emb_b.detach().squeeze(0).mean(dim=0)
            sort_order = diff.abs().argsort(descending=True)
            sort_mode = "diff"
        else:
            # Single-prompt fallback: sort by activation magnitude
            sort_order = dim_means_signed.abs().argsort(descending=True)
            sort_mode = "magnitude"

        all_activations = [
            {"dim": int(idx), "value": round(float(dim_means_signed[idx].item()), 4)}
            for idx in sort_order.tolist()
        ]

        result = {
            "mean": round(mean_val, 4),
            "std": round(std_val, 4),
            "top_dimensions": [
                {"dim": int(idx), "value": round(float(val), 4)}
                for idx, val in zip(top_indices.tolist(), top_vals.tolist())
            ],
            "all_activations": all_activations,
            "sort_mode": sort_mode,
        }

        # Per-dimension A/B reference values for relative mode in Dimension Explorer
        if emb_a is not None:
            a_means = emb_a.detach().squeeze(0).mean(dim=0)  # [768]
            result["emb_a_values"] = {
                int(idx): round(float(a_means[idx].item()), 4)
                for idx in sort_order.tolist()
            }
        if emb_b is not None:
            b_means = emb_b.detach().squeeze(0).mean(dim=0)  # [768]
            result["emb_b_values"] = {
                int(idx): round(float(b_means[idx].item()), 4)
                for idx in sort_order.tolist()
            }

        return result


# =============================================================================
# Singleton
# =============================================================================

_backend: Optional[CrossmodalLabBackend] = None


def get_cross_aesthetic_backend() -> CrossmodalLabBackend:
    global _backend
    if _backend is None:
        _backend = CrossmodalLabBackend()
    return _backend


def reset_cross_aesthetic_backend():
    global _backend
    _backend = None
