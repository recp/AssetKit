/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_bitwise_h
#define ak_bitwise_h

#include "../common.h"
#include <limits.h>

AK_INLINE
uint32_t
ak_bitw_ctz(uint32_t x) {
#if __has_builtin(__builtin_ctz)
	return __builtin_ctz(x);
#else
	uint32_t i;

	i = 0;
	while ((x >>= 1) > 0)
		i++;
	return i;
#endif
}

AK_INLINE
uint32_t
ak_bitw_ffs(uint32_t x) {
#if __has_builtin(__builtin_ffs)
	return __builtin_ffs(x);
#else
	return ak_bitw_ctz(x) + 1;
#endif
}

AK_INLINE
uint32_t
ak_bitw_clz(uint32_t x) {
#if __has_builtin(__builtin_clz)
	return __builtin_clz(x);
#else
	return sizeof(uint32_t) * CHAR_BIT - ak_bitw_ffs(x);
#endif
}

#endif /* ak_bitwise_h */
