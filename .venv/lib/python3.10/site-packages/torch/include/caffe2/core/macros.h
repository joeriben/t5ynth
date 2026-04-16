#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
// Automatically generated header file for caffe2 macros. These
// macros are used to build the Caffe2 binary, and if you are
// building a dependent library, they will need to be set as well
// for your program to link correctly.

#pragma once

#define CAFFE2_BUILD_SHARED_LIBS
/* #undef CAFFE2_FORCE_FALLBACK_CUDA_MPI */
/* #undef CAFFE2_HAS_MKL_DNN */
/* #undef CAFFE2_HAS_MKL_SGEMM_PACK */
/* #undef CAFFE2_PERF_WITH_AVX */
/* #undef CAFFE2_PERF_WITH_AVX2 */
/* #undef CAFFE2_THREADPOOL_MAIN_IMBALANCE */
/* #undef CAFFE2_THREADPOOL_STATS */
/* #undef CAFFE2_USE_ACCELERATE */
/* #undef CAFFE2_USE_CUDNN */
#define CAFFE2_USE_EIGEN_FOR_BLAS
/* #undef CAFFE2_USE_FBCODE */
/* #undef CAFFE2_USE_GOOGLE_GLOG */
/* #undef CAFFE2_USE_LITE_PROTO */
/* #undef CAFFE2_USE_MKL */
/* #undef USE_MKLDNN */
/* #undef CAFFE2_USE_NVTX */
/* #undef CAFFE2_USE_ITT */

#ifndef EIGEN_MPL2_ONLY
#define EIGEN_MPL2_ONLY
#endif

// Useful build settings that are recorded in the compiled binary
// torch.__config__.show()
#define CAFFE2_BUILD_STRINGS { \
  {"TORCH_VERSION", "2.11.0"}, \
  {"CXX_COMPILER", "/usr/bin/c++"}, \
  {"CXX_FLAGS", " -fvisibility-inlines-hidden -DUSE_PTHREADPOOL -DNDEBUG -DUSE_KINETO -DLIBKINETO_NOCUPTI -DLIBKINETO_NOROCTRACER -DLIBKINETO_NOXPUPTI=ON -DUSE_PYTORCH_QNNPACK -DAT_BUILD_ARM_VEC256_WITH_SLEEF -DUSE_XNNPACK -DUSE_PYTORCH_METAL_EXPORT -DSYMBOLICATE_MOBILE_DEBUG_HANDLE -DUSE_COREML_DELEGATE -O2 -fPIC -DC10_NODEPRECATED -Wall -Wextra -Werror=return-type -Werror=non-virtual-dtor -Werror=braced-scalar-init -Werror=range-loop-construct -Werror=bool-operation -Wnarrowing -Wno-missing-field-initializers -Wno-unknown-pragmas -Wno-unused-parameter -Wno-strict-overflow -Wno-strict-aliasing -Wvla-extension -Wsuggest-override -Wnewline-eof -Winconsistent-missing-override -Winconsistent-missing-destructor-override -Wno-pass-failed -Wno-error=old-style-cast -Wconstant-conversion -Qunused-arguments -faligned-new -fno-math-errno -fno-trapping-math -Werror=format -DUSE_MPS -Wno-missing-braces"}, \
  {"BUILD_TYPE", "Release"}, \
  {"BLAS_INFO", "accelerate"}, \
  {"LAPACK_INFO", "accelerate"}, \
  {"USE_CUDA", "OFF"}, \
  {"USE_ROCM", "OFF"}, \
  {"CUDA_VERSION", ""}, \
  {"ROCM_VERSION", ""}, \
  {"USE_CUDNN", "OFF"}, \
  {"COMMIT_SHA", "70d99e998b4955e0049d13a98d77ae1b14db1f45"}, \
  {"CUDNN_VERSION", ""}, \
  {"USE_NCCL", "OFF"}, \
  {"USE_MPI", "OFF"}, \
  {"USE_GFLAGS", "OFF"}, \
  {"USE_GLOG", "OFF"}, \
  {"USE_GLOO", "ON"}, \
  {"USE_NNPACK", "ON"}, \
  {"USE_OPENMP", "ON"}, \
  {"FORCE_FALLBACK_CUDA_MPI", ""}, \
  {"HAS_MKL_DNN", ""}, \
  {"HAS_MKL_SGEMM_PACK", ""}, \
  {"PERF_WITH_AVX", ""}, \
  {"PERF_WITH_AVX2", ""}, \
  {"USE_ACCELERATE", ""}, \
  {"USE_EIGEN_FOR_BLAS", "ON"}, \
  {"USE_LITE_PROTO", ""}, \
  {"USE_MKL", "OFF"}, \
  {"USE_MKLDNN", "OFF"}, \
  {"USE_NVTX", ""}, \
  {"USE_ITT", ""}, \
  {"USE_ROCM_KERNEL_ASSERT", "OFF"}, \
  {"USE_CUSPARSELT", "OFF"}, \
  {"USE_XPU", "OFF"}, \
  {"USE_XCCL", "OFF"}, \
}

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
