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

#ifndef assetkit_3mf_package_source_h
#define assetkit_3mf_package_source_h

#include "../../../include/ak/assetkit.h"
#include "../../common.h"

#include <stdint.h>

#define AK_3MF_PACKAGE_SOURCE_MAGIC 0x334d4650u

typedef struct AK3MFPackageSource {
  const char *path;
  uint32_t    magic;
} AK3MFPackageSource;

static inline
AK3MFPackageSource*
ak_3mf_package_source_get(const AkPrintDocument * __restrict print) {
  AK3MFPackageSource *source;

  source = print ? (AK3MFPackageSource *)print->reserved : NULL;
  if (!source || source->magic != AK_3MF_PACKAGE_SOURCE_MAGIC)
    return NULL;

  return source;
}

static inline
bool
ak_3mf_package_source_set(AkDoc           * __restrict doc,
                          AkPrintDocument * __restrict print,
                          const char      * __restrict path) {
  AK3MFPackageSource *source;
  AkHeap             *heap;

  if (!doc || !print || !path || !*path)
    return false;
  if (print->reserved)
    return ak_3mf_package_source_get(print) != NULL;

  heap = ak_heap_getheap(doc);
  if (!heap)
    return false;

  source = ak_heap_calloc(heap, print, sizeof(*source));
  if (!source)
    return false;

  source->path = ak_heap_strdup(heap, source, path);
  if (!source->path)
    return false;

  source->magic  = AK_3MF_PACKAGE_SOURCE_MAGIC;
  print->reserved = source;
  return true;
}

static inline
bool
ak_3mf_package_source_has_mutated_parts(
                                      const AkPrintDocument * __restrict print) {
  const AkPrintPackagePart *part;

  for (part = print ? print->parts : NULL; part; part = part->next) {
    if ((part->flags & AK_PRINT_PACKAGE_PART_DATA_MUTATED) != 0u)
      return true;
  }

  return false;
}

#endif /* assetkit_3mf_package_source_h */
