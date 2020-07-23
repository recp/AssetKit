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
#include <ctype.h>

#if _MSC_VER
# if (defined(_M_IX86) || defined(_M_X64))
#  define ARCH_X86
# endif
# if (defined(_M_AMD64) || defined(_M_X64))
#  define ARCH_X86_64
# endif
#else
# ifdef __i386__
#  define ARCH_X86
# endif
# ifdef __x86_64__
#  define ARCH_X86_64
# endif
#endif

AK_INLINE
uint16_t
bswapu16(uint16_t val) {
#if !defined(_MSC_VER)
  return (uint16_t)((val << 8) | (val >> 8));
#else
  return _byteswap_ushort(val);
#endif
}

AK_INLINE
uint32_t
bswapu32(uint32_t val) {
  #if defined(__llvm__)
    return __builtin_bswap32(val);
  #elif defined(ARCH_X86)
  # if !defined(_MSC_VER)
    __asm__ ("bswap   %0" : "+r" (val));
    return val;
  # else
    return _byteswap_ulong(val);
  # endif
  #else
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
  #endif
}

AK_INLINE
uint64_t
bswapu64(uint64_t val) {
  #if defined(__llvm__)
    return __builtin_bswap64(val);
  #elif defined(ARCH_X86_64)
  # if !defined(_MSC_VER)
    __asm__ ("bswap   %0" : "+r" (val));
    return val;
  # else
    return _byteswap_uint64(val);
  # endif
  #elif defined(ARCH_X86)
    __asm__ ("bswap   %%eax\n\t"
             "bswap   %%edx\n\t"
             "xchgl   %%eax, %%edx"
             : "+A" (val));
    return val;
  #else
    /*
    return ((((val) & 0xff00000000000000ull) >> 56)
          | (((val) & 0x00ff000000000000ull) >> 40)
          | (((val) & 0x0000ff0000000000ull) >> 24)
          | (((val) & 0x000000ff00000000ull) >> 8)
          | (((val) & 0x00000000ff000000ull) << 8)
          | (((val) & 0x0000000000ff0000ull) << 24)
          | (((val) & 0x000000000000ff00ull) << 40)
          | (((val) & 0x00000000000000ffull) << 56));
    */
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL)
        | ((val >> 8) & 0x00FF00FF00FF00FFULL);
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL)
        | ((val >> 16) & 0x0000FFFF0000FFFFULL);
    return (val << 32) | (val >> 32);
  #endif
}

/* helper for little endian */

/* 16-bit */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
# define le_16(X, DATA)                                                       \
  do {                                                                        \
    memcpy(&X, DATA, 2);                                                      \
    DATA = (char *)DATA + 2;                                                  \
  } while (0)
#else
# define le_16(X, DATA)                                                       \
  do {                                                                        \
    memcpy(&X, DATA, 2);                                                      \
    X    = bswapu16((uint16_t)X);                                             \
    DATA = (char *)DATA + 2;                                                  \
  } while (0)
#endif

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
    X    = bswapu32((uint32_t)X);                                             \
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
    X    = bswapu64((uint64_t)X);                                             \
    DATA = (char *)DATA + 8;                                                  \
  } while (0)
#endif

/* helper for big endian */

/* 16-bit */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define be_16(X, DATA)                                                       \
  do {                                                                        \
    memcpy(&X, DATA, 2);                                                      \
    DATA = (char *)DATA + 2;                                                  \
  } while (0)
#else
# define be_16(X, DATA)                                                       \
  do {                                                                        \
    memcpy(&X, DATA, 2);                                                      \
    X    = bswapu16((uint16_t)X);                                             \
    DATA = (char *)DATA + 2;                                                  \
  } while (0)
#endif

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
    X    = bswapu32((uint32_t)X);                                             \
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
    X    = bswapu64((uint64_t)X);                                             \
    DATA = (char *)DATA + 8;                                                  \
  } while (0)
#endif

#define memcpy_endian64(isLittleEndian, X, DATA)                              \
  do {                                                                        \
    if (isLittleEndian) {                                                     \
      le_64(X, DATA);                                                         \
    } else {                                                                  \
      be_64(X, DATA);                                                         \
    }                                                                         \
  } while (0)
 
#define memcpy_endian32(isLittleEndian, X, DATA)                              \
  do {                                                                        \
    if (isLittleEndian) {                                                     \
      le_32(X, DATA);                                                         \
    } else {                                                                  \
      be_32(X, DATA);                                                         \
    }                                                                         \
  } while (0)
 
#define memcpy_endian16(isLittleEndian, X, DATA)                              \
  do {                                                                        \
    if (isLittleEndian) {                                                     \
      le_16(X, DATA);                                                         \
    } else {                                                                  \
      be_16(X, DATA);                                                         \
    }                                                                         \
  } while (0)

#endif /* endian_h */
