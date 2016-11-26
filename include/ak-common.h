/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__common_h
#define __libassetkit__common_h

/* since C99 or compiler ext */
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#if defined(_WIN32)
#  ifdef _assetkit_dll
#    define AK_EXPORT __declspec(dllexport)
#  else
#    define AK_EXPORT __declspec(dllimport)
#  endif
#  define _assetkit_hide
#  define AK_INLINE __forceinline
#  define AK_ALIGN(X) __declspec(align(X))
#else
#  define AK_EXPORT      __attribute__((visibility("default")))
#  define _assetkit_hide __attribute__((visibility("hidden")))
#  define AK_INLINE inline __attribute((always_inline))
#  define AK_ALIGN(X) __attribute((aligned(X)))
#endif

#define AK_ARRAY_LEN(ARR) sizeof(ARR) / sizeof(ARR[0])

typedef int32_t AkEnum;

typedef enum AkResult {
  AK_OK     = 0,
  AK_ERR    = -1, /* UKNOWN ERR */
  AK_EFOUND = -1000,
  AK_ENOMEM = -ENOMEM,
  AK_EPERM  = -EPERM
} AkResult;

#endif /* __libassetkit__common_h */
