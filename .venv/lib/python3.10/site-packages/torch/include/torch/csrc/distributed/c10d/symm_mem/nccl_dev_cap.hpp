#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#if USE_NCCL

#include <nccl.h>
#include <torch/csrc/cuda/nccl.h>

#if NCCL_VERSION_CODE >= NCCL_VERSION(2, 27, 0)
#define NCCL_HAS_SYMMEM_SUPPORT
#endif

#if NCCL_VERSION_CODE >= NCCL_VERSION(2, 28, 0)
#define NCCL_HAS_SYMMEM_DEVICE_SUPPORT
#include <nccl_device.h>
#endif

#if NCCL_VERSION_CODE >= NCCL_VERSION(2, 29, 0)
#define NCCL_HAS_ONE_SIDED_API
#endif
#endif // USE_NCCL

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
