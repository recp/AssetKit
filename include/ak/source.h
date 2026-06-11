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

#ifndef assetkit_source_h
#define assetkit_source_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

#include <stdint.h>
#include <stdbool.h>
#include "type.h"
#include "url.h"
#include "core-types.h"

/* Modern path: Input -> Accessor -> Buffer. */

/* for vectors: item count,
   for matrics: item count | matrix size
*/
typedef enum AkComponentSize {
  AK_COMPONENT_SIZE_UNKNOWN = 0,
  AK_COMPONENT_SIZE_SCALAR  = 1,
  AK_COMPONENT_SIZE_VEC2    = 2,
  AK_COMPONENT_SIZE_VEC3    = 3,
  AK_COMPONENT_SIZE_VEC4    = 4,
  AK_COMPONENT_SIZE_MAT2    = (4  << 3) | 2,
  AK_COMPONENT_SIZE_MAT3    = (9  << 3) | 3,
  AK_COMPONENT_SIZE_MAT4    = (16 << 3) | 4
} AkComponentSize;

struct AkBuffer {
  struct AkBuffer *next;
  const char      *name;
  void            *data;
  size_t           length;
};

typedef struct AkAccessor {
  struct AkAccessor *next;
  struct AkBuffer   *buffer;
  const char        *name;
  void              *min;
  void              *max;
  size_t             byteOffset;           /* byte offset on the buffer        */
  size_t             byteStride;           /* stride in bytes                  */
  size_t             byteLength;           /* total bytes for this accessor    */
  uint32_t           count;                /* count to access buffer           */
  uint32_t           bytesPerComponent;    /* component stride in bytes        */
  AkComponentSize    componentSize;        /* vec1 | vec2 | vec3 | vec4 ...    */
  AkTypeId           componentType;        /* single component type            */
  uint32_t           componentCount;
  size_t             fillByteSize;         /* filled size for single access    */
  int32_t            gpuTarget;            /* GPU buffer target to bound       */
  bool               normalized;

  /* Source-side metadata preserved across dequantize. When AssetKit
     widens a normalized integer / KHR_mesh_quantization integer
     attribute to float, componentType becomes AKT_FLOAT and normalized
     is cleared — but the original encoding is kept here so callers can
     reason about the source format (and reconstruct the quantized
     mapping if needed). When AK_OPT_PRESERVE_QUANTIZED_ATTRS is set
     and the buffer is left as integers, originalComponentType ==
     componentType and originallyNormalized == normalized. */
  AkTypeId         originalComponentType;
  bool             originallyNormalized;
} AkAccessor;

typedef struct AkDuplicatorRange {
  struct AkDuplicatorRange *next;
  AkIndexArray             *dupc;
  AkIndexArray             *dupcsum;
  size_t                    startIndex;
  size_t                    endIndex;
} AkDuplicatorRange;

typedef struct AkDuplicator {
  AkDuplicatorRange *range;
  void              *buffstate;
  void              *vertices;
  size_t             dupCount;
  size_t             bufCount;
} AkDuplicator;

typedef struct AkBufferEditState {
  AkDuplicator             *duplicator;
  AkAccessor               *oldAccessor;
  AkAccessor               *accessor;
  void                     *buff;
  char                     *url;
  struct AkBufferEditState *next;
  void                     *input;

  size_t                    count;
  uint32_t                  stride;
} AkBufferEditState;

/* Dequantize an accessor's source data into a caller-supplied float buffer.
   Always writes (count * componentCount) floats; outCapacity must be at
   least that many. Uses originalComponentType / originallyNormalized to
   drive integer-to-float conversion (normalized integers divide by the
   type max, non-normalized integers cast to float). Accessors that
   already store floats are copied through unchanged.

   This is the on-demand path callers reach for when AssetKit was asked to
   keep the raw quantized buffer (AK_OPT_PRESERVE_QUANTIZED_ATTRS) but a
   particular consumer wants floats for one accessor.

   Returns the number of floats written (0 on error or zero count). */
AK_EXPORT
size_t
ak_accessorAsFloat(AkAccessor * __restrict acc,
                   float      * __restrict out,
                   size_t                  outCapacity);

/* In-place dequantize: replaces the accessor's buffer with a tightly-packed
   float buffer, updates componentType / byteStride / fillByteSize /
   normalized, and registers the new buffer on the owning doc. Idempotent —
   accessors that are already AKT_FLOAT are left untouched.

   originalComponentType / originallyNormalized are preserved so callers
   can still recover the source-side encoding after the in-place widen. */
AK_EXPORT
void
ak_accessorMakeFloat(AkAccessor * __restrict acc);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_source_h */
