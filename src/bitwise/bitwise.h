/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
