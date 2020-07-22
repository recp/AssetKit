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

#ifndef endian_h
#define endian_h

#include "common.h"

AK_INLINE
uint32_t
bswapu32(uint32_t val) {
  /*
   val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
   return (val << 16) | (val >> 16);
   */
  #if defined(__llvm__)
    return __builtin_bswap32(val);
  #else
    ASM ("bswap   %0" : "+r" (val));
    return val;
  #endif
}

AK_INLINE
uint64_t
bswapu64(uint64_t val) {
  #if defined(__llvm__)
    return __builtin_bswap64(val);
  #else
    ASM ("bswap   %0" : "+r" (val));
    return val;
  #endif
}

/* helper for little endian */

/* 32-bit */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
# define le_32(X, DATA)                                                       \
  do {                                                                        \
    memcpy(&X, DATA, 4);                                                      \
    DATA = (char *)DATA + 4;                                                  \
  } while (0)
#else
# define le_32(X, DATA)                                                       \
  do {                                                                        \
    memcpy(&X, DATA, 4);                                                      \
    X    = bswapu32(X);                                                       \
    DATA = (char *)DATA + 4;                                                  \
  } while (0)
#endif

/* 64-bit */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
# define le_64(X, DATA)                                                       \
  do {                                                                        \
    memcpy(&X, DATA, 8);                                                      \
    DATA = (char *)DATA + 8;                                                  \
  } while (0)
#else
# define le_64(X, DATA)                                                       \
  do {                                                                        \
    memcpy(&X, DATA, 8);                                                      \
    X    = bswapu64(X);                                                       \
    DATA = (char *)DATA + 8;                                                  \
  } while (0)
#endif

/* helper for big endian */

/* 32-bit */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define be_32(X, DATA)                                                       \
  do {                                                                        \
    memcpy(&X, DATA, 4);                                                      \
    DATA = (char *)DATA + 4;                                                  \
  } while (0)
#else
# define be_32(X, DATA)                                                       \
  do {                                                                        \
    memcpy(&X, DATA, 4);                                                      \
    X    = bswapu32(X);                                                       \
    DATA = (char *)DATA + 4;                                                  \
  } while (0)
#endif

/* 64-bit */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define be_64(X, DATA)                                                       \
  do {                                                                        \
    memcpy(&X, DATA, 8);                                                      \
    DATA = (char *)DATA + 8;                                                  \
  } while (0)
#else
# define be_64(X, DATA)                                                       \
  do {                                                                        \
    memcpy(&X, DATA, 8);                                                      \
    X    = bswapu64(X);                                                       \
    DATA = (char *)DATA + 8;                                                  \
  } while (0)
#endif

#endif /* endian_h */
