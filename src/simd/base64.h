/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0.
 *
 * Portions of the ARM64 base64 decode mapping follow the technique used by
 * libbase64:
 * Copyright (c) 2005-2007 Nick Galbreath
 * Copyright (c) 2015-2018 Wojciech Mula
 * Copyright (c) 2016-2017 Matthieu Darbois
 * Copyright (c) 2013-2022 Alfred Klomp
 * BSD 2-Clause license.
 */

#ifndef assetkit_simd_base64_h
#define assetkit_simd_base64_h

#include "intrin.h"

#include <stddef.h>
#include <stdint.h>

#if defined(AK_SIMD_ARM64)
AK_INLINE
int
ak_simd_base64_decode_arm64(const uint8_t * __restrict src,
                            size_t                     len,
                            uint8_t       * __restrict dst,
                            size_t        * __restrict consumed,
                            size_t        * __restrict written) {
  static const uint8_t dec_lut1[] = {
    255U, 255U, 255U, 255U, 255U, 255U, 255U, 255U,
    255U, 255U, 255U, 255U, 255U, 255U, 255U, 255U,
    255U, 255U, 255U, 255U, 255U, 255U, 255U, 255U,
    255U, 255U, 255U, 255U, 255U, 255U, 255U, 255U,
    255U, 255U, 255U, 255U, 255U, 255U, 255U, 255U,
    255U, 255U, 255U,  62U, 255U, 255U, 255U,  63U,
     52U,  53U,  54U,  55U,  56U,  57U,  58U,  59U,
     60U,  61U, 255U, 255U, 255U, 255U, 255U, 255U,
  };
  static const uint8_t dec_lut2[] = {
      0U, 255U,   0U,   1U,   2U,   3U,   4U,   5U,
      6U,   7U,   8U,   9U,  10U,  11U,  12U,  13U,
     14U,  15U,  16U,  17U,  18U,  19U,  20U,  21U,
     22U,  23U,  24U,  25U, 255U, 255U, 255U, 255U,
    255U, 255U,  26U,  27U,  28U,  29U,  30U,  31U,
     32U,  33U,  34U,  35U,  36U,  37U,  38U,  39U,
     40U,  41U,  42U,  43U,  44U,  45U,  46U,  47U,
     48U,  49U,  50U,  51U, 255U, 255U, 255U, 255U,
  };
  const uint8x16x4_t tbl_dec1 = vld1q_u8_x4(dec_lut1);
  const uint8x16x4_t tbl_dec2 = vld1q_u8_x4(dec_lut2);
  const uint8x16_t offset = vdupq_n_u8(63U);
  const uint8x16_t max_valid = vdupq_n_u8(63U);
  size_t i, rounds;

  if (len < 68) {
    *consumed = 0;
    *written = 0;
    return 1;
  }

  rounds = (len - 4) / 64;
  for (i = 0; i < rounds; i++) {
    uint8x16x4_t str, dec1, dec2;
    uint8x16x3_t dec;
    uint8x16_t classified;

    str = vld4q_u8(src);

    dec2.val[0] = vqsubq_u8(str.val[0], offset);
    dec2.val[1] = vqsubq_u8(str.val[1], offset);
    dec2.val[2] = vqsubq_u8(str.val[2], offset);
    dec2.val[3] = vqsubq_u8(str.val[3], offset);

    dec1.val[0] = vqtbl4q_u8(tbl_dec1, str.val[0]);
    dec1.val[1] = vqtbl4q_u8(tbl_dec1, str.val[1]);
    dec1.val[2] = vqtbl4q_u8(tbl_dec1, str.val[2]);
    dec1.val[3] = vqtbl4q_u8(tbl_dec1, str.val[3]);

    dec2.val[0] = vqtbx4q_u8(dec2.val[0], tbl_dec2, dec2.val[0]);
    dec2.val[1] = vqtbx4q_u8(dec2.val[1], tbl_dec2, dec2.val[1]);
    dec2.val[2] = vqtbx4q_u8(dec2.val[2], tbl_dec2, dec2.val[2]);
    dec2.val[3] = vqtbx4q_u8(dec2.val[3], tbl_dec2, dec2.val[3]);

    str.val[0] = vorrq_u8(dec1.val[0], dec2.val[0]);
    str.val[1] = vorrq_u8(dec1.val[1], dec2.val[1]);
    str.val[2] = vorrq_u8(dec1.val[2], dec2.val[2]);
    str.val[3] = vorrq_u8(dec1.val[3], dec2.val[3]);

    classified = vorrq_u8(vorrq_u8(vcgtq_u8(str.val[0], max_valid),
                                   vcgtq_u8(str.val[1], max_valid)),
                          vorrq_u8(vcgtq_u8(str.val[2], max_valid),
                                   vcgtq_u8(str.val[3], max_valid)));
    if (vmaxvq_u8(classified) != 0U)
      return 0;

    dec.val[0] = vorrq_u8(vshlq_n_u8(str.val[0], 2),
                          vshrq_n_u8(str.val[1], 4));
    dec.val[1] = vorrq_u8(vshlq_n_u8(str.val[1], 4),
                          vshrq_n_u8(str.val[2], 2));
    dec.val[2] = vorrq_u8(vshlq_n_u8(str.val[2], 6), str.val[3]);

    vst3q_u8(dst, dec);
    src += 64;
    dst += 48;
  }

  *consumed = rounds * 64;
  *written = rounds * 48;
  return 1;
}
#endif

AK_INLINE
int
ak_simd_base64_decode(const uint8_t * __restrict src,
                      size_t                     len,
                      uint8_t       * __restrict dst,
                      size_t        * __restrict consumed,
                      size_t        * __restrict written) {
  *consumed = 0;
  *written = 0;

#if defined(AK_SIMD_ARM64)
  return ak_simd_base64_decode_arm64(src, len, dst, consumed, written);
#else
  (void)src;
  (void)len;
  (void)dst;
  return 1;
#endif
}

#endif /* assetkit_simd_base64_h */
