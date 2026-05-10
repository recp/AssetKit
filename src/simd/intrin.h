/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef assetkit_simd_intrin_h
#define assetkit_simd_intrin_h

#if defined(__AVX2__) || defined(__AVX__) || defined(__SSSE3__) || defined(__SSE2__) || defined(_M_X64) || defined(_M_IX86)
#  include "x86.h"
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM)
#  include "arm.h"
#endif

#if defined(__wasm__) && defined(__wasm_simd128__)
#  include "wasm.h"
#endif

#if defined(AK_SIMD_X86) || defined(AK_SIMD_ARM) || defined(AK_SIMD_WASM)
#  define AK_SIMD 1
#endif

#endif /* assetkit_simd_intrin_h */
