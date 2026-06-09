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

#include "accessor.h"
#include "../strpool.h"

#include <stdint.h>

static
int
gltf_component_type(AkTypeId type) {
  switch (type) {
    case AKT_BYTE:   return 5120;
    case AKT_UBYTE:  return 5121;
    case AKT_SHORT:  return 5122;
    case AKT_USHORT: return 5123;
    case AKT_UINT:   return 5125;
    case AKT_FLOAT:  return 5126;
    default: break;
  }

  return 0;
}

static
bool
gltf_accessor_type(AkComponentSize compSize,
                   uint32_t        compCount,
                   const char    **type,
                   size_t         *typeLen) {
  switch (compSize) {
    case AK_COMPONENT_SIZE_SCALAR:
      *type = _s_gltf_SCALAR; *typeLen = _s_gltf_SCALAR_len; return true;
    case AK_COMPONENT_SIZE_VEC2:
      *type = _s_gltf_VEC2; *typeLen = _s_gltf_VEC2_len; return true;
    case AK_COMPONENT_SIZE_VEC3:
      *type = _s_gltf_VEC3; *typeLen = _s_gltf_VEC3_len; return true;
    case AK_COMPONENT_SIZE_VEC4:
      *type = _s_gltf_VEC4; *typeLen = _s_gltf_VEC4_len; return true;
    case AK_COMPONENT_SIZE_MAT2:
      *type = _s_gltf_MAT2; *typeLen = _s_gltf_MAT2_len; return true;
    case AK_COMPONENT_SIZE_MAT3:
      *type = _s_gltf_MAT3; *typeLen = _s_gltf_MAT3_len; return true;
    case AK_COMPONENT_SIZE_MAT4:
      *type = _s_gltf_MAT4; *typeLen = _s_gltf_MAT4_len; return true;
    default:
      break;
  }

  switch (compCount) {
    case 1: *type = _s_gltf_SCALAR; *typeLen = _s_gltf_SCALAR_len; return true;
    case 2: *type = _s_gltf_VEC2;   *typeLen = _s_gltf_VEC2_len;   return true;
    case 3: *type = _s_gltf_VEC3;   *typeLen = _s_gltf_VEC3_len;   return true;
    case 4: *type = _s_gltf_VEC4;   *typeLen = _s_gltf_VEC4_len;   return true;
    default: break;
  }

  return false;
}

void
gltf_write_buffers(GLTFExpWriter * __restrict w,
                   GLTFExpState  * __restrict st) {
  if (st->accessors.count == 0)
    return;

  gltf_w_key(w, _s_gltf_buffers, _s_gltf_buffers_len);
  gltf_w_ch(w, '[');
  gltf_w_ch(w, '{');
  if (!st->glb) {
    gltf_w_key_str(w, _s_gltf_uri, _s_gltf_uri_len, st->binUri);
    gltf_w_ch(w, ',');
  }
  gltf_w_key_uint(w, _s_gltf_byteLength, _s_gltf_byteLength_len,
                  st->binByteLength);
  gltf_w_ch(w, '}');
  gltf_w_ch(w, ']');
}

void
gltf_write_buffer_views(GLTFExpWriter * __restrict w,
                        GLTFExpState  * __restrict st) {
  size_t i;

  if (st->accessors.count == 0)
    return;

  gltf_w_key(w, _s_gltf_bufferViews, _s_gltf_bufferViews_len);
  gltf_w_ch(w, '[');

  for (i = 0; i < st->accessors.count; i++) {
    GLTFExpAccessorOut *out;

    out = &st->accessors.items[i];

    if (i > 0)
      gltf_w_ch(w, ',');

    gltf_w_ch(w, '{');
    gltf_w_key_uint(w, _s_gltf_buffer, _s_gltf_buffer_len, 0);
    gltf_w_ch(w, ',');
    gltf_w_key_uint(w, _s_gltf_byteOffset, _s_gltf_byteOffset_len,
                    out->byteOffset);
    gltf_w_ch(w, ',');
    gltf_w_key_uint(w, _s_gltf_byteLength, _s_gltf_byteLength_len,
                    out->byteLength);
    if (out->bufferViewTarget) {
      gltf_w_ch(w, ',');
      gltf_w_key_uint(w,
                      _s_gltf_target,
                      _s_gltf_target_len,
                      out->bufferViewTarget);
    }
    gltf_w_ch(w, '}');
  }

  gltf_w_ch(w, ']');
}

static
bool
gltf_accessor_writes_json(GLTFExpAccessorOut * __restrict out) {
  return out->kind != GLTF_EXP_ACCESSOR_RAW_VIEW
         && out->kind != GLTF_EXP_ACCESSOR_RAW_FILE_VIEW;
}

bool
gltf_has_accessors(GLTFExpState * __restrict st) {
  return st->accessors.jsonCount > 0;
}

static
void
gltf_write_accessor(GLTFExpWriter      * __restrict w,
                    GLTFExpAccessorOut * __restrict out,
                    GLTFExpIndex                    index) {
  AkTypeId        componentType;
  AkComponentSize componentSize;
  const char     *type;
  size_t          typeLen;
  uint32_t        count;
  uint32_t        componentCount;
  bool            normalized;
  int             glComponentType;

  normalized     = false;
  componentCount = 1;

  if (out->kind == GLTF_EXP_ACCESSOR_ASSETKIT) {
    AkAccessor *acc;

    acc            = out->accessor;
    componentType  = acc->componentType;
    componentSize  = acc->componentSize;
    componentCount = acc->componentCount;
    count          = acc->count;
    normalized     = acc->normalized;
  } else if (out->kind == GLTF_EXP_ACCESSOR_INDEX_ARRAY) {
    componentType = out->indexComponentType
                    ? out->indexComponentType
                    : out->indices->componentType;
    componentSize = AK_COMPONENT_SIZE_SCALAR;
    count         = (uint32_t)out->indices->count;
  } else {
    componentType  = out->rawComponentType;
    componentSize  = out->rawComponentSize;
    componentCount = out->rawComponentCount;
    count          = out->rawCount;
  }

  glComponentType = gltf_component_type(componentType);
  if (!glComponentType
      || !gltf_accessor_type(componentSize, componentCount, &type, &typeLen)) {
    w->result = AK_ERR;
    return;
  }

  gltf_w_ch(w, '{');
  gltf_w_key_uint(w, _s_gltf_bufferView, _s_gltf_bufferView_len, index);
  gltf_w_ch(w, ',');
  gltf_w_key_uint(w, _s_gltf_componentType, _s_gltf_componentType_len,
                  (size_t)glComponentType);
  gltf_w_ch(w, ',');
  gltf_w_key_uint(w, _s_gltf_count, _s_gltf_count_len, count);
  gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_type, _s_gltf_type_len);
  gltf_w_qstr_len(w, type, typeLen);

  if (normalized) {
    gltf_w_ch(w, ',');
    gltf_w_key_bool(w, _s_gltf_normalized, _s_gltf_normalized_len, true);
  }

  if (out->minMaxRequired) {
    uint32_t i;

    if (!out->hasMinMax
        || out->minMaxCount == 0
        || out->minMaxCount > AK_ARRAY_LEN(out->min)) {
      w->result = AK_ERR;
      return;
    }

    gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_min, _s_gltf_min_len);
    gltf_w_ch(w, '[');
    for (i = 0; i < out->minMaxCount; i++) {
      if (i > 0)
        gltf_w_ch(w, ',');
      gltf_w_number(w, out->min[i]);
    }
    gltf_w_ch(w, ']');

    gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_max, _s_gltf_max_len);
    gltf_w_ch(w, '[');
    for (i = 0; i < out->minMaxCount; i++) {
      if (i > 0)
        gltf_w_ch(w, ',');
      gltf_w_number(w, out->max[i]);
    }
    gltf_w_ch(w, ']');
  }

  gltf_w_ch(w, '}');
}

void
gltf_write_accessors(GLTFExpWriter * __restrict w,
                     GLTFExpState  * __restrict st) {
  size_t i;
  bool   comma;

  if (!gltf_has_accessors(st))
    return;

  gltf_w_key(w, _s_gltf_accessors, _s_gltf_accessors_len);
  gltf_w_ch(w, '[');

  comma = false;
  for (i = 0; i < st->accessors.count; i++) {
    if (!gltf_accessor_writes_json(&st->accessors.items[i]))
      continue;

    if (comma)
      gltf_w_ch(w, ',');
    gltf_write_accessor(w, &st->accessors.items[i], (GLTFExpIndex)i);
    comma = true;
  }

  gltf_w_ch(w, ']');
}
