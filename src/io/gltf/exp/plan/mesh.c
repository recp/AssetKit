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
gltf_accessor_float_vec_supported(AkAccessor * __restrict acc,
                                  uint32_t                componentCount) {
  size_t fillSize;
  size_t stride;

  if (!acc
      || !acc->buffer
      || !acc->buffer->data
      || acc->componentType != AKT_FLOAT
      || acc->bytesPerComponent != sizeof(float)
      || acc->componentCount < componentCount
      || componentCount == 0
      || componentCount > 4
      || acc->count == 0)
    return false;

  fillSize = acc->fillByteSize
             ? acc->fillByteSize
             : (size_t)acc->bytesPerComponent * acc->componentCount;
  stride   = acc->byteStride ? acc->byteStride : fillSize;

  if (fillSize < sizeof(float) * componentCount
      || stride < fillSize
      || acc->byteOffset > acc->buffer->length
      || (size_t)(acc->count - 1u) > ((size_t)-1 - acc->byteOffset) / stride)
    return false;

  return acc->byteOffset + (size_t)(acc->count - 1u) * stride + fillSize
         <= acc->buffer->length;
}

AK_HIDE
const float*
gltf_accessor_float_row(AkAccessor * __restrict acc, uint32_t index) {
  size_t fillSize;
  size_t stride;

  fillSize = acc->fillByteSize
             ? acc->fillByteSize
             : (size_t)acc->bytesPerComponent * acc->componentCount;
  stride   = acc->byteStride ? acc->byteStride : fillSize;

  return (const float *)((const char *)acc->buffer->data
                         + acc->byteOffset
                         + (size_t)index * stride);
}

AK_HIDE
bool
gltf_float_close(float a, float b) {
  float diff;
  float scale;

  diff  = fabsf(a - b);
  scale = fmaxf(fmaxf(fabsf(a), fabsf(b)), 1.0f);

  return diff <= scale * 1.0e-4f;
}

AK_HIDE
void
gltf_node_matrix(AkNode * __restrict node, mat4 matrix) {
  AkMatrix akMatrix;

  ak_transformCombine(node ? node->transform : NULL, akMatrix.val[0]);
  memcpy(matrix, akMatrix.val, sizeof(mat4));
}

AK_HIDE
bool
gltf_node_matrix_decomposable(AkNode * __restrict node) {
  mat4   m;
  mat4   rotm;
  mat4   recon;
  vec4   t4;
  versor rotation;
  vec3   scale;
  int    c;
  int    r;

  if (!node || !node->transform)
    return true;

  gltf_node_matrix(node, m);
  if (!gltf_float_close(m[0][3], 0.0f)
      || !gltf_float_close(m[1][3], 0.0f)
      || !gltf_float_close(m[2][3], 0.0f)
      || !gltf_float_close(m[3][3], 1.0f))
    return false;

  glm_decompose(m, t4, rotm, scale);
  if (!isfinite(t4[0]) || !isfinite(t4[1]) || !isfinite(t4[2])
      || !isfinite(scale[0]) || !isfinite(scale[1]) || !isfinite(scale[2]))
    return false;

  glm_mat4_quat(rotm, rotation);
  glm_quat_normalize(rotation);
  if (!isfinite(rotation[0]) || !isfinite(rotation[1])
      || !isfinite(rotation[2]) || !isfinite(rotation[3]))
    return false;

  glm_quat_mat4(rotation, recon);
  for (r = 0; r < 3; r++) {
    recon[0][r] *= scale[0];
    recon[1][r] *= scale[1];
    recon[2][r] *= scale[2];
  }
  recon[3][0] = t4[0];
  recon[3][1] = t4[1];
  recon[3][2] = t4[2];
  recon[3][3] = 1.0f;

  for (c = 0; c < 4; c++) {
    for (r = 0; r < 4; r++) {
      if (!gltf_float_close(m[c][r], recon[c][r]))
        return false;
    }
  }

  return true;
}

AK_HIDE
bool
gltf_node_can_bake_local_mesh(AkNode             * __restrict node,
                              AkInstanceGeometry * __restrict inst) {
  return node
         && node->transform
         && inst
         && !gltf_node_matrix_decomposable(node)
         && !node->chld
         && !node->node
         && !node->camera
         && !node->light
         && !node->gpuInstancing
         && !inst->skinner
         && !inst->morpher;
}

AK_HIDE
bool
gltf_position_input_needs_vec3_expansion(AkInput * __restrict input) {
  AkAccessor *acc;

  acc = input ? input->accessor : NULL;
  return input
         && input->semantic == AK_INPUT_POSITION
         && gltf_accessor_export_component_count(acc) == 2
         && gltf_accessor_float_vec_supported(acc, 2);
}

AK_HIDE
bool
gltf_plan_position_vec2(GLTFExpState    * __restrict st,
                        AkMeshPrimitive * __restrict prim,
                        AkInput         * __restrict input) {
  GLTFExpPositionAttrOut *entry;
  AkAccessor             *acc;
  float                  *data;
  size_t                  byteLength;
  uint32_t                count;
  uint32_t                i;

  if (!st || !prim || !gltf_position_input_needs_vec3_expansion(input))
    return false;

  acc = input->accessor;
#if SIZE_MAX <= UINT32_MAX
  if ((size_t)acc->count > SIZE_MAX / (sizeof(float) * 3u))
    return false;
#endif

  count      = acc->count;
  byteLength = (size_t)count * sizeof(float) * 3u;
  if (byteLength == 0)
    return false;

  entry = gltf_position_attrs_add(&st->positionAttrs, prim);
  if (!entry)
    return false;

  data = malloc(byteLength);
  if (!data)
    return false;

  entry->data        = data;
  entry->vertexCount = count;

  for (i = 0; i < count; i++) {
    const float *row;
    float       *dst;

    row = gltf_accessor_float_row(acc, i);
    dst = &data[(size_t)i * 3u];

    dst[0] = row[0];
    dst[1] = row[1];
    dst[2] = 0.0f;
  }

  if (!gltf_accessors_add_raw_target(&st->accessors,
                                     data,
                                     data,
                                     byteLength,
                                     count,
                                     AKT_FLOAT,
                                     AK_COMPONENT_SIZE_VEC3,
                                     3,
                                     GLTF_EXP_BUFFER_VIEW_TARGET_ARRAY))
    return false;

  entry->accessorIndex = gltf_raw_accessor_index(&st->accessors, data);
  return entry->accessorIndex != GLTF_EXP_INDEX_NONE
         && gltf_raw_accessor_require_minmax(&st->accessors, data);
}

AK_HIDE
bool
gltf_plan_baked_position(GLTFExpState            * __restrict st,
                         AkNode                  * __restrict bakeNode,
                         AkMeshPrimitive         * __restrict prim,
                         AkInput                 * __restrict input,
                         GLTFExpBakedPrimAttrOut * __restrict attr,
                         mat4                                 matrix) {
  AkAccessor *acc;
  float      *data;
  size_t      byteLength;
  uint32_t    componentCount;
  uint32_t    count;
  uint32_t    i;

  acc = input ? input->accessor : NULL;
  componentCount = gltf_accessor_export_component_count(acc);
  if ((componentCount != 2 && componentCount != 3)
      || !gltf_accessor_float_vec_supported(acc, componentCount))
    return false;

#if SIZE_MAX <= UINT32_MAX
  if ((size_t)acc->count > SIZE_MAX / (sizeof(float) * 3u))
    return false;
#endif

  count      = acc->count;
  byteLength = (size_t)count * sizeof(float) * 3u;
  data       = malloc(byteLength);
  if (!data)
    return false;

  for (i = 0; i < count; i++) {
    const float *row;
    vec4         src;
    vec4         dst;

    row = gltf_accessor_float_row(acc, i);
    src[0] = row[0];
    src[1] = row[1];
    src[2] = componentCount == 3 ? row[2] : 0.0f;
    src[3] = 1.0f;
    glm_mat4_mulv(matrix, src, dst);

    data[(size_t)i * 3u + 0u] = dst[0];
    data[(size_t)i * 3u + 1u] = dst[1];
    data[(size_t)i * 3u + 2u] = dst[2];
  }

  if (!gltf_accessors_add_raw_target(&st->accessors,
                                     data,
                                     data,
                                     byteLength,
                                     count,
                                     AKT_FLOAT,
                                     AK_COMPONENT_SIZE_VEC3,
                                     3,
                                     GLTF_EXP_BUFFER_VIEW_TARGET_ARRAY)) {
    free(data);
    return false;
  }

  attr->node                  = bakeNode;
  attr->primitive             = prim;
  attr->positionData          = data;
  attr->positionAccessorIndex = gltf_raw_accessor_index(&st->accessors, data);
  attr->vertexCount           = count;

  return attr->positionAccessorIndex != GLTF_EXP_INDEX_NONE
         && gltf_raw_accessor_require_minmax(&st->accessors, data);
}

AK_HIDE
bool
gltf_plan_baked_normal(GLTFExpState            * __restrict st,
                       AkInput                 * __restrict input,
                       GLTFExpBakedPrimAttrOut * __restrict attr,
                       mat4                                 matrix) {
  AkAccessor *acc;
  float      *data;
  size_t      byteLength;
  uint32_t    count;
  uint32_t    i;

  acc = input ? input->accessor : NULL;
  if (!acc)
    return true;

  if (!gltf_accessor_float_vec_supported(acc, 3)
      || (attr->vertexCount > 0 && acc->count != attr->vertexCount))
    return true;

#if SIZE_MAX <= UINT32_MAX
  if ((size_t)acc->count > SIZE_MAX / (sizeof(float) * 3u))
    return false;
#endif

  count      = acc->count;
  byteLength = (size_t)count * sizeof(float) * 3u;
  data       = malloc(byteLength);
  if (!data)
    return false;

  for (i = 0; i < count; i++) {
    const float *row;
    vec3         dst;
    float        len2;

    row = gltf_accessor_float_row(acc, i);
    dst[0] = matrix[0][0] * row[0] + matrix[1][0] * row[1] + matrix[2][0] * row[2];
    dst[1] = matrix[0][1] * row[0] + matrix[1][1] * row[1] + matrix[2][1] * row[2];
    dst[2] = matrix[0][2] * row[0] + matrix[1][2] * row[1] + matrix[2][2] * row[2];

    len2 = glm_vec3_norm2(dst);
    if (!isfinite(len2) || len2 <= 0.00000001f) {
      glm_vec3_copy((float *)row, dst);
      len2 = glm_vec3_norm2(dst);
    }

    if (isfinite(len2) && len2 > 0.00000001f)
      glm_vec3_scale(dst, 1.0f / sqrtf(len2), dst);
    else {
      dst[0] = 0.0f;
      dst[1] = 0.0f;
      dst[2] = 1.0f;
    }

    data[(size_t)i * 3u + 0u] = dst[0];
    data[(size_t)i * 3u + 1u] = dst[1];
    data[(size_t)i * 3u + 2u] = dst[2];
  }

  if (!gltf_accessors_add_raw_target(&st->accessors,
                                     data,
                                     data,
                                     byteLength,
                                     count,
                                     AKT_FLOAT,
                                     AK_COMPONENT_SIZE_VEC3,
                                     3,
                                     GLTF_EXP_BUFFER_VIEW_TARGET_ARRAY)) {
    free(data);
    return false;
  }

  attr->normalData          = data;
  attr->normalAccessorIndex = gltf_raw_accessor_index(&st->accessors, data);

  return attr->normalAccessorIndex != GLTF_EXP_INDEX_NONE;
}

AK_HIDE
bool
gltf_plan_baked_primitive_attrs(GLTFExpState    * __restrict st,
                                AkNode          * __restrict bakeNode,
                                AkMeshPrimitive * __restrict prim,
                                AkInput         * __restrict posInput) {
  GLTFExpBakedPrimAttrOut *attr;
  AkInput                 *normalInput;
  mat4                     matrix;

  attr = gltf_baked_attrs_add(&st->bakedAttrs, bakeNode, prim);
  if (!attr)
    return false;

  gltf_node_matrix(bakeNode, matrix);
  if (!gltf_plan_baked_position(st, bakeNode, prim, posInput, attr, matrix))
    return false;

  normalInput = gltf_primitive_input_by_semantic(prim, AK_INPUT_NORMAL);
  return gltf_plan_baked_normal(st, normalInput, attr, matrix);
}

AK_HIDE
bool
gltf_plan_normalized_morph_input(GLTFExpState       * __restrict st,
                                 AkInput            * __restrict baseInput,
                                 AkInput            * __restrict targetInput,
                                 AkInputSemantic                  semantic,
                                 GLTFExpMorphAttrOut * __restrict attr) {
  AkAccessor *baseAcc;
  AkAccessor *targetAcc;
  float      *data;
  size_t      byteLength;
  uint32_t    count;
  uint32_t    componentCount;
  uint32_t    i;

  baseAcc        = baseInput ? baseInput->accessor : NULL;
  targetAcc      = targetInput ? targetInput->accessor : NULL;
  componentCount = gltf_morph_semantic_component_count(semantic);

  if (!gltf_accessor_float_vec_supported(baseAcc, componentCount)
      || !gltf_accessor_float_vec_supported(targetAcc, componentCount)
      || baseAcc->count != targetAcc->count
      || targetAcc->count > UINT32_MAX)
    return false;

  count      = targetAcc->count;
  if (componentCount == 0
      || (size_t)count > SIZE_MAX / componentCount
      || (size_t)count * componentCount > SIZE_MAX / sizeof(float))
    return false;

  byteLength = (size_t)count * componentCount * sizeof(float);
  if (byteLength == 0)
    return false;

  data = malloc(byteLength);
  if (!data)
    return false;

  for (i = 0; i < count; i++) {
    const float *baseRow;
    const float *targetRow;
    uint32_t     c;

    baseRow   = gltf_accessor_float_row(baseAcc, i);
    targetRow = gltf_accessor_float_row(targetAcc, i);
    for (c = 0; c < componentCount; c++)
      data[(size_t)i * componentCount + c] = targetRow[c] - baseRow[c];
  }

  if (!gltf_accessors_add_raw_target(&st->accessors,
                                     data,
                                     data,
                                     byteLength,
                                     count,
                                     AKT_FLOAT,
                                     componentCount == 3
                                       ? AK_COMPONENT_SIZE_VEC3
                                       : AK_COMPONENT_SIZE_VEC4,
                                     componentCount,
                                     GLTF_EXP_BUFFER_VIEW_TARGET_ARRAY)) {
    free(data);
    return false;
  }

  if (semantic == AK_INPUT_POSITION) {
    attr->positionData          = data;
    attr->positionAccessorIndex = gltf_raw_accessor_index(&st->accessors, data);
    if (!gltf_raw_accessor_require_minmax(&st->accessors, data))
      return false;
  } else if (semantic == AK_INPUT_NORMAL) {
    attr->normalData          = data;
    attr->normalAccessorIndex = gltf_raw_accessor_index(&st->accessors, data);
  } else if (semantic == AK_INPUT_TANGENT) {
    attr->tangentData          = data;
    attr->tangentAccessorIndex = gltf_raw_accessor_index(&st->accessors, data);
  } else {
    free(data);
    return false;
  }

  attr->vertexCount = count;

  return true;
}

AK_HIDE
bool
gltf_plan_normalized_morph_target(GLTFExpState       * __restrict st,
                                  AkMeshPrimitive    * __restrict basePrim,
                                  AkMeshPrimitive    * __restrict targetPrim,
                                  GLTFExpMorphAttrOut * __restrict attr) {
  AkInputSemantic semantics[] = {
    AK_INPUT_POSITION,
    AK_INPUT_NORMAL,
    AK_INPUT_TANGENT
  };
  uint32_t i;
  bool     any;

  if (!basePrim || !targetPrim || !attr)
    return false;

  attr->positionAccessorIndex = GLTF_EXP_INDEX_NONE;
  attr->normalAccessorIndex   = GLTF_EXP_INDEX_NONE;
  attr->tangentAccessorIndex  = GLTF_EXP_INDEX_NONE;
  any                         = false;

  for (i = 0; i < AK_ARRAY_LEN(semantics); i++) {
    AkInput *baseInput;
    AkInput *targetInput;

    baseInput   = gltf_primitive_input_by_semantic(basePrim, semantics[i]);
    targetInput = gltf_primitive_input_by_semantic(targetPrim, semantics[i]);
    if (!baseInput || !targetInput)
      continue;

    if (!gltf_plan_normalized_morph_input(st,
                                          baseInput,
                                          targetInput,
                                          semantics[i],
                                          attr))
      return false;

    any = true;
  }

  return any && attr->positionAccessorIndex != GLTF_EXP_INDEX_NONE;
}

AK_HIDE
bool
gltf_plan_morph_target_accessors(GLTFExpState * __restrict st,
                                 AkMorph      * __restrict morph,
                                 AkMeshPrimitive * __restrict basePrim,
                                 uint32_t                  primIndex,
                                 GLTFExpMorphAttrOut * __restrict morphAttrs,
                                 uint32_t                  morphAttrPrimCount) {
  AkMorphTarget *target;
  uint32_t       targetIndex;

  if (!morph)
    return true;

  if (morph->targetCount == 0)
    return true;

  if (morph->method != AK_MORPH_METHOD_RELATIVE
      && morph->method != AK_MORPH_METHOD_ADDITIVE
      && morph->method != AK_MORPH_METHOD_NORMALIZED)
    return false;

  targetIndex = 0;
  for (target = morph->target; target; target = target->next, targetIndex++) {
    AkMorphable *morphable;
    AkInput     *input;
    bool         any;

    if (target->target && target->target->type == AK_MORPHABLE_GEOMETRY) {
      AkGeometry      *targetGeom;
      AkMeshPrimitive *targetPrim;

      targetGeom = ak_objGetTarget(target->target);
      targetPrim = gltf_geometry_primitive_at(targetGeom, primIndex);
      if (!targetPrim)
        return false;

      if (morph->method == AK_MORPH_METHOD_NORMALIZED) {
        if (!morphAttrs
            || morphAttrPrimCount == 0
            || !gltf_plan_normalized_morph_target(
                  st,
                  basePrim,
                  targetPrim,
                  &morphAttrs[(size_t)targetIndex * morphAttrPrimCount
                              + primIndex]))
          return false;
        continue;
      }

      morphable = NULL;
      any       = false;
      for (input = targetPrim->input; input; input = input->next) {
        if (!input->accessor || !gltf_morph_input_supported(input))
          continue;

        any = true;
        if (input->semantic == AK_INPUT_POSITION) {
          if (!gltf_accessors_require_minmax_target(&st->accessors,
                                                    input->accessor,
                                                    GLTF_EXP_BUFFER_VIEW_TARGET_ARRAY))
            return false;
        } else if (!gltf_accessors_add_accessor_target_flags(
                                                       &st->accessors,
                                                       input->accessor,
                                                       GLTF_EXP_BUFFER_VIEW_TARGET_ARRAY,
                                                       false)) {
          return false;
        }
      }

      if (!any)
        return false;
      continue;
    }

    morphable = gltf_morphable_at(target, primIndex);
    if (!morphable)
      return false;

    any = false;
    for (input = morphable->input; input; input = input->next) {
      if (!input->accessor)
        continue;
      if (!gltf_morph_input_supported(input))
        continue;

      any = true;
      if (input->semantic == AK_INPUT_POSITION) {
        if (!gltf_accessors_require_minmax_target(&st->accessors,
                                                  input->accessor,
                                                  GLTF_EXP_BUFFER_VIEW_TARGET_ARRAY))
          return false;
      } else if (!gltf_accessors_add_accessor_target_flags(
                                                     &st->accessors,
                                                     input->accessor,
                                                     GLTF_EXP_BUFFER_VIEW_TARGET_ARRAY,
                                                     false)) {
        return false;
      }
    }

    if (!any)
      return false;
  }

  return true;
}

AK_HIDE
void
gltf_plan_mesh_quantization_input(GLTFExpState * __restrict st,
                                  AkInput      * __restrict input) {
  AkAccessor *accessor;

  accessor = input ? input->accessor : NULL;
  if (!accessor || accessor->componentType == AKT_FLOAT)
    return;

  switch (input->semantic) {
    case AK_INPUT_POSITION:
    case AK_INPUT_NORMAL:
    case AK_INPUT_TANGENT:
    case AK_INPUT_TEXCOORD:
    case AK_INPUT_UV:
      st->usesMeshQuantization = true;
      break;
    default:
      break;
  }
}

AK_HIDE
bool
gltf_plan_mesh_accessors(GLTFExpState       * __restrict st,
                         AkGeometry         * __restrict geom,
                         AkInstanceGeometry * __restrict inst,
                         AkNode             * __restrict bakeNode,
                         GLTFExpIndex       * __restrict skinAttrOffset,
                         uint32_t           * __restrict skinAttrCount,
                         GLTFExpIndex       * __restrict morphAttrOffset,
                         uint32_t           * __restrict morphAttrCount,
                         uint32_t           * __restrict morphAttrPrimCount) {
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkMorph         *morph;
  AkInstanceSkin  *skinner;
  uint32_t         primIndex;
  uint32_t         primCount;
  bool             needsGeneratedSkinAttrs;

  if (!geom || !geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
    return false;

  mesh = ak_objGet(geom->gdata);
  if (!mesh)
    return false;

  *skinAttrOffset = GLTF_EXP_INDEX_NONE;
  *skinAttrCount  = 0;
  *morphAttrOffset = GLTF_EXP_INDEX_NONE;
  *morphAttrCount = 0;
  *morphAttrPrimCount = 0;

  morph     = inst && inst->morpher ? inst->morpher->morph : NULL;
  skinner   = inst && gltf_skin_valid(inst->skinner) ? inst->skinner : NULL;
  needsGeneratedSkinAttrs = skinner && gltf_mesh_needs_generated_skin_attrs(mesh);
  primCount = 0;
  if (skinner || morph) {
    for (prim = mesh->primitive; prim; prim = prim->next)
      primCount++;
  }

  if (needsGeneratedSkinAttrs) {
    if (primCount > 0) {
      if (!gltf_skin_attrs_reserve_span(&st->skinAttrs,
                                        primCount,
                                        skinAttrOffset))
        return false;
      *skinAttrCount = primCount;
    }
  }

  if (morph
      && morph->targetCount > 0
      && morph->method == AK_MORPH_METHOD_NORMALIZED) {
    size_t morphAttrTotal;

    if (primCount == 0
        || morph->targetCount == 0
        || morph->targetCount > UINT32_MAX
        || primCount > UINT32_MAX
        || (size_t)morph->targetCount > (size_t)-1 / primCount)
      return false;

    morphAttrTotal = (size_t)morph->targetCount * primCount;
    if (!gltf_morph_attrs_reserve_span(&st->morphAttrs,
                                       morphAttrTotal,
                                       morphAttrOffset))
      return false;

    *morphAttrCount     = (uint32_t)morphAttrTotal;
    *morphAttrPrimCount = primCount;
  }

  primIndex = 0;
  for (prim = mesh->primitive; prim; prim = prim->next, primIndex++) {
    AkInput *input;
    AkInput *posInput;
    uint32_t posComponentCount;
    GLTFExpIndex mode;
    bool     hasAttribute;
    AkResolvedMaterial resolved;
    AkMaterialVariantMapping *mapping;

    if (!gltf_primitive_mode(prim, &mode))
      return false;

    if (ak_meshPrimitiveEnsureSingleIndex(prim) != AK_OK)
      return false;

    posInput = gltf_primitive_position_input(prim);
    hasAttribute = false;

    if (!gltf_index_accessor_supported(prim->indexAccessor))
      return false;

    if (!gltf_accessors_add_accessor_target_flags(
                                            &st->accessors,
                                            prim->indexAccessor,
                                            GLTF_EXP_BUFFER_VIEW_TARGET_ELEMENT_ARRAY,
                                            false))
      return false;

    if (!prim->indexAccessor
        && prim->indices
        && !gltf_accessors_add_indices(&st->accessors, prim))
      return false;

    if (posInput) {
      gltf_plan_mesh_quantization_input(st, posInput);
      posComponentCount = gltf_accessor_export_component_count(posInput->accessor);
      if (bakeNode) {
        if (!gltf_plan_baked_primitive_attrs(st, bakeNode, prim, posInput))
          return false;
        hasAttribute = true;
      } else if (posComponentCount == 2) {
        if (!gltf_plan_position_vec2(st, prim, posInput))
          return false;
        hasAttribute = true;
      } else if (posComponentCount == 3) {
        if (!gltf_accessors_require_minmax_target(&st->accessors,
                                                  posInput->accessor,
                                                  GLTF_EXP_BUFFER_VIEW_TARGET_ARRAY))
          return false;
        hasAttribute = true;
      } else {
        return false;
      }
    }

    for (input = prim->input; input; input = input->next) {
      if (!input->accessor
          || input == posInput
          || input->semantic == AK_INPUT_POSITION
          || (bakeNode && input->semantic == AK_INPUT_NORMAL)
          || !gltf_input_supported(input))
        continue;
      if (!gltf_normal_input_valid(input))
        continue;
      if (!gltf_input_count_valid(prim, input, posInput))
        continue;

      gltf_plan_mesh_quantization_input(st, input);
      hasAttribute = true;

      if (!gltf_accessors_add_accessor_target_flags(
            &st->accessors,
            input->accessor,
            GLTF_EXP_BUFFER_VIEW_TARGET_ARRAY,
            input->semantic == AK_INPUT_NORMAL))
        return false;
    }

    if (needsGeneratedSkinAttrs
        && !gltf_primitive_has_exportable_skin_inputs(prim, posInput)) {
      if (*skinAttrOffset == GLTF_EXP_INDEX_NONE
          || primIndex >= *skinAttrCount
          || !gltf_plan_generated_skin_attrs(
                st,
                skinner,
                prim,
                posInput,
                primIndex,
                &st->skinAttrs.items[*skinAttrOffset + primIndex]))
        return false;
    }

    if (!hasAttribute)
      return false;

    if (!gltf_plan_extra_extensions(st,
                                    prim->extra,
                                    gltf_plan_skip_primitive_core_extension,
                                    NULL))
      return false;

    if (ak_materialResolve(prim, inst, UINT32_MAX, &resolved)) {
      if (!gltf_plan_material(st, resolved.material, prim, inst))
        return false;
    }

    for (mapping = prim->variantMappings; mapping; mapping = mapping->next) {
      if (!mapping->material
          || mapping->variantIndex >= st->materialVariantCount) {
        st->failResult = AK_EINVAL;
        return false;
      }

      if (gltf_material_is_default_noop(mapping->material)
          || !gltf_material_surface_compatible(st,
                                               prim,
                                               inst,
                                               mapping->material->surface,
                                               false))
        continue;

      if (!gltf_plan_material(st, mapping->material, prim, inst))
        return false;

      st->usesMaterialVariants = true;
    }

    if (!gltf_plan_morph_target_accessors(
          st,
          morph,
          prim,
          primIndex,
          *morphAttrOffset != GLTF_EXP_INDEX_NONE
            ? &st->morphAttrs.items[*morphAttrOffset]
            : NULL,
          *morphAttrPrimCount))
      return false;
  }

  return true;
}

AK_HIDE
bool
gltf_plan_mesh(GLTFExpState       * __restrict st,
               AkInstanceGeometry * __restrict inst,
               AkNode             * __restrict bakeNode,
               GLTFExpIndex       * __restrict meshIndex) {
  AkGeometry *geom;
  AkMesh     *mesh;
  void       *key;
  GLTFExpIndex idx;
  GLTFExpIndex skinAttrOffset;
  GLTFExpIndex morphAttrOffset;
  uint32_t    skinAttrCount;
  uint32_t    morphAttrCount;
  uint32_t    morphAttrPrimCount;

  *meshIndex = GLTF_EXP_INDEX_NONE;

  if (!inst || !inst->base.object)
    return true;

  geom = inst->base.object;

  if (!geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH) {
    st->failResult = AK_EINVAL;
    return false;
  }

  mesh = ak_objGet(geom->gdata);
  if (!mesh)
    return false;

  if (!mesh->primitive)
    return true;

  if (!gltf_plan_extra_extensions(st,
                                  mesh->extra ? mesh->extra : geom->extra,
                                  NULL,
                                  NULL))
    return false;

  key = bakeNode ? (void *)bakeNode : gltf_mesh_key(geom, inst);
  idx = gltf_mesh_index(&st->meshes, key);
  if (idx != GLTF_EXP_INDEX_NONE) {
    *meshIndex = idx;
    return true;
  }

  if (!gltf_plan_mesh_accessors(st,
                                geom,
                                inst,
                                bakeNode,
                                &skinAttrOffset,
                                &skinAttrCount,
                                &morphAttrOffset,
                                &morphAttrCount,
                                &morphAttrPrimCount))
    return false;

  if (!gltf_meshes_add(&st->meshes,
                       key,
                       geom,
                       inst,
                       bakeNode,
                       skinAttrOffset,
                       skinAttrCount,
                       morphAttrOffset,
                       morphAttrCount,
                       morphAttrPrimCount))
    return false;

  idx = gltf_mesh_index(&st->meshes, key);
  if (idx == GLTF_EXP_INDEX_NONE)
    return false;

  *meshIndex = idx;

  return true;
}
