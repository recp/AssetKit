/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_bitwise.h"
#include <limits.h>

uint32_t
ak_bitw_ctz(uint32_t x) {
#if __has_builtin(__builtin_ctz)
  return __builtin_ctz(x);
#else
  uint32_t i;

  i = 0;
  while ((x >>= 1) > 0)
    i++;
#endif
}

uint32_t
ak_bitw_clz(uint32_t x) {
  #if __has_builtin(__builtin_clz)
    return __builtin_clz(x);
  #else
    return sizeof(uint32_t) * CHAR_BIT - ak_bitw_ctz(x) + 1;
  #endif
}
