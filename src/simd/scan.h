/*
 Copyright (C) 2023 Recep Aslantas.
 SPDX-License-Identifier: MIT
 */

#ifndef assetkit_simd_scan_h
#define assetkit_simd_scan_h

#include "intrin.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(AK_SIMD_ARM64)
#  include <arm_neon.h>
#endif

#if defined(AK_SIMD_X86)
#  include <immintrin.h>
#endif

AK_INLINE
uint32_t
ak_simd_ctz32(uint32_t v) {
#if defined(__GNUC__) || defined(__clang__)
  return (uint32_t)__builtin_ctz(v);
#else
  uint32_t n;

  n = 0;
  while ((v & 1u) == 0u) {
    v >>= 1u;
    n++;
  }

  return n;
#endif
}

AK_INLINE
uint16_t
ak_simd_ascii_sep_mask16(const char *p, bool lineOnly) {
#if defined(AK_SIMD_ARM64)
  uint8x16_t v, m;
  uint8_t   lanes[16];
  uint16_t  mask;
  int       i;

  v = vld1q_u8((const uint8_t *)p);
  m = vceqq_u8(v, vdupq_n_u8((uint8_t)' '));
  m = vorrq_u8(m, vceqq_u8(v, vdupq_n_u8((uint8_t)'\t')));
  m = vorrq_u8(m, vceqq_u8(v, vdupq_n_u8((uint8_t)'\f')));
  m = vorrq_u8(m, vceqq_u8(v, vdupq_n_u8((uint8_t)'\v')));

  if (!lineOnly) {
    m = vorrq_u8(m, vceqq_u8(v, vdupq_n_u8((uint8_t)'\n')));
    m = vorrq_u8(m, vceqq_u8(v, vdupq_n_u8((uint8_t)'\r')));
  }

  if (vminvq_u8(m) == 0xffu)
    return 0xffffu;
  if (vmaxvq_u8(m) == 0u)
    return 0u;

  vst1q_u8(lanes, m);

  mask = 0;
  for (i = 0; i < 16; i++) {
    if (lanes[i])
      mask |= (uint16_t)(1u << i);
  }

  return mask;
#elif defined(AK_SIMD_X86)
  __m128i v, m;

  v = _mm_loadu_si128((const __m128i *)p);
  m = _mm_cmpeq_epi8(v, _mm_set1_epi8(' '));
  m = _mm_or_si128(m, _mm_cmpeq_epi8(v, _mm_set1_epi8('\t')));
  m = _mm_or_si128(m, _mm_cmpeq_epi8(v, _mm_set1_epi8('\f')));
  m = _mm_or_si128(m, _mm_cmpeq_epi8(v, _mm_set1_epi8('\v')));

  if (!lineOnly) {
    m = _mm_or_si128(m, _mm_cmpeq_epi8(v, _mm_set1_epi8('\n')));
    m = _mm_or_si128(m, _mm_cmpeq_epi8(v, _mm_set1_epi8('\r')));
  }

  return (uint16_t)_mm_movemask_epi8(m);
#else
  (void)p;
  (void)lineOnly;
  return 0;
#endif
}

AK_INLINE
char *
ak_simd_skip_ascii_sep(char *p, char *end, bool lineOnly) {
#if defined(AK_SIMD_ARM64) || defined(AK_SIMD_X86)
  while ((size_t)(end - p) >= 16) {
    uint16_t mask;

    mask = ak_simd_ascii_sep_mask16(p, lineOnly);
    if (mask == 0xffffu) {
      p += 16;
      continue;
    }

    return p + ak_simd_ctz32((uint32_t)(~mask & 0xffffu));
  }
#else
  (void)lineOnly;
#endif

  return p;
}

#endif /* assetkit_simd_scan_h */
