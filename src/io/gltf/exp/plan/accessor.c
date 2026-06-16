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

#include "internal.h"

AK_HIDE
bool
gltf_accessor_supported(AkAccessor * __restrict accessor) {
  uint32_t componentCount;
  size_t   componentSize;
  size_t   naturalSize;
  size_t   fillSize;
  size_t   stride;

  if (!accessor)
    return false;

  componentSize = gltf_component_type_size(accessor->componentType);
  if (componentSize == 0)
    return false;

  componentCount = gltf_component_size_count(accessor->componentSize);
  if (componentCount == 0) {
    componentCount = accessor->componentCount;
    if (componentCount == 0 || componentCount > 4)
      return false;
  } else if (accessor->componentCount != componentCount) {
    return false;
  }

  if (accessor->bytesPerComponent != componentSize)
    return false;

  if (accessor->normalized
      && accessor->componentType != AKT_BYTE
      && accessor->componentType != AKT_UBYTE
      && accessor->componentType != AKT_SHORT
      && accessor->componentType != AKT_USHORT)
    return false;

  if (componentCount > SIZE_MAX / componentSize)
    return false;

  naturalSize = componentSize * componentCount;
  fillSize    = accessor->fillByteSize ? accessor->fillByteSize : naturalSize;
  stride      = accessor->byteStride ? accessor->byteStride : fillSize;

  if (fillSize != naturalSize || stride < fillSize)
    return false;

  if ((accessor->componentSize == AK_COMPONENT_SIZE_MAT2
       || accessor->componentSize == AK_COMPONENT_SIZE_MAT3
       || accessor->componentSize == AK_COMPONENT_SIZE_MAT4)
      && accessor->componentType != AKT_FLOAT)
    return false;

  if (!accessor->buffer
      || !accessor->buffer->data
      || accessor->count == 0
      || accessor->byteOffset > accessor->buffer->length)
    return false;

  if ((size_t)(accessor->count - 1u)
      > ((size_t)-1 - accessor->byteOffset) / stride)
    return false;

  if (fillSize > (size_t)-1 - (accessor->byteOffset
                                + (size_t)(accessor->count - 1u) * stride))
    return false;

  return accessor->byteOffset
         + (size_t)(accessor->count - 1u) * stride
         + fillSize <= accessor->buffer->length;
}

AK_HIDE
bool
gltf_accessors_reserve(GLTFExpAccessorTable * __restrict table,
                       size_t                            capacity) {
  GLTFExpAccessorOut *items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

AK_HIDE
bool
gltf_accessors_add_out(GLTFExpAccessorTable * __restrict table,
                       GLTFExpAccessorOut   * __restrict out,
                       void                 * __restrict key,
                       RBTree               * __restrict map) {
  uintptr_t idx;
  size_t    newCap;

  idx = (uintptr_t)rb_find(map, key);
  if (idx > 0) {
    GLTFExpAccessorOut *existing;

    existing = &table->items[(size_t)idx - 1u];
    if (out->bufferViewTarget) {
      if (existing->bufferViewTarget
          && existing->bufferViewTarget != out->bufferViewTarget)
        return false;
      existing->bufferViewTarget = out->bufferViewTarget;
    }
  
    if (out->normalizeVec3)
      existing->normalizeVec3 = true;

    return true;
  }

  if (table->count >= GLTF_EXP_INDEX_NONE
      || (out->kind != GLTF_EXP_ACCESSOR_RAW_VIEW
          && out->kind != GLTF_EXP_ACCESSOR_RAW_FILE_VIEW
          && table->jsonCount >= GLTF_EXP_INDEX_NONE))
    return false;

  if (table->count == table->capacity) {
    if (!gltf_next_capacity(table->capacity, 128, &newCap))
      return false;
    if (!gltf_accessors_reserve(table, newCap))
      return false;
  }

  out->jsonIndex = out->kind == GLTF_EXP_ACCESSOR_RAW_VIEW
                   || out->kind == GLTF_EXP_ACCESSOR_RAW_FILE_VIEW
                   ? GLTF_EXP_INDEX_NONE
                   : table->jsonCount++;
  idx = (uintptr_t)table->count + 1;
  table->items[table->count++] = *out;
  rb_insert(map, key, (void *)idx);

  return true;
}

AK_HIDE
bool
gltf_accessors_add_accessor_target_flags(GLTFExpAccessorTable * __restrict table,
                                         AkAccessor           * __restrict accessor,
                                         uint32_t                          target,
                                         bool                              normalizeVec3) {
  GLTFExpAccessorOut out;

  if (!accessor)
    return true;

  if (!gltf_accessor_supported(accessor))
    return false;

  memset(&out, 0, sizeof(out));
  out.kind             = GLTF_EXP_ACCESSOR_ASSETKIT;
  out.accessor         = accessor;
  out.bufferViewTarget = target;
  out.normalizeVec3    = normalizeVec3;

  return gltf_accessors_add_out(table, &out, accessor, table->accessorMap);
}

AK_HIDE
bool
gltf_accessors_require_minmax_target(GLTFExpAccessorTable * __restrict table,
                                     AkAccessor           * __restrict accessor,
                                     uint32_t                          target) {
  uintptr_t idx;

  if (!accessor)
    return true;

  idx = (uintptr_t)rb_find(table->accessorMap, accessor);
  if (idx > 0) {
    GLTFExpAccessorOut *out;

    out = &table->items[(size_t)idx - 1u];
    if (target) {
      if (out->bufferViewTarget && out->bufferViewTarget != target)
        return false;
      out->bufferViewTarget = target;
    }
    out->minMaxRequired = true;
    return true;
  }

  if (!gltf_accessors_add_accessor_target_flags(table, accessor, target, false))
    return false;

  idx = (uintptr_t)rb_find(table->accessorMap, accessor);
  if (idx > 0)
    table->items[(size_t)idx - 1u].minMaxRequired = true;

  return idx > 0;
}

AK_HIDE
AkTypeId
gltf_index_component_type_for_max(AkUInt maxIndex) {
  if (maxIndex < UINT8_MAX)  return AKT_UBYTE;
  if (maxIndex < UINT16_MAX) return AKT_USHORT;
  if (maxIndex < UINT32_MAX) return AKT_UINT;
  return AKT_NONE;
}

AK_HIDE
AkUInt
gltf_index_array_max(const AkIndexArray * __restrict indices) {
  AkUInt maxIndex;
  size_t i;

  if (!indices)
    return 0;

  maxIndex = 0;
  switch (indices->componentType) {
    case AKT_UBYTE: {
      const uint8_t *src;

      src = (const uint8_t *)indices->items;
      for (i = 0; i < indices->count; i++) {
        if (src[i] > maxIndex)
          maxIndex = src[i];
      }
      break;
    }
    case AKT_USHORT: {
      const uint16_t *src;

      src = (const uint16_t *)(const void *)indices->items;
      for (i = 0; i < indices->count; i++) {
        if (src[i] > maxIndex)
          maxIndex = src[i];
      }
      break;
    }
    case AKT_UINT: {
      const uint32_t *src;

      src = (const uint32_t *)(const void *)indices->items;
      for (i = 0; i < indices->count; i++) {
        if (src[i] > maxIndex)
          maxIndex = src[i];
      }
      break;
    }
    default:
      break;
  }

  return maxIndex;
}

AK_HIDE
bool
gltf_accessors_add_indices(GLTFExpAccessorTable * __restrict table,
                           AkMeshPrimitive      * __restrict prim) {
  GLTFExpAccessorOut out;
  AkTypeId           indexComponentType;
  AkUInt             maxIndex;

  if (!prim || !prim->indices)
    return true;

  if (prim->indices->count == 0
      || prim->indices->count > UINT32_MAX
      || ak_indexComponentSize(prim->indices->componentType) == 0)
    return false;

  maxIndex           = gltf_index_array_max(prim->indices);
  prim->indices->max = maxIndex;
  indexComponentType = gltf_index_component_type_for_max(maxIndex);

  if (ak_indexComponentSize(indexComponentType) == 0)
    return false;

  memset(&out, 0, sizeof(out));
  out.kind               = GLTF_EXP_ACCESSOR_INDEX_ARRAY;
  out.primitive          = prim;
  out.indices            = prim->indices;
  out.indexComponentType = indexComponentType;
  out.bufferViewTarget   = GLTF_EXP_BUFFER_VIEW_TARGET_ELEMENT_ARRAY;

  return gltf_accessors_add_out(table, &out, prim, table->primitiveMap);
}

AK_HIDE
bool
gltf_accessors_add_raw_target(GLTFExpAccessorTable * __restrict table,
                              const void           * __restrict key,
                              const void           * __restrict data,
                              size_t                            byteLength,
                              uint32_t                          count,
                              AkTypeId                          componentType,
                              AkComponentSize                   componentSize,
                              uint32_t                          componentCount,
                              uint32_t                          target) {
  GLTFExpAccessorOut out;

  if (!key || !data || byteLength == 0 || count == 0)
    return false;

  memset(&out, 0, sizeof(out));
  out.kind              = GLTF_EXP_ACCESSOR_RAW;
  out.rawData           = data;
  out.rawByteLength     = byteLength;
  out.rawCount          = count;
  out.rawComponentType  = componentType;
  out.rawComponentSize  = componentSize;
  out.rawComponentCount = componentCount;
  out.bufferViewTarget  = target;

  return gltf_accessors_add_out(table, &out, (void *)key, table->rawMap);
}

AK_HIDE
bool
gltf_accessors_add_raw_view(GLTFExpAccessorTable * __restrict table,
                            const void           * __restrict key,
                            const void           * __restrict data,
                            size_t                            byteLength) {
  GLTFExpAccessorOut out;

  if (!key || !data || byteLength == 0)
    return false;

  memset(&out, 0, sizeof(out));
  out.kind          = GLTF_EXP_ACCESSOR_RAW_VIEW;
  out.rawData       = data;
  out.rawByteLength = byteLength;

  return gltf_accessors_add_out(table, &out, (void *)key, table->rawMap);
}

AK_HIDE
bool
gltf_accessors_add_file_view(GLTFExpAccessorTable * __restrict table,
                             const void           * __restrict key,
                             const char           * __restrict path,
                             size_t                            byteLength) {
  GLTFExpAccessorOut out;
  size_t             pathLen;

  if (!key || !path || byteLength == 0)
    return false;

  memset(&out, 0, sizeof(out));
  out.kind          = GLTF_EXP_ACCESSOR_RAW_FILE_VIEW;
  out.rawByteLength = byteLength;

  pathLen     = strlen(path);
  out.rawPath = malloc(pathLen + 1u);

  if (!out.rawPath)
    return false;

  memcpy(out.rawPath, path, pathLen + 1u);

  if (!gltf_accessors_add_out(table, &out, (void *)key, table->rawMap)) {
    free(out.rawPath);
    return false;
  }

  return true;
}

AK_HIDE
GLTFExpIndex
gltf_raw_accessor_index(GLTFExpAccessorTable * __restrict table,
                        const void           * __restrict key) {
  uintptr_t idx;

  if (!key)
    return GLTF_EXP_INDEX_NONE;

  idx = (uintptr_t)rb_find(table->rawMap, (void *)key);
  if (idx == 0)
    return GLTF_EXP_INDEX_NONE;

  return table->items[(size_t)idx - 1].jsonIndex;
}

AK_HIDE
bool
gltf_raw_accessor_require_minmax(GLTFExpAccessorTable * __restrict table,
                                 const void           * __restrict key) {
  GLTFExpAccessorOut *out;
  const float        *data;
  uintptr_t           idx;
  uint32_t            i;
  uint32_t            c;

  if (!key)
    return false;

  idx = (uintptr_t)rb_find(table->rawMap, (void *)key);
  if (idx == 0)
    return false;

  out = &table->items[(size_t)idx - 1u];
  if (out->rawComponentType != AKT_FLOAT
      || out->rawComponentCount == 0
      || out->rawComponentCount > AK_ARRAY_LEN(out->min)
      || !out->rawData
      || out->rawCount == 0)
    return false;

  data = out->rawData;
  for (c = 0; c < out->rawComponentCount; c++) {
    out->min[c] = data[c];
    out->max[c] = data[c];
  }

  for (i = 1; i < out->rawCount; i++) {
    const float *row;

    row = data + (size_t)i * out->rawComponentCount;
    for (c = 0; c < out->rawComponentCount; c++) {
      if (row[c] < out->min[c])
        out->min[c] = row[c];
      if (row[c] > out->max[c])
        out->max[c] = row[c];
    }
  }

  out->minMaxCount    = out->rawComponentCount;
  out->hasMinMax      = true;
  out->minMaxRequired = true;

  return true;
}

AK_HIDE
GLTFExpIndex
gltf_raw_buffer_view_index(GLTFExpAccessorTable * __restrict table,
                           const void           * __restrict key) {
  uintptr_t idx;

  if (!key)
    return GLTF_EXP_INDEX_NONE;

  idx = (uintptr_t)rb_find(table->rawMap, (void *)key);
  if (idx == 0)
    return GLTF_EXP_INDEX_NONE;

  return (GLTFExpIndex)(idx - 1);
}

GLTFExpIndex
gltf_accessor_index(GLTFExpAccessorTable * __restrict table,
                    AkAccessor           * __restrict accessor) {
  uintptr_t idx;

  if (!accessor)
    return GLTF_EXP_INDEX_NONE;

  idx = (uintptr_t)rb_find(table->accessorMap, accessor);
  if (idx == 0)
    return GLTF_EXP_INDEX_NONE;

  return table->items[(size_t)idx - 1].jsonIndex;
}

GLTFExpIndex
gltf_position_accessor_index(GLTFExpState    * __restrict st,
                             AkMeshPrimitive * __restrict prim) {
  size_t i;

  if (!st || !prim)
    return GLTF_EXP_INDEX_NONE;

  for (i = 0; i < st->positionAttrs.count; i++) {
    GLTFExpPositionAttrOut *entry;

    entry = &st->positionAttrs.items[i];
    if (entry->primitive == prim)
      return entry->accessorIndex;
  }

  return GLTF_EXP_INDEX_NONE;
}

GLTFExpIndex
gltf_baked_accessor_index(GLTFExpState     * __restrict st,
                          AkNode           * __restrict node,
                          AkMeshPrimitive  * __restrict prim,
                          AkInputSemantic                 semantic) {
  size_t i;

  if (!st || !node || !prim)
    return GLTF_EXP_INDEX_NONE;

  for (i = 0; i < st->bakedAttrs.count; i++) {
    GLTFExpBakedPrimAttrOut *entry;

    entry = &st->bakedAttrs.items[i];
    if (entry->node != node || entry->primitive != prim)
      continue;

    if (semantic == AK_INPUT_POSITION)
      return entry->positionAccessorIndex;
    if (semantic == AK_INPUT_NORMAL)
      return entry->normalAccessorIndex;
  }

  return GLTF_EXP_INDEX_NONE;
}

GLTFExpIndex
gltf_prim_index_accessor_index(GLTFExpAccessorTable * __restrict table,
                               AkMeshPrimitive      * __restrict prim) {
  uintptr_t idx;

  if (!prim)
    return GLTF_EXP_INDEX_NONE;

  idx = (uintptr_t)rb_find(table->primitiveMap, prim);
  if (idx == 0)
    return GLTF_EXP_INDEX_NONE;

  return table->items[(size_t)idx - 1].jsonIndex;
}

bool
gltf_primitive_mode(AkMeshPrimitive * __restrict prim, GLTFExpIndex *mode) {
  if (!prim)
    return false;

  switch (prim->type) {
    case AK_PRIMITIVE_POINTS:
      *mode = 0;
      return true;
    case AK_PRIMITIVE_LINES: {
      AkLines *lines;
      /* zero/unknown mode means a normal line list for legacy/fixup prims. */
      lines = (AkLines *)prim;
      switch (lines->mode) {
        case AK_LINES:      *mode = 1; return true;
        case AK_LINE_LOOP:  *mode = 2; return true;
        case AK_LINE_STRIP: *mode = 3; return true;
        default:            *mode = 1; return true;
      }
    }
    case AK_PRIMITIVE_TRIANGLES: {
      AkTriangles *triangles;
      /* defensive fallback for legacy or manually-built triangle lists. */
      triangles = (AkTriangles *)prim;
      switch (triangles->mode) {
        case AK_TRIANGLES:      *mode = 4; return true;
        case AK_TRIANGLE_STRIP: *mode = 5; return true;
        case AK_TRIANGLE_FAN:   *mode = 6; return true;
        default:                *mode = 4; return true;
      }
    }
    default:
      break;
  }

  return false;
}

AK_HIDE
bool
gltf_index_accessor_supported(AkAccessor * __restrict accessor) {
  if (!accessor)
    return true;

  if (ak_indexComponentSize(accessor->componentType) == 0)
    return false;

  if (accessor->componentSize != AK_COMPONENT_SIZE_SCALAR
      || accessor->componentCount != 1)
    return false;

  return accessor->count > 0;
}

AK_HIDE
uint32_t
gltf_accessor_export_component_count(AkAccessor * __restrict acc) {
  uint32_t count;

  if (!acc)
    return 0;

  count = gltf_component_size_count(acc->componentSize);
  return count ? count : acc->componentCount;
}

AK_HIDE
bool
gltf_attr_float_vec(AkAccessor * __restrict acc, uint32_t count) {
  return acc
         && acc->componentType == AKT_FLOAT
         && gltf_accessor_export_component_count(acc) == count;
}

AK_HIDE
bool
gltf_attr_uint_norm_vec(AkAccessor * __restrict acc, uint32_t count) {
  return acc
         && (acc->componentType == AKT_UBYTE
             || acc->componentType == AKT_USHORT)
         && acc->normalized
         && gltf_accessor_export_component_count(acc) == count;
}

bool
gltf_input_supported(AkInput * __restrict input) {
  AkAccessor *acc;
  uint32_t    count;

  if (!input)
    return false;

  acc = input->accessor;
  if (!acc)
    return false;

  count = gltf_accessor_export_component_count(acc);

  switch (input->semantic) {
    case AK_INPUT_POSITION:
    case AK_INPUT_NORMAL:
      return gltf_attr_float_vec(acc, 3);
    case AK_INPUT_TANGENT:
      return gltf_attr_float_vec(acc, 4);
    case AK_INPUT_TEXCOORD:
    case AK_INPUT_UV:
      return gltf_attr_float_vec(acc, 2)
             || gltf_attr_uint_norm_vec(acc, 2);
    case AK_INPUT_COLOR:
      return (count == 3 || count == 4)
             && (acc->componentType == AKT_FLOAT
                 || gltf_attr_uint_norm_vec(acc, count));
    case AK_INPUT_JOINT:
      return count == 4
             && !acc->normalized
             && (acc->componentType == AKT_UBYTE
                 || acc->componentType == AKT_USHORT);
    case AK_INPUT_WEIGHT:
      return gltf_attr_float_vec(acc, 4)
             || gltf_attr_uint_norm_vec(acc, 4);
    default:
      break;
  }

  return false;
}

AK_HIDE
bool
gltf_accessor_range_ok(AkAccessor * __restrict acc,
                       size_t                  fillSize,
                       size_t                  stride) {
  size_t lastOffset;
  size_t endOffset;

  if ((!acc || !acc->buffer) 
      || (acc->byteOffset > acc->buffer->length) 
      || (acc->count == 0) 
      || ((size_t)(acc->count - 1u) > ((size_t)-1 - acc->byteOffset) / stride))
    return false;

  lastOffset = acc->byteOffset + (size_t)(acc->count - 1u) * stride;
  if (fillSize > (size_t)-1 - lastOffset)
    return false;

  endOffset = lastOffset + fillSize;

  return endOffset <= acc->buffer->length;
}

bool
gltf_normal_input_valid(AkInput * __restrict input) {
  AkAccessor          *acc;
  const unsigned char *src;
  size_t               fillSize;
  size_t               stride;
  uint32_t             i;

  if (!input || input->semantic != AK_INPUT_NORMAL)
    return true;

  if (!(acc = input->accessor) || !acc->buffer || !acc->buffer->data)
    return false;

  if (acc->componentType != AKT_FLOAT
      || acc->componentCount != 3
      || acc->bytesPerComponent != sizeof(float))
    return true;

  fillSize = acc->fillByteSize;
  if (!fillSize)
    fillSize = (size_t)acc->bytesPerComponent * acc->componentCount;
  stride = acc->byteStride ? acc->byteStride : fillSize;

  if (fillSize < sizeof(float) * 3u
      || stride < fillSize
      || !gltf_accessor_range_ok(acc, fillSize, stride))
    return false;

  src = (const unsigned char *)acc->buffer->data + acc->byteOffset;
  for (i = 0; i < acc->count; i++) {
    const unsigned char *item;
    float                x;
    float                y;
    float                z;
    float                len2;

    item = src + (size_t)i * stride;
    memcpy(&x, item, sizeof(x));
    memcpy(&y, item + sizeof(float), sizeof(y));
    memcpy(&z, item + sizeof(float) * 2u, sizeof(z));

    len2 = x * x + y * y + z * z;
    if (!isfinite(len2) || len2 <= 0.00000001f)
      return false;
  }

  return true;
}

AkInput*
gltf_primitive_position_input(AkMeshPrimitive * __restrict prim) {
  return io_primitive_find_accessor_input(prim, AK_INPUT_POSITION, 0u);
}

bool
gltf_input_count_valid(AkMeshPrimitive * __restrict prim,
                       AkInput         * __restrict input,
                       AkInput         * __restrict posInput) {
  AkAccessor *posAcc;

  (void)prim;

  if (!input || !input->accessor)
    return false;

  if (input == posInput || input->semantic == AK_INPUT_POSITION)
    return true;

  posAcc = posInput ? posInput->accessor : NULL;
  if (!posAcc)
    return false;

  return input->accessor->count == posAcc->count;
}

uint32_t
gltf_input_source_set(AkInput * __restrict input) {
  return input ? input->set : 0;
}

AK_HIDE
uint32_t
gltf_input_set_kind(AkInput * __restrict input) {
  if (!input)
    return 0;

  switch (input->semantic) {
    case AK_INPUT_TEXCOORD:
    case AK_INPUT_UV:     return 1;
    case AK_INPUT_COLOR:  return 2;
    case AK_INPUT_JOINT:  return 3;
    case AK_INPUT_WEIGHT: return 4;
    default:              break;
  }

  return 0;
}

static inline
bool
gltf_export_nonsequential_attribute_sets(void) {
  return (AkGltfExportVersion)ak_opt_get(AK_OPT_GLTF_EXPORT_VERSION)
         == AK_GLTF_EXPORT_VERSION_2_1;
}

bool
gltf_input_has_source_set_before(AkMeshPrimitive * __restrict prim,
                                 AkInput         * __restrict first,
                                 AkInput         * __restrict limit,
                                 AkInput         * __restrict posInput,
                                 uint32_t                     kind,
                                 uint32_t                     sourceSet) {
  AkInput *scan;

  for (scan = first; scan && scan != limit; scan = scan->next) {
    if (!scan->accessor
        || !gltf_input_supported(scan)
        || !gltf_input_count_valid(prim, scan, posInput))
      continue;
    if (gltf_input_set_kind(scan) != kind)
      continue;
    if (gltf_input_source_set(scan) == sourceSet)
      return true;
  }

  return false;
}

uint32_t
gltf_input_export_set(AkMeshPrimitive * __restrict prim,
                      AkInput         * __restrict input) {
  AkInput *scan;
  AkInput *posInput;
  uint32_t kind;
  uint32_t sourceSet;
  uint32_t exportSet;

  kind = gltf_input_set_kind(input);
  if (!kind)
    return 0;

  sourceSet = gltf_input_source_set(input);
  if (gltf_export_nonsequential_attribute_sets())
    return sourceSet;

  exportSet = 0;
  posInput  = gltf_primitive_position_input(prim);

  for (scan = prim ? prim->input : NULL; scan; scan = scan->next) {
    uint32_t scanSet;

    if (!scan->accessor
        || !gltf_input_supported(scan)
        || !gltf_input_count_valid(prim, scan, posInput))
      continue;

    if (gltf_input_set_kind(scan) != kind)
      continue;

    scanSet = gltf_input_source_set(scan);
    if (scanSet >= sourceSet)
      continue;

    if (gltf_input_has_source_set_before(prim,
                                         prim->input,
                                         scan,
                                         posInput,
                                         kind,
                                         scanSet))
      continue;

    exportSet++;
  }

  return exportSet;
}

int32_t
gltf_texcoord_export_set(AkMeshPrimitive * __restrict prim,
                         int32_t                       sourceSet) {
  AkInput *input;
  AkInput *posInput;

  if (sourceSet < 0)
    return 0;

  posInput = gltf_primitive_position_input(prim);
  for (input = prim ? prim->input : NULL; input; input = input->next) {
    if (!input->accessor
        || !gltf_input_supported(input)
        || !gltf_input_count_valid(prim, input, posInput))
      continue;
  
    if (input->semantic != AK_INPUT_TEXCOORD && input->semantic != AK_INPUT_UV)
      continue;
    
    if ((int32_t)gltf_input_source_set(input) == sourceSet)
      return (int32_t)gltf_input_export_set(prim, input);
  }

  return 0;
}

bool
gltf_texcoord_source_set_valid(AkMeshPrimitive * __restrict prim,
                               int32_t                       sourceSet) {
  AkInput *input;
  AkInput *posInput;

  if (sourceSet < 0)
    sourceSet = 0;

  posInput = gltf_primitive_position_input(prim);
  for (input = prim ? prim->input : NULL; input; input = input->next) {
    if (!input->accessor
        || !gltf_input_supported(input)
        || !gltf_input_count_valid(prim, input, posInput))
      continue;
    
    if (input->semantic != AK_INPUT_TEXCOORD && input->semantic != AK_INPUT_UV)
      continue;
    
    if ((int32_t)gltf_input_source_set(input) == sourceSet)
      return true;
  }

  return false;
}

AK_HIDE
bool
gltf_morph_input_supported(AkInput * __restrict input) {
  if (!input)
    return false;

  switch (input->semantic) {
    case AK_INPUT_POSITION:
    case AK_INPUT_NORMAL:
    case AK_INPUT_TANGENT:
      return true;
    default:
      break;
  }

  return false;
}
AkMorphable*
gltf_morphable_at(AkMorphTarget * __restrict target, uint32_t primIndex) {
  AkMorphable *morphable;
  uint32_t     i;

  if (!target || !target->target)
    return NULL;

  if (target->target->type != AK_MORPHABLE_MORPHABLE)
    return NULL;

  morphable = ak_objGet(target->target);
  for (i = 0; morphable && i < primIndex; i++)
    morphable = morphable->next;

  return morphable;
}

AK_HIDE
AkMeshPrimitive*
gltf_geometry_primitive_at(AkGeometry * __restrict geom, uint32_t primIndex) {
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  uint32_t         i;

  if (!geom || !geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
    return NULL;

  mesh = ak_objGet(geom->gdata);
  if (!mesh)
    return NULL;

  prim = mesh->primitive;
  for (i = 0; prim && i < primIndex; i++)
    prim = prim->next;

  return prim;
}

AK_HIDE
AkInput*
gltf_primitive_input_by_semantic(AkMeshPrimitive * __restrict prim,
                                 AkInputSemantic               semantic) {
  AkInput *input;

  for (input = prim ? prim->input : NULL; input; input = input->next) {
    if (input->semantic == semantic && input->accessor)
      return input;
  }

  return NULL;
}

AK_HIDE
uint32_t
gltf_morph_semantic_component_count(AkInputSemantic semantic) {
  switch (semantic) {
    case AK_INPUT_POSITION:
    case AK_INPUT_NORMAL:
    case AK_INPUT_TANGENT:
      return 3;
    default:
      break;
  }

  return 0;
}
