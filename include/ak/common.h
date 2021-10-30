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

#ifndef assetkit_common_h
#define assetkit_common_h

/* since C99 or compiler ext */
#include <stdint.h>
#include <stddef.h>
#include <float.h>
#include <stdbool.h>
#include <errno.h>

#ifdef DEBUG
#  include <assert.h>
#  include <stdio.h>
#endif

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#  define AK_WINAPI
#endif

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#  ifdef AK_STATIC
#    define AK_EXPORT
#  elif defined(AK_EXPORTS)
#    define AK_EXPORT __declspec(dllexport)
#  else
#    define AK_EXPORT __declspec(dllimport)
#  endif
#  define AK_HIDE
#else
#  define AK_EXPORT   __attribute__((visibility("default")))
#  define AK_HIDE     __attribute__((visibility("hidden")))
#endif

#if defined(_MSC_VER)
#  define AK_INLINE   __forceinline
#  define AK_ALIGN(X) __declspec(align(X))
#else
#  define AK_ALIGN(X) __attribute((aligned(X)))
#  define AK_INLINE   static inline __attribute((always_inline))
#endif


#ifndef __has_builtin
#  define __has_builtin(x) 0
#endif

#define AK_ARRAY_LEN(ARR) sizeof(ARR) / sizeof(ARR[0])

#define AK_APPEND_FLINK(SRC,LAST,ITEM)                                        \
  do {                                                                        \
    if (LAST)                                                                 \
      LAST->next = ITEM;                                                      \
    else                                                                      \
      SRC = ITEM;                                                             \
    LAST = ITEM;                                                              \
  } while (0)

typedef int32_t AkEnum;

typedef enum AkResult {
  AK_NOOP     = 1,     /* no operation needed */
  AK_OK       = 0,
  AK_ERR      = -1,    /* UKNOWN ERR */
  AK_ETCOMMON = -2,    /* no tcommon found */
  AK_EFOUND   = -1000,
  AK_ENOMEM   = -ENOMEM,
  AK_EPERM    = -EPERM,
  AK_EBADF    = -EBADF /* docoumnt couldn't parsed / loaded */
} AkResult;

typedef struct AkOneWayIterBase {
  struct AkOneWayIterBase *next;
} AkOneWayIterBase;

typedef struct AkTwoWayIterBase {
  struct AkTwoWayIterBase *next;
  struct AkTwoWayIterBase *prev;
} AkTwoWayIterBase;

#endif /* assetkit_common_h */
