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

#ifndef ak_src_common_h
#define ak_src_common_h

#define AK_INTERN

#include "../include/ak/assetkit.h"
#include "../include/ak/options.h"
#include "../include/ak/map.h"
#include "../include/ak/type.h"
#include "../include/ak/url.h"
#include "../include/ak/transform.h"

#include "mem/common.h"
#include "material_legacy.h"

/* AssetKit often runs cglm directly on imported/accessor-backed memory.
   Those buffers are byte-aligned by file layout, not by cglm's 16/32-byte
   mat alignment contract. Keep cglm's SIMD paths, but use unaligned
   loads/stores inside AssetKit sources. */
#ifndef CGLM_ALL_UNALIGNED
#  define CGLM_ALL_UNALIGNED
#endif

#include <cglm/cglm.h>

#include <ds/rb.h>
#include <ds/forward-list.h>
#include <ds/forward-list-sep.h>

#include <stddef.h>
#include <sys/types.h>
#include <string.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef struct AkDocPtrMapSlot {
  void *key;
  void *val;
} AkDocPtrMapSlot;

typedef struct AkDocPtrMap {
  AkDocPtrMapSlot *slots;
  uint32_t         capacity;
  uint32_t         count;
} AkDocPtrMap;

typedef struct AkDocPrivate {
  AkDocPtrMap duplicators;
  AkPrintDocument *print;
} AkDocPrivate;

AK_INLINE
uintptr_t
ak__docPtrHash(const void *key) {
  uintptr_t h;

  h  = (uintptr_t)key >> 4;
  h ^= h >> 16;
  h *= (uintptr_t)0x45d9f3bu;
  h ^= h >> 16;
  return h ? h : 1u;
}

AK_INLINE
AkDocPtrMapSlot*
ak__docPtrMapSlot(AkDocPtrMapSlot * __restrict slots,
                  uint32_t                     capacity,
                  const void      * __restrict key) {
  uintptr_t h, mask;

  h    = ak__docPtrHash(key);
  mask = capacity - 1u;

  for (;;) {
    AkDocPtrMapSlot *slot;

    slot = &slots[h & mask];
    if (!slot->key || slot->key == key)
      return slot;

    h++;
  }
}

AK_INLINE
bool
ak__docPtrMapGrow(AkDocPrivate * __restrict priv,
                  AkDocPtrMap  * __restrict map,
                  uint32_t                  minCapacity) {
  AkHeap           *heap;
  AkDocPtrMapSlot *slots, *oldSlots;
  uint32_t          oldCap, newCap, i;

  heap = ak_heap_getheap(priv);
  if (!heap)
    return false;

  oldCap = map->capacity;
  newCap = oldCap ? oldCap << 1 : 16u;
  while (newCap < minCapacity)
    newCap <<= 1;

  slots = ak_heap_calloc(heap, priv, sizeof(*slots) * newCap);
  if (!slots)
    return false;

  oldSlots = map->slots;
  for (i = 0; i < oldCap; i++) {
    AkDocPtrMapSlot *oldSlot;

    oldSlot = &oldSlots[i];
    if (oldSlot->key)
      *ak__docPtrMapSlot(slots, newCap, oldSlot->key) = *oldSlot;
  }

  map->slots    = slots;
  map->capacity = newCap;

  if (oldSlots)
    ak_free(oldSlots);

  return true;
}

AK_INLINE
AkDocPrivate*
ak__docPrivate(AkDoc * __restrict doc, bool create) {
  AkDocPrivate *priv;
  AkHeap       *heap;

  if (!doc)
    return NULL;

  priv = doc->reserved;
  if (!priv && create) {
    heap = ak_heap_getheap(doc);
    if (!heap)
      return NULL;

    priv          = ak_heap_calloc(heap, doc, sizeof(*priv));
    doc->reserved = priv;
  }

  return priv;
}

AK_INLINE
void*
ak__docDuplicatorFind(AkDoc      * __restrict doc,
                      const void * __restrict key) {
  AkDocPrivate *priv;
  AkDocPtrMap  *map;
  AkDocPtrMapSlot *slot;

  priv = ak__docPrivate(doc, false);
  if (!priv || !key)
    return NULL;

  map = &priv->duplicators;
  if (!map->slots)
    return NULL;

  slot = ak__docPtrMapSlot(map->slots, map->capacity, key);
  return slot->key ? slot->val : NULL;
}

AK_INLINE
bool
ak__docDuplicatorSet(AkDoc      * __restrict doc,
                     const void * __restrict key,
                     void       * __restrict val) {
  AkDocPrivate    *priv;
  AkDocPtrMap     *map;
  AkDocPtrMapSlot *slot;

  if (!key)
    return false;

  priv = ak__docPrivate(doc, true);
  if (!priv)
    return false;

  map = &priv->duplicators;
  if (map->slots) {
    slot = ak__docPtrMapSlot(map->slots, map->capacity, key);
    if (slot->key) {
      slot->val = val;
      return true;
    }
  }

  if (!map->slots
      || (uint64_t)(map->count + 1u) * 4u >= (uint64_t)map->capacity * 3u) {
    if (!ak__docPtrMapGrow(priv, map, map->count + 1u))
      return false;
  }

  slot      = ak__docPtrMapSlot(map->slots, map->capacity, key);
  slot->key = (void *)key;
  slot->val = val;
  map->count++;

  return true;
}

#ifdef _MSC_VER
#  define strncasecmp _strnicmp
#  define strcasecmp  _stricmp
#  define strtok_r    strtok_s
#  define mktemp      _mktemp
#  define ASM __asm
# else
#  define ASM __asm__
#endif

#ifdef __GNUC__
#  define AK_DESTRUCTOR __attribute__((destructor))
#  define AK_CONSTRUCTOR __attribute__((constructor))
#else
#  define AK_DESTRUCTOR
#  define AK_CONSTRUCTOR
#endif

#define AK__UNUSED(X) (void)X

#define AK_LIB_PREPEND(LIB, ITEM, NEXT)                                       \
  do {                                                                        \
    (ITEM)->NEXT = (LIB).first;                                               \
    if (!(LIB).last)                                                          \
      (LIB).last = (ITEM);                                                    \
    (LIB).first = (ITEM);                                                     \
    (LIB).count++;                                                            \
  } while (0)

#define AK_LIB_APPEND(LIB, ITEM, NEXT)                                        \
  do {                                                                        \
    (ITEM)->NEXT = NULL;                                                      \
    if ((LIB).last)                                                           \
      (LIB).last->NEXT = (ITEM);                                              \
    else                                                                      \
      (LIB).first = (ITEM);                                                   \
    (LIB).last = (ITEM);                                                      \
    (LIB).count++;                                                            \
  } while (0)

#define I2P (void *)(intptr_t)

/*!
 * @brief get sign of 32 bit integer as +1 or -1
 *
 * @param X integer value
 */
#define AK_GET_SIGN(X) ((X >> 31) - (-X >> 31))

#define AK_ARRAY_SEP_CHECK (c == ' ' || c == '\n' || c == '\t' \
                              || c == '\r' || c == '\f' || c == '\v')

#define AK_ARRAY_SEPLINE_CHECK (c == ' ' || c == '\t'  || c == '\f' || c == '\v')
#define AK_ARRAY_SPACE_CHECK (c == ' ' || c == '\t' || c == '\f' || c == '\v')
#define AK_ARRAY_NLINE_CHECK (c == '\n' || c == '\r')

typedef struct ak_enumpair {
  const char *key;
  AkEnum      val;
} ak_enumpair;

typedef struct {
  const char * name;
  AkEnum       val;
} dae_enum;

AK_HIDE int
ak_enumpair_cmp(const void * a, const void * b);

AK_HIDE int
ak_enumpair_cmp2(const void * a, const void * b);

AK_HIDE int
ak_enumpair_json_val_cmp(const void * a, const void * b);

AK_EXPORT
int
ak_cmp_str(void * key1, void *key2);

AK_EXPORT
int
ak_cmp_ptr(void *key1, void *key2);

AK_EXPORT
int
ak_cmp_i32(void *key1, void *key2);

AK_EXPORT
int
ak_cmp_vec3(void * key1, void *key2);

AK_EXPORT
int
ak_cmp_ivec3(void *key1, void *key2);

AK_EXPORT
int
ak_cmp_vec4(void *key1, void *key2);

typedef int (*AkCmpFn)(void * key1, void *key2);

#endif /* ak_src_common_h */
