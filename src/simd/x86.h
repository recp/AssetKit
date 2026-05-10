/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef assetkit_simd_x86_h
#define assetkit_simd_x86_h

#if defined(__AVX2__)
#  include <immintrin.h>
#  define AK_SIMD_X86 1
#  define AK_SIMD_AVX2 1
#elif defined(__SSSE3__)
#  include <tmmintrin.h>
#  define AK_SIMD_X86 1
#  define AK_SIMD_SSSE3 1
#elif defined(__SSE2__) || defined(_M_X64) || defined(_M_IX86)
#  include <emmintrin.h>
#  define AK_SIMD_X86 1
#  define AK_SIMD_SSE2 1
#endif

#endif /* assetkit_simd_x86_h */
