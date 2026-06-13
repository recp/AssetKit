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

#ifndef io_common_primitive_h
#define io_common_primitive_h

#include "../../../include/ak/assetkit.h"
#include "../../common.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct IOFloatRows {
  AkAccessor *accessor;
  float      *scratch;
  const char *directData;
  size_t      byteStride;
  uint32_t    componentCount;
  bool        direct;
} IOFloatRows;

typedef struct IOIndexRows {
  const unsigned char *data;
  size_t              byteStride;
  uint32_t            count;
  AkTypeId            componentType;
  bool                valid;
} IOIndexRows;

typedef struct IOTriangleIter {
  AkTriangleMode mode;
  uint32_t       count;
  uint32_t       cursor;
} IOTriangleIter;

AK_HIDE
uint32_t
io_primitive_vertex_count(AkMeshPrimitive * __restrict prim);

AK_HIDE
AkUInt
io_primitive_input_index(AkMeshPrimitive * __restrict prim,
                         AkInput         * __restrict input,
                         uint32_t                     vertexIndex);

static inline
bool
io_index_rows_init(IOIndexRows * __restrict rows,
                   AkAccessor  * __restrict acc) {
  size_t stride;

  memset(rows, 0, sizeof(*rows));
  if (!acc || !acc->buffer || !acc->buffer->data || acc->count == 0)
    return false;

  switch (acc->componentType) {
    case AKT_UBYTE:
    case AKT_USHORT:
    case AKT_UINT:
      break;
    default:
      return false;
  }

  stride = acc->byteStride ? acc->byteStride : acc->bytesPerComponent;
  if (stride < acc->bytesPerComponent
      || acc->byteOffset > acc->buffer->length
      || (size_t)(acc->count - 1u)
           > ((size_t)-1 - acc->byteOffset) / stride
      || acc->byteOffset
           + (size_t)(acc->count - 1u) * stride
           + acc->bytesPerComponent > acc->buffer->length)
    return false;

  rows->data          = (const unsigned char *)acc->buffer->data + acc->byteOffset;
  rows->byteStride    = stride;
  rows->count         = acc->count;
  rows->componentType = acc->componentType;
  rows->valid         = true;
  return true;
}

static inline
AkUInt
io_index_rows_get_unchecked(const IOIndexRows * __restrict rows,
                            uint32_t                       index) {
  const unsigned char *src;

  src = rows->data + (size_t)index * rows->byteStride;
  switch (rows->componentType) {
    case AKT_UBYTE:
      return src[0];
    case AKT_USHORT: {
      uint16_t v;
      memcpy(&v, src, sizeof(v));
      return v;
    }
    case AKT_UINT: {
      uint32_t v;
      memcpy(&v, src, sizeof(v));
      return v;
    }
    default:
      return index;
  }
}

static inline
AkUInt
io_index_rows_get(const IOIndexRows * __restrict rows,
                  uint32_t                       index) {
  if (!rows->valid || index >= rows->count)
    return index;

  return io_index_rows_get_unchecked(rows, index);
}

static inline
AkUInt
io_index_accessor_get(AkAccessor * __restrict acc, uint32_t index) {
  const unsigned char *src;
  size_t              stride;

  if (!acc || !acc->buffer || !acc->buffer->data || index >= acc->count)
    return index;

  stride = acc->byteStride ? acc->byteStride : acc->bytesPerComponent;
  src    = (const unsigned char *)acc->buffer->data
           + acc->byteOffset
           + (size_t)index * stride;

  switch (acc->componentType) {
    case AKT_UBYTE:
      return src[0];
    case AKT_USHORT: {
      uint16_t v;
      memcpy(&v, src, sizeof(v));
      return v;
    }
    case AKT_UINT: {
      uint32_t v;
      memcpy(&v, src, sizeof(v));
      return v;
    }
    default:
      return index;
  }
}

static inline
AkInput*
io_primitive_find_input(AkMeshPrimitive * __restrict prim,
                        AkInputSemantic              semantic) {
  AkInput *input;

  if (!prim)
    return NULL;

  if (semantic == AK_INPUT_POSITION && prim->pos)
    return prim->pos;

  for (input = prim->input; input; input = input->next) {
    if (input->semantic == semantic)
      return input;
  }

  return NULL;
}

static inline
AkInput*
io_primitive_find_set_input(AkMeshPrimitive * __restrict prim,
                            AkInputSemantic              semantic,
                            AkInputSemantic              altSemantic,
                            uint32_t                     minComponents) {
  AkInput *input;
  AkInput *fallback;

  fallback = NULL;
  for (input = prim ? prim->input : NULL; input; input = input->next) {
    if (input->semantic != semantic && input->semantic != altSemantic)
      continue;
    if (minComponents
        && (!input->accessor
            || input->accessor->componentCount < minComponents))
      continue;
    if (input->set == 0)
      return input;
    if (!fallback)
      fallback = input;
  }

  return fallback;
}

static inline
AkInput*
io_primitive_find_accessor_input(AkMeshPrimitive * __restrict prim,
                                 AkInputSemantic              semantic,
                                 uint32_t                     minComponents) {
  AkInput *input;

  if (!prim)
    return NULL;

  if (semantic == AK_INPUT_POSITION
      && prim->pos
      && prim->pos->accessor
      && (!minComponents
          || prim->pos->accessor->componentCount >= minComponents))
    return prim->pos;

  for (input = prim->input; input; input = input->next) {
    if (input->semantic == semantic
        && input->accessor
        && (!minComponents
            || input->accessor->componentCount >= minComponents))
      return input;
  }

  return NULL;
}

AK_HIDE
bool
io_accessor_float_direct(AkAccessor * __restrict acc);

AK_HIDE
const float*
io_accessor_float_row(AkAccessor * __restrict acc, uint32_t index);

AK_HIDE
bool
io_float_rows_init(IOFloatRows * __restrict rows,
                   AkAccessor  * __restrict acc);

AK_HIDE
void
io_float_rows_destroy(IOFloatRows * __restrict rows);

static inline
const float*
io_float_rows_get(IOFloatRows * __restrict rows, uint32_t index) {
  if (index >= rows->accessor->count)
    index = 0;

  return rows->direct
         ? (const float *)(rows->directData + (size_t)index * rows->byteStride)
         : rows->scratch + (size_t)index * rows->componentCount;
}

static inline
float
io_float_row_component(const float * __restrict row,
                       uint32_t                 componentCount,
                       uint32_t                 component,
                       float                    fallback) {
  return component < componentCount ? row[component] : fallback;
}

AK_HIDE
bool
io_triangle_iter_init(IOTriangleIter  * __restrict it,
                      AkMeshPrimitive * __restrict prim);

AK_HIDE
bool
io_triangle_iter_next(IOTriangleIter * __restrict it,
                      uint32_t                    tri[3]);

static inline
void
io_vec3_normalize_or_zero(vec3 value) {
  float len;

  len = glm_vec3_norm(value);
  if (len > 0.0f && isfinite(len)) {
    glm_vec3_scale(value, 1.0f / len, value);
  } else {
    glm_vec3_zero(value);
  }
}

static inline
void
io_triangle_normal(vec3 a, vec3 b, vec3 c, vec3 normal) {
  vec3 ab;
  vec3 ac;

  glm_vec3_sub(b, a, ab);
  glm_vec3_sub(c, a, ac);
  glm_vec3_cross(ab, ac, normal);
  io_vec3_normalize_or_zero(normal);
}

#endif /* io_common_primitive_h */
