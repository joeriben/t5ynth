# Bundling Diffusers + PyTorch with PyInstaller: A Field Report

**Project:** T5ynth — a JUCE-based synthesizer that launches a PyInstaller-bundled Python backend for neural audio generation (Stable Audio, AudioLDM2).

**Architecture:** The JUCE C++ application spawns the bundled Python binary as a subprocess. Communication happens over stdin/stdout pipes (IPC). The Python process loads torch, diffusers, transformers, and torchsde to run inference on GPU (CUDA / MPS).

**Timeline:** April 4–7, 2026 — 17 commits, 10 release candidates (v1.0.0-rc.1 through rc.10).

**Stack:** Python 3.11, PyTorch 2.11, Diffusers 0.33, Transformers 4.x, PyInstaller 6.x, macOS (Apple Silicon MPS), Windows (CUDA 12.4), Linux (CUDA 12.4).

---

## Table of Contents

1. [The Baseline Spec](#1-the-baseline-spec)
2. [Problem 1: CUDA Bundle Size (2 GB → GitHub Limit)](#2-problem-1-cuda-bundle-size)
3. [Problem 2: macOS .app Bundle Structure Lost by CI](#3-problem-2-macos-app-bundle-structure-lost-by-ci)
4. [Problem 3: Unix Permission Bits Stripped by CI Artifacts](#4-problem-3-unix-permission-bits-stripped-by-ci-artifacts)
5. [Problem 4: torch.utils Pulls in unittest — Excluded by PyInstaller](#5-problem-4-torchutils-pulls-in-unittest)
6. [Problem 5: transformers Checks Package Metadata at Import](#6-problem-5-transformers-checks-package-metadata-at-import)
7. [Problem 6: diffusers Uses find_spec() — Fails in PYZ Archives](#7-problem-6-diffusers-uses-find_spec)
8. [Problem 7: The Fork Bomb — multiprocessing.resource_tracker](#8-problem-7-the-fork-bomb)
9. [Problem 8: macOS Gatekeeper Quarantine](#9-problem-8-macos-gatekeeper-quarantine)
10. [Summary: The Final Spec File](#10-summary-the-final-spec-file)
11. [Lessons Learned](#11-lessons-learned)

---

## 1. The Baseline Spec

The initial `.spec` file (April 4) was straightforward:

- `collect_submodules()` for packages with lazy/dynamic imports (torchsde, diffusers pipelines/schedulers, transformers T5, stable_audio_tools, accelerate)
- `collect_data_files()` for JSON/YAML config files that diffusers and transformers bundle
- Standard excludes: matplotlib, tkinter, flask, IPython, unittest, pytest
- UPX disabled — it breaks torch `.dylib` files on macOS
- Single-folder mode (COLLECT), not one-file — required for torch's dynamic library loading

This worked locally. Then CI happened.

---

## 2. Problem 1: CUDA Bundle Size

**RC affected:** rc.1
**Symptom:** GitHub release asset upload failed — archive exceeded the 2 GB limit.
**Root cause:** PyInstaller bundled *all* CUDA 12.4 libraries (~2 GB total), including training-only libraries that inference never uses.

**Solution:** Regex filter on `a.binaries` to strip libraries not needed for inference:

```python
import re as _re
_cuda_exclude = _re.compile(
    r'libnccl|nccl\d'           # multi-GPU communication
    r'|libcufft|cufft\d'        # FFT (training losses only)
    r'|libcusparse|cusparse\d'  # sparse matrices
    r'|libcusolver|cusolver\d'  # dense/sparse solvers
    r'|libnvrtc|nvrtc\d'        # runtime compiler
    r'|libnvJitLink|nvJitLink'  # JIT linker
    r'|libcupti|cupti\d'        # profiling tools
    r'|triton'                  # Triton JIT compiler
)
a.binaries = [b for b in a.binaries if not _cuda_exclude.search(b[0])]
a.datas    = [d for d in a.datas    if 'triton' not in d[0]]
```

**What to keep for inference:** cublas, cublasLt, cudnn, curand, cudart. Saved ~800 MB.

Also switched release archives from gzip to xz for better compression of binary data.

---

## 3. Problem 2: macOS .app Bundle Structure Lost by CI

**RC affected:** rc.2, rc.3
**Symptom:** Downloaded macOS artifact was a flat directory of files, not a `.app` bundle. macOS refused to open it.
**Root cause:** GitHub Actions `upload-artifact@v4` strips directory extensions when you point directly at a `.app`, `.vst3`, or `.component` bundle. It uploads the *contents* without the wrapper directory.

**Solution:** Upload the *parent* directory instead:

```yaml
# WRONG — uploads contents of T5ynth.app as flat files
path: build/T5ynth_artefacts/Release/Standalone/T5ynth.app

# RIGHT — preserves the .app directory structure
path: build/T5ynth_artefacts/Release/Standalone/
```

Same fix applied to `.vst3` and `.component` (AU) bundles.

---

## 4. Problem 3: Unix Permission Bits Stripped by CI Artifacts

**RC affected:** rc.3, rc.4
**Symptom:** macOS `.app` binary had permission `644` instead of `755` after download. "Application is damaged" error.
**Root cause:** `upload-artifact` / `download-artifact` strips Unix executable bits. The release job ran on `ubuntu-latest` and re-packed artifacts that had already lost their permissions.

**Solution:** Create tar archives *on the build machine* (where permissions are correct), then upload the `.tar.xz` as the artifact:

```yaml
# On macOS builder:
- name: Create archives
  run: |
    tar -cJf archives/T5ynth-macOS-Standalone.tar.xz \
      -C build/T5ynth_artefacts/Release/Standalone T5ynth.app

# Release job uses the pre-built archives directly — no re-packing
```

This preserves `+x` bits, symlinks, and the complete bundle structure.

---

## 5. Problem 4: torch.utils Pulls in unittest

**RC affected:** rc.5
**Symptom:** Bundled backend crashed immediately on launch with `ModuleNotFoundError: No module named 'unittest'`.
**Root cause:** PyTorch 2.11 added `torch.utils._config_module`, which imports `unittest` at startup. Our spec file excluded `unittest` as "not needed at runtime" — reasonable for most applications, but torch now depends on it unconditionally.

**Solution:** Remove `unittest` from the excludes list:

```python
excludes=[
    'flask',
    'matplotlib',
    'tkinter',
    # 'unittest',  # required by torch.utils._config_module since torch 2.11
    'pytest',
    'IPython',
    'notebook',
    'jupyter',
],
```

**Takeaway:** PyTorch makes no stability guarantees about internal imports. A new minor version can pull in any stdlib module. Never exclude stdlib modules without testing each PyTorch update.

---

## 6. Problem 5: transformers Checks Package Metadata at Import

**RC affected:** rc.6
**Symptom:** `import transformers` crashed with `importlib.metadata.PackageNotFoundError` for packages like `requests`, `safetensors`, `huggingface_hub`.
**Root cause:** Transformers runs version checks via `importlib.metadata.version('package_name')` at import time. PyInstaller bundles the `.py` files but not the `dist-info` metadata directories unless explicitly told to.

**Solution:** Use `copy_metadata()` for every package that transformers checks:

```python
from PyInstaller.utils.hooks import copy_metadata

datas += copy_metadata('requests')
datas += copy_metadata('transformers')
datas += copy_metadata('torch')
datas += copy_metadata('safetensors')
datas += copy_metadata('huggingface_hub')
datas += copy_metadata('filelock')
datas += copy_metadata('pyyaml')
datas += copy_metadata('regex')
datas += copy_metadata('packaging')
datas += copy_metadata('tqdm')
datas += copy_metadata('numpy')
datas += copy_metadata('torchsde')
datas += copy_metadata('accelerate')
datas += copy_metadata('diffusers')
```

**Takeaway:** Any package that uses `importlib.metadata` at import time needs `copy_metadata()` in the spec. This is not limited to direct dependencies — transitive checks cascade. The error message names the missing package, so it's easy to fix iteratively, but there are many of them.

---

## 7. Problem 6: diffusers Uses find_spec() — Fails in PYZ Archives

**RC affected:** rc.7
**Symptom:** `diffusers` raised errors or silently disabled features because `importlib.util.find_spec('torchsde')` returned `None`, even though torchsde was bundled.
**Root cause:** PyInstaller compresses Python modules into a PYZ archive. The standard `importlib.util.find_spec()` function cannot locate modules inside PYZ archives — it returns `None`. Diffusers uses `find_spec()` to check for optional dependencies (torchsde, accelerate, safetensors) and disables code paths when it returns `None`.

**Solution:** A runtime hook that pre-imports these packages before diffusers gets a chance to check:

```python
# runtime_hook.py
import importlib

for _pkg in ('torchsde', 'safetensors'):
    try:
        importlib.import_module(_pkg)
    except Exception:
        pass
```

Once a module is in `sys.modules`, `find_spec()` finds it there regardless of the PYZ archive. The runtime hook runs before any application code.

**Spec file:**
```python
runtime_hooks=['runtime_hook.py'],
```

---

## 8. Problem 7: The Fork Bomb

**RC affected:** rc.7 → rc.9 (the most dangerous bug)
**Symptom:** Launching the app consumed 10+ GB of RAM per second, spawning hundreds of processes. On one occasion, 190 GB of swap was allocated before the system killed everything.
**Root cause chain:**

1. `import torch` initializes Python's `multiprocessing` module
2. `multiprocessing` starts a `resource_tracker` subprocess
3. The tracker is spawned via `sys.executable` — normally this is `python`
4. **In a PyInstaller bundle, `sys.executable` points to the frozen binary itself**
5. The tracker subprocess re-executes the entire application
6. Which imports torch → starts another tracker → imports torch → ...
7. **Infinite fork bomb**, each copy consuming ~10 GB (the full torch + diffusers bundle in memory)

This only manifests when the PyInstaller binary is launched *as a subprocess* (as in the JUCE → Python IPC architecture). Running it directly from a terminal may not trigger it because the process group and pipe configuration differ.

**Solution:** Disable `resource_tracker` entirely in the runtime hook, before any torch import:

```python
import multiprocessing
multiprocessing.freeze_support()

import multiprocessing.resource_tracker as _rt
_rt.ensure_running = lambda: None
_rt._resource_tracker.ensure_running = lambda: None
_rt.register = lambda *a, **kw: None
_rt.unregister = lambda *a, **kw: None
```

This is safe when the application does not use shared memory objects (`multiprocessing.shared_memory`). T5ynth's inference is single-process — it only uses pipes for IPC with the JUCE host.

**Additional fix on the C++ side:** An atomic `launching_` flag prevents concurrent `launch()` calls, and `shutdown()` now properly reaps the child process (SIGTERM + 3s grace + SIGKILL) instead of fire-and-forget.

**Critical rule:** Never add packages that use `multiprocessing` at import time (e.g. `accelerate`) to the runtime hook's pre-import list. Adding `accelerate` was what originally triggered the fork bomb — it imports multiprocessing during initialization, before the tracker is disabled.

---

## 9. Problem 8: macOS Gatekeeper Quarantine

**Not a PyInstaller bug**, but a distribution problem that affects all unsigned macOS apps.

**Symptom:** "T5ynth.app is damaged and can't be opened."
**Root cause:** macOS adds a `com.apple.quarantine` extended attribute to every file downloaded from the internet. Gatekeeper then requires a valid Apple Developer signature + notarization. Open-source projects without a $99/year Apple Developer certificate cannot pass this check.

**Solution for users:**
```bash
xattr -cr /Applications/T5ynth.app
```

This is documented in a `DO_NOT_FORGET.txt` that ships inside every release archive.

---

## 10. Summary: The Final Spec File

The final working spec file (as of rc.7) differs from the initial version in these ways:

| Area | Initial (rc.0) | Final (rc.7+) |
|------|----------------|---------------|
| `copy_metadata()` | Not used | 14 packages |
| `runtime_hooks` | None | `runtime_hook.py` (find_spec fix + fork bomb fix) |
| `unittest` exclude | Excluded | Kept (torch 2.11 needs it) |
| `requests` hidden import | Missing | Added |
| CUDA library filter | None | Regex excludes ~800 MB of training-only libs |
| CI artifact handling | Direct bundle upload | tar.xz on build machine, upload archive |
| Smoke test | None | Binary launch test + app bundle launch test |

---

## 11. Lessons Learned

### For anyone bundling Diffusers/Transformers/PyTorch with PyInstaller:

1. **`find_spec()` is broken in PYZ archives.** Any library that uses `importlib.util.find_spec()` to detect optional dependencies will think they're missing. Pre-import them in a runtime hook.

2. **`importlib.metadata` needs `copy_metadata()`.** Transformers aggressively checks dependency versions at import time. Without the metadata, you get `PackageNotFoundError` for packages that are physically present in the bundle.

3. **The multiprocessing fork bomb is real.** `sys.executable` in a frozen app points to the frozen binary, not Python. Any code path that spawns via `sys.executable` (resource_tracker, spawn-method workers) will re-execute the entire application. Disable `resource_tracker` if you don't use shared memory. Call `freeze_support()` at the top of your runtime hook.

4. **PyTorch's internal imports change between minor versions.** Never exclude stdlib modules based on assumptions about what torch "should" need. Test every torch update against your excludes list.

5. **CUDA bundles are enormous.** Filter out training-only libraries (nccl, cufft, cusparse, cusolver, nvrtc, triton) to cut 800+ MB. Only cublas, cudnn, curand, and cudart are needed for inference.

6. **GitHub Actions `upload-artifact` destroys macOS bundles.** It strips directory extensions (`.app`, `.vst3`, `.component`) and removes Unix permission bits. Create tar archives on the build machine and upload those instead.

7. **Test the bundle the way users will run it.** The fork bomb only appeared when launched as a subprocess with pipe redirection — not when run directly from a terminal. Your CI smoke test must replicate the actual launch path.

8. **UPX breaks torch on macOS.** Always set `upx=False`. UPX corrupts torch's `.dylib` files on macOS, causing silent crashes or incorrect results.

### The debugging cycle:

Each of these problems only appeared in the *distributed* binary — never during development (where `python pipe_inference.py` runs normally). The typical cycle was:

1. Build succeeds in CI
2. App launches, backend crashes
3. Error message points to a missing module or metadata — but only inside the frozen bundle
4. Fix requires understanding how PyInstaller's module resolution differs from standard Python

This is fundamentally different from typical Python debugging. The errors are not in your code — they're in the interaction between your dependencies' import-time behavior and PyInstaller's module packaging strategy.

---

## Appendix: Commit History

| Date | Commit | Description |
|------|--------|-------------|
| Apr 4, 11:21 | `984dfe4` | Initial PyInstaller spec + bundled binary support |
| Apr 4, 11:30 | `f23778e` | GitHub Actions CI for macOS, Windows, Linux |
| Apr 4, 16:25 | `bab3668` | Fix: libcurl optional on Windows, CPU torch on CI |
| Apr 4, 16:27 | `129be93` | Fix: CUDA torch on Windows/Linux, swap for Linux |
| Apr 4, 20:15 | `bc850fb` | Fix: swap file creation on Linux runner |
| Apr 4, 20:46 | `fd3fa12` | Fix: MSVC includes, Linux deps |
| Apr 5, 00:59 | `d12716d` | Fix: use gh CLI for release creation |
| Apr 5, 17:29 | `b2a6839` | Fix: checkout before download-artifact |
| **Apr 6, 11:18** | **`d64df93`** | **Strip CUDA libs (~800 MB saved) → rc.1** |
| Apr 6, 22:38 | `28feddc` | Fix: preserve .app bundle structure |
| Apr 6, 22:39 | `03f694d` | Fix: preserve .vst3/.component structure → rc.3 |
| **Apr 7, 09:57** | **`8e7d2c7`** | **Preserve Unix permissions (tar on build machine)** |
| Apr 7, 09:58 | `25ea0a8` | Fix: YAML parsing in release notes → rc.4 |
| **Apr 7, 12:08** | **`f31c0bd`** | **Re-include unittest (torch 2.11) → rc.5** |
| **Apr 7, 17:23** | **`299ffef`** | **Add copy_metadata for 14 packages → rc.6** |
| **Apr 7, 18:19** | **`67a3908`** | **Runtime hook for find_spec() → rc.7** |
| **Apr 7, 19:14** | **`60ee4d4`** | **Fork bomb fix (resource_tracker)** |

---

*This document accompanies T5ynth, an open-source GPLv3 neural audio synthesizer. The problems described here are not specific to T5ynth — they apply to any project that bundles PyTorch + Diffusers + Transformers into a standalone application with PyInstaller.*
