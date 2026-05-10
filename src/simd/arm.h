/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef assetkit_simd_arm_h
#define assetkit_simd_arm_h

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM)
#  if defined(_MSC_VER) && (defined(_M_ARM64) || defined(_M_ARM64EC))
#    include <arm64_neon.h>
#  else
#    include <arm_neon.h>
#  endif
#  define AK_SIMD_ARM 1
#  if defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
#    define AK_SIMD_ARM64 1
#  endif
#endif

#endif /* assetkit_simd_arm_h */
