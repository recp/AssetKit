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

#include "anim.h"
#include "source.h"
#include "../../../string_fast.h"

#include <cglm/cglm.h>

#include <math.h>
#include <string.h>

static
AkInput*
dae_anim_input(AkAnimSampler  * __restrict sampler,
               AkInputSemantic             semantic) {
  AkInput *input;

  if (!sampler)
    return NULL;

  switch (semantic) {
    case AK_INPUT_INPUT:
      if (sampler->inputInput)
        return sampler->inputInput;
      break;
    case AK_INPUT_OUTPUT:
      if (sampler->outputInput)
        return sampler->outputInput;
      break;
    case AK_INPUT_IN_TANGENT:
      if (sampler->inTangentInput)
        return sampler->inTangentInput;
      break;
    case AK_INPUT_OUT_TANGENT:
      if (sampler->outTangentInput)
        return sampler->outTangentInput;
      break;
    case AK_INPUT_INTERPOLATION:
      if (sampler->interpInput)
        return sampler->interpInput;
      break;
    default:
      break;
  }

  for (input = sampler->input; input; input = input->next) {
    if (input->semantic == semantic)
      return input;
  }

  return NULL;
}

static
AkAccessor*
dae_anim_accessor(AkAnimSampler  * __restrict sampler,
                  AkInputSemantic             semantic) {
  AkInput *input;

  input = dae_anim_input(sampler, semantic);
  return input ? input->accessor : NULL;
}

static
AkObject*
dae_anim_transform_target(AkChannel * __restrict channel) {
  AkObject *target;

  target = channel && channel->resolvedTarget
           ? channel->resolvedTarget->target
           : NULL;
  if (!target || ak_typeid(target) != AKT_OBJECT)
    return NULL;

  switch ((AkTypeId)target->type) {
    case AKT_MATRIX:
    case AKT_ROTATE:
    case AKT_QUATERNION:
    case AKT_SCALE:
    case AKT_TRANSLATE:
      return target;
    default:
      return NULL;
  }
}

static
AkTargetPropertyType
dae_anim_transform_target_type(AkObject * __restrict target) {
  if (!target)
    return AK_TARGET_UNKNOWN;

  switch ((AkTypeId)target->type) {
    case AKT_MATRIX:     return AK_TARGET_FLOAT;
    case AKT_ROTATE:     return AK_TARGET_ROTATE;
    case AKT_QUATERNION: return AK_TARGET_QUAT;
    case AKT_SCALE:      return AK_TARGET_SCALE;
    case AKT_TRANSLATE:  return AK_TARGET_POSITION;
    default:             return AK_TARGET_UNKNOWN;
  }
}

static
AkTargetPropertyType
dae_anim_channel_target_type(AkChannel * __restrict channel) {
  AkObject *target;

  if (channel
      && channel->resolvedTarget
      && channel->resolvedTarget->isPartial
      && channel->targetType != AK_TARGET_UNKNOWN)
    return channel->targetType;

  target = dae_anim_transform_target(channel);
  if (target)
    return dae_anim_transform_target_type(target);

  return channel ? channel->targetType : AK_TARGET_UNKNOWN;
}

static
bool
dae_anim_accessor_storage_supported(AkAccessor * __restrict acc) {
  size_t rowBytes, stride, last;

  if (!acc
      || !acc->buffer
      || !acc->buffer->data
      || acc->count == 0u
      || acc->componentCount == 0u
      || acc->bytesPerComponent == 0u
      || (size_t)acc->componentCount
           > (size_t)-1 / acc->bytesPerComponent)
    return false;

  rowBytes = (size_t)acc->componentCount * acc->bytesPerComponent;
  stride   = acc->byteStride ? acc->byteStride : rowBytes;
  if (stride < rowBytes || acc->byteOffset > acc->buffer->length)
    return false;
  if ((size_t)(acc->count - 1u) > ((size_t)-1 - acc->byteOffset) / stride)
    return false;
  last = acc->byteOffset + (size_t)(acc->count - 1u) * stride;
  return last <= acc->buffer->length
         && rowBytes <= acc->buffer->length - last;
}

static
bool
dae_anim_float_accessor_supported(AkAccessor            * __restrict acc,
                                  uint32_t                           count,
                                  AkTargetPropertyType               targetType) {
  if (!dae_anim_accessor_storage_supported(acc)
      || acc->count != count
      || acc->componentCount == 0)
    return false;

  switch (targetType) {
    case AK_TARGET_POSITION:
    case AK_TARGET_SCALE:
      return acc->componentCount == 3;
    case AK_TARGET_ROTATE:
    case AK_TARGET_QUAT:
      return acc->componentCount == 4;
    case AK_TARGET_FLOAT:
      return acc->componentCount == 1 || acc->componentCount == 16;
    default:
      break;
  }

  return true;
}

static
bool
dae_anim_tangent_accessor_supported(AkAccessor            * __restrict acc,
                                    uint32_t                           count,
                                    AkTargetPropertyType               targetType,
                                    uint32_t                           outputComponents) {
  if (!dae_anim_accessor_storage_supported(acc) || acc->count != count)
    return false;

  switch (targetType) {
    case AK_TARGET_POSITION:
    case AK_TARGET_SCALE:
      return outputComponents == 3u
             && (acc->componentCount == 3u || acc->componentCount == 6u);
    case AK_TARGET_ROTATE:
      return outputComponents == 4u
             && (acc->componentCount == 4u || acc->componentCount == 8u);
    case AK_TARGET_QUAT:
      return false;
    case AK_TARGET_FLOAT:
      if (outputComponents == 1u)
        return acc->componentCount == 1u || acc->componentCount == 2u;
      if (outputComponents == 16u)
        return acc->componentCount == 16u;
      return false;
    case AK_TARGET_UNKNOWN:
      return true;
    default:
      return acc->componentCount == outputComponents;
  }
}

static
bool
dae_anim_interp_accessor_supported(AkAccessor * __restrict acc,
                                   uint32_t                count) {
  size_t fillSize;
  size_t stride;

  if (!acc)
    return true;

  if (!acc->buffer
      || !acc->buffer->data
      || acc->componentType != AKT_UBYTE
      || acc->componentCount == 0
      || acc->count != count)
    return false;

  fillSize = acc->fillByteSize
             ? acc->fillByteSize
             : (size_t)acc->bytesPerComponent * acc->componentCount;
  stride   = acc->byteStride && acc->byteStride >= fillSize
             ? acc->byteStride
             : fillSize;

  return fillSize >= 1
         && stride >= fillSize
         && acc->byteOffset <= acc->buffer->length
         && (count == 0
             || (size_t)(count - 1u)
                  <= ((size_t)-1 - acc->byteOffset) / stride)
         && (count == 0
             || acc->byteOffset + (size_t)(count - 1u) * stride + fillSize
                  <= acc->buffer->length);
}

static
uint32_t
dae_anim_morph_target_count(DAEExpState * __restrict st,
                            AkChannel   * __restrict channel) {
  AkInstanceMorph *morpher;

  if (!st
      || !channel
      || channel->targetType != AK_TARGET_WEIGHTS
      || !channel->resolvedTarget
      || !channel->resolvedTarget->target)
    return 0;

  morpher = channel->resolvedTarget->target;
  if (!morpher->morph
      || dae_map_index(st->morphs, morpher->morph) == UINT32_MAX)
    return 0;

  return morpher->morph->targetCount;
}

static
bool
dae_anim_morph_output_split_supported(AkAccessor * __restrict acc,
                                      uint32_t                keyCount,
                                      uint32_t                targetCount) {
  if (!dae_anim_accessor_storage_supported(acc)
      || keyCount == 0
      || targetCount == 0)
    return false;

  if (acc->componentCount == targetCount && acc->count == keyCount)
    return true;

  return acc->componentCount == 1
         && keyCount   <= UINT32_MAX / targetCount
         && acc->count == keyCount * targetCount;
}

static
uint32_t
dae_anim_morph_split_count(AkChannel     * __restrict channel,
                           AkAnimSampler * __restrict sampler,
                           uint32_t                   targetCount) {
  AkAccessor *inputAcc;
  AkAccessor *outputAcc;
  uint32_t    keyCount;

  if (!channel
      || channel->targetType != AK_TARGET_WEIGHTS
      || !sampler
      || targetCount <= 1)
    return 0;

  inputAcc  = dae_anim_accessor(sampler, AK_INPUT_INPUT);
  outputAcc = dae_anim_accessor(sampler, AK_INPUT_OUTPUT);
  keyCount  = inputAcc ? inputAcc->count : 0;

  if (!dae_anim_morph_output_split_supported(outputAcc, keyCount, targetCount))
    return 0;

  if (outputAcc->count == keyCount && outputAcc->componentCount == 1)
    return 0;

  return targetCount;
}

static
uint32_t
dae_anim_sampler_morph_split_count(DAEExpState    * __restrict st,
                                   AkAnimation    * __restrict anim,
                                   AkAnimSampler  * __restrict sampler) {
  AkChannel *channel;
  uint32_t   splitCount;
  bool       hasSplit;
  bool       hasNormal;

  splitCount = 0;
  hasSplit   = false;
  hasNormal  = false;

  for (channel = anim ? anim->channel : NULL; channel; channel = channel->next) {
    uint32_t targetCount;
    uint32_t curSplitCount;

    if (ak_getObjectByUrl(&channel->source) != sampler)
      continue;

    targetCount   = dae_anim_morph_target_count(st, channel);
    curSplitCount = dae_anim_morph_split_count(channel, sampler, targetCount);
    if (curSplitCount > 0) {
      hasSplit = true;
      if (splitCount != 0 && splitCount != curSplitCount)
        return UINT32_MAX;
      splitCount = curSplitCount;
    } else {
      hasNormal = true;
    }
  }

  return hasSplit && hasNormal ? UINT32_MAX : splitCount;
}

static
bool
dae_anim_target_supported(DAEExpState * __restrict st,
                          AkChannel   * __restrict channel) {
  AkObject   *transformTarget;
  const char *sid;
  const char *target;

  target = channel ? channel->target : NULL;
  if (!target || !*target)
    goto resolved;

  if (ak_str_has_char_fast(target, '/'))
    return true;

  return channel->targetType == AK_TARGET_WEIGHTS
         && channel->resolvedTarget
         && channel->resolvedTarget->target
         && ak_str_has_char_fast(target, '(')
         && ak_str_has_char_fast(target, ')');

resolved:
  if (channel
      && channel->targetType == AK_TARGET_WEIGHTS
      && channel->resolvedTarget
      && channel->resolvedTarget->target)
    return true;

  transformTarget = dae_anim_transform_target(channel);
  sid             = dae_transform_sid(transformTarget);
  return transformTarget
         && sid
         && dae_node_for_transform(st, transformTarget) != NULL;
}

static
bool
dae_anim_sampler_supported(AkAnimSampler        * __restrict sampler,
                           AkTargetPropertyType              targetType,
                           uint32_t                          morphTargetCount) {
  AkAccessor *inputAcc;
  AkAccessor *outputAcc;
  AkAccessor *inTanAcc;
  AkAccessor *outTanAcc;
  AkAccessor *interpAcc;
  uint32_t    keyCount;

  inputAcc = dae_anim_accessor(sampler, AK_INPUT_INPUT);
  if (!inputAcc
      || !dae_anim_accessor_storage_supported(inputAcc)
      || inputAcc->count == 0
      || inputAcc->componentCount != 1)
    return false;

  keyCount = inputAcc->count;
  outputAcc = dae_anim_accessor(sampler, AK_INPUT_OUTPUT);

  if (targetType == AK_TARGET_WEIGHTS && morphTargetCount > 0) {
    if (!outputAcc
        || !dae_anim_accessor_storage_supported(outputAcc)
        || outputAcc->componentCount == 0)
      return false;

    inTanAcc  = dae_anim_accessor(sampler, AK_INPUT_IN_TANGENT);
    outTanAcc = dae_anim_accessor(sampler, AK_INPUT_OUT_TANGENT);
    if (inTanAcc || outTanAcc)
      return false;

    if (outputAcc->count == keyCount && outputAcc->componentCount == 1)
      goto interp;

    if (!dae_anim_morph_output_split_supported(outputAcc,
                                               keyCount,
                                               morphTargetCount))
      return false;

    goto interp;
  }

  if (!dae_anim_float_accessor_supported(outputAcc, keyCount, targetType))
    return false;

  inTanAcc = dae_anim_accessor(sampler, AK_INPUT_IN_TANGENT);
  if (targetType == AK_TARGET_QUAT && inTanAcc)
    return false;
  if (inTanAcc
      && !dae_anim_tangent_accessor_supported(inTanAcc,
                                               keyCount,
                                               targetType,
                                               outputAcc->componentCount))
    return false;

  outTanAcc = dae_anim_accessor(sampler, AK_INPUT_OUT_TANGENT);
  if (targetType == AK_TARGET_QUAT && outTanAcc)
    return false;
  if (outTanAcc
      && !dae_anim_tangent_accessor_supported(outTanAcc,
                                               keyCount,
                                               targetType,
                                               outputAcc->componentCount))
    return false;

interp:
  interpAcc = dae_anim_accessor(sampler, AK_INPUT_INTERPOLATION);
  if (!dae_anim_interp_accessor_supported(interpAcc, keyCount))
    return false;

  return true;
}

static
bool
dae_animation_supported(DAEExpState * __restrict st,
                        AkAnimation * __restrict anim) {
  AkAnimSampler *sampler;
  AkChannel     *channel;

  if (!anim)
    return true;

  for (sampler = anim->sampler;
       sampler;
       sampler = (AkAnimSampler *)sampler->base.next) {
    uint32_t splitCount;

    splitCount = dae_anim_sampler_morph_split_count(st, anim, sampler);
    if (splitCount == UINT32_MAX)
      return false;
    if (!dae_anim_sampler_supported(sampler,
                                    splitCount > 0
                                      ? AK_TARGET_WEIGHTS
                                      : AK_TARGET_UNKNOWN,
                                    splitCount))
      return false;
  }

  for (channel = anim->channel; channel; channel = channel->next) {
    AkAnimSampler        *sampler;
    AkTargetPropertyType  targetType;
    uint32_t              morphTargetCount;

    if (!dae_anim_target_supported(st, channel))
      return false;

    if (channel->targetType == AK_TARGET_WEIGHTS
        && channel->resolvedTarget
        && channel->resolvedTarget->target) {
      AkInstanceMorph *morpher;

      morpher = channel->resolvedTarget->target;
      if (!morpher->morph
          || dae_map_index(st->morphs, morpher->morph) == UINT32_MAX)
        return false;
    }

    sampler = ak_getObjectByUrl(&channel->source);
    targetType = dae_anim_channel_target_type(channel);
    morphTargetCount = dae_anim_morph_target_count(st, channel);
    if (!sampler
        || !dae_anim_sampler_supported(sampler,
                                       targetType,
                                       morphTargetCount))
      return false;
  }

  return true;
}

static
bool
dae_animation_tree_has_supported(DAEExpState * __restrict st,
                                 AkAnimation * __restrict anim) {
  for (; anim; anim = anim->next) {
    if (dae_animation_supported(st, anim)
        || dae_animation_tree_has_supported(st, anim->animation))
      return true;
  }

  return false;
}

static
void
dae_w_anim_id(DAEExpWriter * __restrict w,
              uint32_t                  animIdx) {
  dae_w_id(w, DAE_EXP_NAME(animation), animIdx);
}

static
void
dae_w_anim_sampler_id(DAEExpWriter * __restrict w,
                      uint32_t                  animIdx,
                      uint32_t                  samplerIdx) {
  dae_w_anim_id(w, animIdx);
  dae_w_lit(w, "_sampler_");
  dae_w_uint_fast(w, samplerIdx);
}

static
void
dae_w_anim_sampler_id_variant(DAEExpWriter * __restrict w,
                              uint32_t                  animIdx,
                              uint32_t                  samplerIdx,
                              uint32_t                  variantIdx) {
  dae_w_anim_sampler_id(w, animIdx, samplerIdx);
  if (variantIdx != UINT32_MAX) {
    dae_w_lit(w, "_weight_");
    dae_w_uint_fast(w, variantIdx);
  }
}

static
void
dae_w_anim_source_id(DAEExpWriter * __restrict w,
                     uint32_t                  animIdx,
                     uint32_t                  samplerIdx,
                     DAEExpName                semantic) {
  dae_w_anim_sampler_id_variant(w, animIdx, samplerIdx, UINT32_MAX);
  dae_w_ch(w, '_');
  dae_w_name(w, semantic);
}

static
void
dae_w_anim_source_id_variant(DAEExpWriter * __restrict w,
                             uint32_t                  animIdx,
                             uint32_t                  samplerIdx,
                             uint32_t                  variantIdx,
                             DAEExpName                semantic) {
  dae_w_anim_sampler_id_variant(w, animIdx, samplerIdx, variantIdx);
  dae_w_ch(w, '_');
  dae_w_name(w, semantic);
}

static
DAEExpName
dae_anim_float_param_name(AkInputSemantic       semantic,
                          AkTargetPropertyType targetType,
                          uint32_t             componentCount,
                          uint32_t             idx) {
  if (semantic == AK_INPUT_INPUT)
    return DAE_EXP_NAME_LIT("TIME");

  if ((semantic == AK_INPUT_IN_TANGENT
       || semantic == AK_INPUT_OUT_TANGENT)
      && (componentCount == 2u
          || (targetType == AK_TARGET_POSITION && componentCount == 6u)
          || (targetType == AK_TARGET_SCALE && componentCount == 6u)
          || (targetType == AK_TARGET_ROTATE && componentCount == 8u))) {
    uint32_t valueIndex;

    if ((idx & 1u) == 0u)
      return DAE_EXP_NAME_LIT("TIME");
    valueIndex = idx / 2u;
    if (targetType == AK_TARGET_ROTATE && valueIndex == 3u)
      return DAE_EXP_NAME_LIT("ANGLE");
    if (targetType == AK_TARGET_FLOAT)
      return DAE_EXP_NAME_LIT("VALUE");
    return dae_param_exp_name(valueIndex);
  }

  if ((targetType == AK_TARGET_ROTATE || targetType == AK_TARGET_QUAT)
      && idx == 3)
    return DAE_EXP_NAME_LIT("ANGLE");

  return dae_param_exp_name(idx);
}

static
DAEExpName
dae_anim_input_semantic_name(AkInputSemantic semantic) {
  switch (semantic) {
    case AK_INPUT_INPUT:         return DAE_EXP_NAME(INPUT);
    case AK_INPUT_OUTPUT:        return DAE_EXP_NAME(OUTPUT);
    case AK_INPUT_IN_TANGENT:    return DAE_EXP_NAME(IN_TANGENT);
    case AK_INPUT_OUT_TANGENT:   return DAE_EXP_NAME(OUT_TANGENT);
    case AK_INPUT_INTERPOLATION: return DAE_EXP_NAME(INTERPOLATION);
    default:                     return DAE_EXP_NAME_LIT("");
  }
}

static
DAEExpName
dae_anim_interpolation_name(AkInterpolationType interpolation) {
  switch (interpolation) {
    case AK_INTERPOLATION_BEZIER:   return DAE_EXP_NAME(BEZIER);
    case AK_INTERPOLATION_CARDINAL: return DAE_EXP_NAME(CARDINAL);
    case AK_INTERPOLATION_HERMITE:  return DAE_EXP_NAME(HERMITE);
    case AK_INTERPOLATION_BSPLINE:  return DAE_EXP_NAME(BSPLINE);
    case AK_INTERPOLATION_STEP:     return DAE_EXP_NAME(STEP);
    case AK_INTERPOLATION_LINEAR:
    case AK_INTERPOLATION_UNKNOWN:
    default:                        return DAE_EXP_NAME(LINEAR);
  }
}

static
uint8_t
dae_anim_interp_at(AkAccessor * __restrict acc, uint32_t index) {
  const unsigned char *src;
  size_t              fillSize;
  size_t              stride;

  fillSize = acc->fillByteSize
             ? acc->fillByteSize
             : (size_t)acc->bytesPerComponent * acc->componentCount;
  stride   = acc->byteStride && acc->byteStride >= fillSize
             ? acc->byteStride
             : fillSize;
  src      = (const unsigned char *)acc->buffer->data
             + acc->byteOffset
             + (size_t)index * stride;

  return src[0];
}

static
bool
dae_anim_quat_axis_angle_deg(const float * __restrict row,
                             vec3                     axis,
                             float      * __restrict angleDeg) {
  AkQuaternion quat;

  quat.val[0] = row[0];
  quat.val[1] = row[1];
  quat.val[2] = row[2];
  quat.val[3] = row[3];
  dae_quat_axis_angle_deg(&quat, axis, angleDeg);

  return isfinite(*angleDeg) && fabsf(*angleDeg) > 1.0e-5f;
}

static
const float*
dae_anim_float_row(AkAccessor * __restrict acc,
                   const float * __restrict scratch,
                   bool                      direct,
                   uint32_t                  componentCount,
                   uint32_t                  index) {
  return direct
         ? io_accessor_float_row(acc, index)
         : scratch + (size_t)index * componentCount;
}

static
bool
dae_write_anim_float_source_variant(DAEExpState    * __restrict st,
                                    AkAccessor     * __restrict acc,
                                    uint32_t                    animIdx,
                                    uint32_t                    samplerIdx,
                                    uint32_t                    variantIdx,
                                    AkInputSemantic             semantic,
                                    AkTargetPropertyType        targetType,
                                    bool                        scalarAngle) {
  DAEExpWriter *w;
  DAEExpName    semName;
  float        *scratch;
  uint32_t      i;
  uint32_t      c;
  uint32_t      componentCount;
  size_t        floatCount;
  bool          direct;
  bool          convertQuatOutput;
  bool          convertMatrixValues;
  bool          hasQuatAxisHint;
  vec3          quatAxisHint;

  if (!acc || acc->componentCount == 0)
    return false;

  if ((size_t)acc->count > (size_t)-1 / acc->componentCount)
    return false;
  floatCount = (size_t)acc->count * acc->componentCount;
  if (floatCount > (size_t)-1 / sizeof(float))
    return false;

  w              = &st->w;
  semName        = dae_anim_input_semantic_name(semantic);
  componentCount = acc->componentCount;
  direct         = io_accessor_float_direct(acc);
  scratch        = NULL;
  convertQuatOutput = semantic == AK_INPUT_OUTPUT
                      && targetType == AK_TARGET_QUAT
                      && componentCount == 4;
  convertMatrixValues = targetType == AK_TARGET_FLOAT
                        && componentCount == 16
                        && (semantic == AK_INPUT_OUTPUT
                            || semantic == AK_INPUT_IN_TANGENT
                            || semantic == AK_INPUT_OUT_TANGENT);
  hasQuatAxisHint = false;
  quatAxisHint[0] = 0.0f;
  quatAxisHint[1] = 0.0f;
  quatAxisHint[2] = 1.0f;

  if (!direct) {
    scratch = dae_scratch(st, sizeof(float) * floatCount);
    if (!scratch)
      return false;
    if (ak_accessorAsFloat(acc, scratch, floatCount) != floatCount)
      return false;
  }

  if (convertQuatOutput) {
    for (i = 0; i < acc->count; i++) {
      const float *row;
      vec3         axis;
      float        angleDeg;

      row = dae_anim_float_row(acc, scratch, direct, componentCount, i);
      if (dae_anim_quat_axis_angle_deg(row, axis, &angleDeg)) {
        glm_vec3_copy(axis, quatAxisHint);
        hasQuatAxisHint = true;
        break;
      }
    }
  }

  dae_w_lit(w, "<source id=\"");
  dae_w_anim_source_id_variant(w, animIdx, samplerIdx, variantIdx, semName);
  dae_w_lit(w, "\"><float_array id=\"");
  dae_w_anim_source_id_variant(w, animIdx, samplerIdx, variantIdx, semName);
  dae_w_lit(w, "_array\" count=\"");
  dae_w_uint_fast(w, floatCount);
  dae_w_lit(w, "\">");

  for (i = 0; i < acc->count; i++) {
    const float *row;

    float quatConverted[4];

    row = dae_anim_float_row(acc, scratch, direct, componentCount, i);

    if (convertQuatOutput) {
      vec3         axis;
      float        angleDeg;

      if (dae_anim_quat_axis_angle_deg(row, axis, &angleDeg)) {
        glm_vec3_copy(axis, quatAxisHint);
        hasQuatAxisHint = true;
      } else if (hasQuatAxisHint) {
        glm_vec3_copy(quatAxisHint, axis);
      }
      quatConverted[0] = axis[0];
      quatConverted[1] = axis[1];
      quatConverted[2] = axis[2];
      quatConverted[3] = angleDeg;
      row = quatConverted;
    }

    if (convertMatrixValues) {
      mat4 matrix;

      /* Animation matrices use AssetKit's internal column-major layout,
         while COLLADA float arrays store matrices row-major.  Mirror static
         matrix and skin export instead of serializing the raw accessor row. */
      if (i > 0)
        dae_w_ch(w, ' ');
      memcpy(matrix, row, sizeof(matrix));
      dae_w_matrix4x4_dae(w, matrix);
      continue;
    }

    for (c = 0; c < componentCount; c++) {
      float val;

      if (i > 0 || c > 0)
        dae_w_ch(w, ' ');

      val = row[c];
      if ((semantic == AK_INPUT_OUTPUT
           || semantic == AK_INPUT_IN_TANGENT
           || semantic == AK_INPUT_OUT_TANGENT)
          && targetType == AK_TARGET_ROTATE
          && ((componentCount == 4u && c == 3u)
              || (componentCount == 8u && c == 7u)))
        val = glm_deg(val);
      if (scalarAngle
          && (semantic == AK_INPUT_OUTPUT
              || semantic == AK_INPUT_IN_TANGENT
              || semantic == AK_INPUT_OUT_TANGENT)
          && ((componentCount == 1u && c == 0u)
              || (componentCount == 2u && c == 1u)))
        val = glm_deg(val);
      dae_w_float_fast(w, val);
    }
  }

  dae_w_lit(w, "</float_array><technique_common><accessor source=\"#");
  dae_w_anim_source_id_variant(w, animIdx, samplerIdx, variantIdx, semName);
  dae_w_lit(w, "_array\" count=\"");
  dae_w_uint_fast(w, acc->count);
  dae_w_lit(w, "\" stride=\"");
  dae_w_uint_fast(w, componentCount);
  dae_w_lit(w, "\">");

  for (c = 0; c < componentCount; c++) {
    dae_w_lit(w, "<param name=\"");
    dae_w_name(w, dae_anim_float_param_name(semantic,
                                            targetType,
                                            componentCount,
                                            c));
    dae_w_lit(w, "\" type=\"float\"/>");
  }

  dae_w_lit(w, "</accessor></technique_common></source>");

  return w->result == AK_OK;
}

static
bool
dae_write_anim_morph_output_source(DAEExpState * __restrict st,
                                   AkAccessor  * __restrict acc,
                                   uint32_t                 animIdx,
                                   uint32_t                 samplerIdx,
                                   uint32_t                 weightIdx,
                                   uint32_t                 keyCount,
                                   uint32_t                 targetCount) {
  DAEExpWriter *w;
  float        *scratch;
  uint32_t      i;
  uint32_t      componentCount;
  size_t        floatCount;
  bool          direct;
  bool          interleavedWeights;

  if (!acc
      || acc->componentCount == 0
      || weightIdx >= targetCount
      || !dae_anim_morph_output_split_supported(acc, keyCount, targetCount))
    return false;

  if ((size_t)acc->count > (size_t)-1 / acc->componentCount)
    return false;
  floatCount = (size_t)acc->count * acc->componentCount;
  if (floatCount > (size_t)-1 / sizeof(float))
    return false;

  w                  = &st->w;
  componentCount     = acc->componentCount;
  direct             = io_accessor_float_direct(acc);
  scratch            = NULL;
  interleavedWeights = acc->componentCount == targetCount
                       && acc->count == keyCount;

  if (!direct) {
    scratch = dae_scratch(st, sizeof(float) * floatCount);
    if (!scratch)
      return false;
    if (ak_accessorAsFloat(acc, scratch, floatCount) != floatCount)
      return false;
  }

  dae_w_lit(w, "<source id=\"");
  dae_w_anim_source_id_variant(w,
                               animIdx,
                               samplerIdx,
                               weightIdx,
                               DAE_EXP_NAME(OUTPUT));
  dae_w_lit(w, "\"><float_array id=\"");
  dae_w_anim_source_id_variant(w,
                               animIdx,
                               samplerIdx,
                               weightIdx,
                               DAE_EXP_NAME(OUTPUT));
  dae_w_lit(w, "_array\" count=\"");
  dae_w_uint_fast(w, keyCount);
  dae_w_lit(w, "\">");

  for (i = 0; i < keyCount; i++) {
    const float *row;
    float        val;

    if (i > 0)
      dae_w_ch(w, ' ');

    if (interleavedWeights) {
      row = direct
            ? io_accessor_float_row(acc, i)
            : scratch + (size_t)i * componentCount;
      val = row[weightIdx];
    } else {
      uint32_t flatIdx;

      flatIdx = i * targetCount + weightIdx;
      row     = direct
                ? io_accessor_float_row(acc, flatIdx)
                : scratch + (size_t)flatIdx * componentCount;
      val     = row[0];
    }

    dae_w_float_fast(w, val);
  }

  dae_w_lit(w, "</float_array><technique_common><accessor source=\"#");
  dae_w_anim_source_id_variant(w,
                               animIdx,
                               samplerIdx,
                               weightIdx,
                               DAE_EXP_NAME(OUTPUT));
  dae_w_lit(w, "_array\" count=\"");
  dae_w_uint_fast(w, keyCount);
  dae_w_lit(w, "\" stride=\"1\"><param name=\"MORPH_WEIGHT\" type=\"float\"/>"
               "</accessor></technique_common></source>");

  return w->result == AK_OK;
}

static
bool
dae_write_anim_interp_source_variant(DAEExpState    * __restrict st,
                                     AkAnimSampler  * __restrict sampler,
                                     uint32_t                    animIdx,
                                     uint32_t                    samplerIdx,
                                     uint32_t                    variantIdx) {
  DAEExpWriter       *w;
  AkAccessor         *inputAcc;
  AkAccessor         *interpAcc;
  AkInterpolationType interpolation;
  uint32_t            i;

  w         = &st->w;
  inputAcc  = dae_anim_accessor(sampler, AK_INPUT_INPUT);
  interpAcc = dae_anim_accessor(sampler, AK_INPUT_INTERPOLATION);
  interpolation = sampler->uniInterpolation == AK_INTERPOLATION_UNKNOWN
                  ? AK_INTERPOLATION_LINEAR
                  : sampler->uniInterpolation;

  dae_w_lit(w, "<source id=\"");
  dae_w_anim_source_id_variant(w,
                               animIdx,
                               samplerIdx,
                               variantIdx,
                               DAE_EXP_NAME(INTERPOLATION));
  dae_w_lit(w, "\"><Name_array id=\"");
  dae_w_anim_source_id_variant(w,
                               animIdx,
                               samplerIdx,
                               variantIdx,
                               DAE_EXP_NAME(INTERPOLATION));
  dae_w_lit(w, "_array\" count=\"");
  dae_w_uint_fast(w, inputAcc->count);
  dae_w_lit(w, "\">");

  for (i = 0; i < inputAcc->count; i++) {
    DAEExpName name;

    if (i > 0)
      dae_w_ch(w, ' ');

    name = interpAcc
           ? dae_anim_interpolation_name(dae_anim_interp_at(interpAcc, i))
           : dae_anim_interpolation_name(interpolation);
    dae_w_name(w, name);
  }

  dae_w_lit(w, "</Name_array><technique_common><accessor source=\"#");
  dae_w_anim_source_id_variant(w,
                               animIdx,
                               samplerIdx,
                               variantIdx,
                               DAE_EXP_NAME(INTERPOLATION));
  dae_w_lit(w, "_array\" count=\"");
  dae_w_uint_fast(w, inputAcc->count);
  dae_w_lit(w, "\" stride=\"1\"><param name=\"INTERPOLATION\" type=\"name\"/>"
               "</accessor></technique_common></source>");

  return w->result == AK_OK;
}

static
bool
dae_write_anim_interp_source(DAEExpState    * __restrict st,
                             AkAnimSampler  * __restrict sampler,
                             uint32_t                    animIdx,
                             uint32_t                    samplerIdx) {
  return dae_write_anim_interp_source_variant(st,
                                              sampler,
                                              animIdx,
                                              samplerIdx,
                                              UINT32_MAX);
}

static
uint32_t
dae_anim_sampler_index(AkAnimation    * __restrict anim,
                       AkAnimSampler  * __restrict sampler) {
  AkAnimSampler *it;
  uint32_t       idx;

  idx = 0;
  for (it = anim ? anim->sampler : NULL;
       it;
       it = (AkAnimSampler *)it->base.next, idx++) {
    if (it == sampler)
      return idx;
  }

  return UINT32_MAX;
}

static
AkTargetPropertyType
dae_anim_sampler_target_type(AkAnimation   * __restrict anim,
                             AkAnimSampler * __restrict sampler) {
  AkChannel *channel;

  for (channel = anim ? anim->channel : NULL; channel; channel = channel->next) {
    if (ak_getObjectByUrl(&channel->source) == sampler)
      return dae_anim_channel_target_type(channel);
  }

  return AK_TARGET_UNKNOWN;
}

static
bool
dae_anim_sampler_is_scalar_angle(AkAnimation   * __restrict anim,
                                 AkAnimSampler * __restrict sampler) {
  AkChannel *channel;

  for (channel = anim ? anim->channel : NULL; channel; channel = channel->next) {
    AkObject *target;

    if (ak_getObjectByUrl(&channel->source) != sampler
        || !channel->resolvedTarget
        || !channel->resolvedTarget->isPartial
        || channel->resolvedTarget->off != 3u)
      continue;
    target = channel->resolvedTarget->target;
    if (target && ak_typeid(target) == AKT_OBJECT && target->type == AKT_ROTATE)
      return true;
  }

  return false;
}

static
bool
dae_write_anim_sampler_source_variant(DAEExpState    * __restrict st,
                                      AkAnimSampler  * __restrict sampler,
                                      uint32_t                    animIdx,
                                      uint32_t                    samplerIdx,
                                      uint32_t                    variantIdx,
                                      AkInputSemantic             semantic,
                                      AkTargetPropertyType        targetType,
                                      bool                        scalarAngle) {
  AkAccessor *acc;

  acc = dae_anim_accessor(sampler, semantic);
  if (!acc)
    return true;

  return dae_write_anim_float_source_variant(st,
                                             acc,
                                             animIdx,
                                             samplerIdx,
                                             variantIdx,
                                             semantic,
                                             targetType,
                                             scalarAngle);
}

static
bool
dae_write_anim_sampler_source(DAEExpState    * __restrict st,
                              AkAnimSampler  * __restrict sampler,
                              uint32_t                    animIdx,
                              uint32_t                    samplerIdx,
                              AkInputSemantic             semantic,
                              AkTargetPropertyType        targetType,
                              bool                        scalarAngle) {
  return dae_write_anim_sampler_source_variant(st,
                                               sampler,
                                               animIdx,
                                               samplerIdx,
                                               UINT32_MAX,
                                               semantic,
                                               targetType,
                                               scalarAngle);
}

static
bool
dae_write_anim_sampler(DAEExpState    * __restrict st,
                       AkAnimation    * __restrict anim,
                       AkAnimSampler  * __restrict sampler,
                       uint32_t                    animIdx,
                       uint32_t                    samplerIdx) {
  DAEExpWriter *w;
  AkTargetPropertyType targetType;
  bool                 scalarAngle;

  targetType = dae_anim_sampler_target_type(anim, sampler);
  scalarAngle = dae_anim_sampler_is_scalar_angle(anim, sampler);

  if (!dae_write_anim_sampler_source(st,
                                     sampler,
                                     animIdx,
                                     samplerIdx,
                                     AK_INPUT_INPUT,
                                     targetType,
                                     scalarAngle)
      || !dae_write_anim_sampler_source(st,
                                        sampler,
                                        animIdx,
                                        samplerIdx,
                                        AK_INPUT_OUTPUT,
                                        targetType,
                                        scalarAngle)
      || !dae_write_anim_sampler_source(st,
                                        sampler,
                                        animIdx,
                                        samplerIdx,
                                        AK_INPUT_IN_TANGENT,
                                        targetType,
                                        scalarAngle)
      || !dae_write_anim_sampler_source(st,
                                        sampler,
                                        animIdx,
                                        samplerIdx,
                                        AK_INPUT_OUT_TANGENT,
                                        targetType,
                                        scalarAngle)
      || !dae_write_anim_interp_source(st, sampler, animIdx, samplerIdx)) {
    return false;
  }

  w = &st->w;
  dae_w_lit(w, "<sampler id=\"");
  dae_w_anim_sampler_id(w, animIdx, samplerIdx);
  dae_w_lit(w, "\">");

#define DAE_WRITE_ANIM_INPUT(SEM)                                             \
  do {                                                                        \
    if (SEM == AK_INPUT_INTERPOLATION || dae_anim_accessor(sampler, SEM)) {    \
      DAEExpName _semName = dae_anim_input_semantic_name(SEM);                \
      dae_w_lit(w, "<input semantic=\"");                                     \
      dae_w_name(w, _semName);                                                \
      dae_w_lit(w, "\" source=\"#");                                          \
      dae_w_anim_source_id(w, animIdx, samplerIdx, _semName);                 \
      dae_w_lit(w, "\"/>");                                                   \
    }                                                                         \
  } while (0)

  DAE_WRITE_ANIM_INPUT(AK_INPUT_INPUT);
  DAE_WRITE_ANIM_INPUT(AK_INPUT_OUTPUT);
  DAE_WRITE_ANIM_INPUT(AK_INPUT_IN_TANGENT);
  DAE_WRITE_ANIM_INPUT(AK_INPUT_OUT_TANGENT);
  DAE_WRITE_ANIM_INPUT(AK_INPUT_INTERPOLATION);

#undef DAE_WRITE_ANIM_INPUT

  dae_w_lit(w, "</sampler>");
  return w->result == AK_OK;
}

static
bool
dae_write_anim_morph_split_common_sources(DAEExpState    * __restrict st,
                                          AkAnimSampler  * __restrict sampler,
                                          uint32_t                    animIdx,
                                          uint32_t                    samplerIdx) {
  return dae_write_anim_sampler_source(st,
                                       sampler,
                                       animIdx,
                                       samplerIdx,
                                       AK_INPUT_INPUT,
                                       AK_TARGET_WEIGHTS,
                                       false)
         && dae_write_anim_interp_source(st, sampler, animIdx, samplerIdx);
}

static
bool
dae_write_anim_morph_split_sampler(DAEExpState    * __restrict st,
                                   AkAnimSampler  * __restrict sampler,
                                   uint32_t                    animIdx,
                                   uint32_t                    samplerIdx,
                                   uint32_t                    weightIdx,
                                   uint32_t                    targetCount) {
  DAEExpWriter *w;
  AkAccessor   *inputAcc;
  AkAccessor   *outputAcc;
  DAEExpName    semName;

  inputAcc  = dae_anim_accessor(sampler, AK_INPUT_INPUT);
  outputAcc = dae_anim_accessor(sampler, AK_INPUT_OUTPUT);
  if (!inputAcc || !outputAcc)
    return false;

  if (!dae_write_anim_morph_output_source(st,
                                             outputAcc,
                                             animIdx,
                                             samplerIdx,
                                             weightIdx,
                                             inputAcc->count,
                                             targetCount)) {
    return false;
  }

  w = &st->w;
  dae_w_lit(w, "<sampler id=\"");
  dae_w_anim_sampler_id_variant(w, animIdx, samplerIdx, weightIdx);
  dae_w_lit(w, "\">");

#define DAE_WRITE_MORPH_ANIM_INPUT(SEM, VARIANT)                              \
  do {                                                                        \
    semName = dae_anim_input_semantic_name(SEM);                              \
    dae_w_lit(w, "<input semantic=\"");                                       \
    dae_w_name(w, semName);                                                   \
    dae_w_lit(w, "\" source=\"#");                                            \
    dae_w_anim_source_id_variant(w, animIdx, samplerIdx, VARIANT, semName);   \
    dae_w_lit(w, "\"/>");                                                     \
  } while (0)

  DAE_WRITE_MORPH_ANIM_INPUT(AK_INPUT_INPUT, UINT32_MAX);
  DAE_WRITE_MORPH_ANIM_INPUT(AK_INPUT_OUTPUT, weightIdx);
  DAE_WRITE_MORPH_ANIM_INPUT(AK_INPUT_INTERPOLATION, UINT32_MAX);

#undef DAE_WRITE_MORPH_ANIM_INPUT

  dae_w_lit(w, "</sampler>");
  return w->result == AK_OK;
}

static
void
dae_write_animation(DAEExpState * __restrict st,
                    AkAnimation * __restrict anim,
                    uint32_t    * __restrict animIdx);

static
bool
dae_write_animation_channel_target_at(DAEExpState * __restrict st,
                                      AkChannel   * __restrict channel,
                                      uint32_t                 weightIdx) {
  DAEExpWriter *w;
  AkObject     *transformTarget;

  w = &st->w;

  if (channel->targetType == AK_TARGET_WEIGHTS
      && channel->resolvedTarget
      && channel->resolvedTarget->target) {
    AkInstanceMorph *morpher;
    uint32_t         morphIdx;
    uint32_t         targetIdx;

    morpher  = channel->resolvedTarget->target;
    morphIdx = morpher && morpher->morph
                 ? dae_map_index(st->morphs, morpher->morph)
                 : UINT32_MAX;
    if (morphIdx == UINT32_MAX)
      return false;

    targetIdx = weightIdx == UINT32_MAX
                ? channel->resolvedTarget->off
                : weightIdx;
    dae_w_lit(w, "morph_");
    dae_w_uint_fast(w, morphIdx);
    dae_w_lit(w, "_weights(");
    dae_w_uint_fast(w, targetIdx);
    dae_w_ch(w, ')');
    return true;
  }

  transformTarget = dae_anim_transform_target(channel);
  if (transformTarget
      && transformTarget->type == AKT_MATRIX
      && channel->target
      && channel->resolvedTarget
      && channel->resolvedTarget->isPartial
      && channel->resolvedTarget->off < 16u) {
    const char *open;

    open = strchr(channel->target, '(');
    if (open) {
      size_t prefixLen;
      char  *target;
      uint32_t off;

      prefixLen = (size_t)(open - channel->target);
      target = dae_scratch(st, prefixLen + 7u);
      if (!target) {
        w->result = AK_ENOMEM;
        return false;
      }

      off = channel->resolvedTarget->off;
      memcpy(target, channel->target, prefixLen);
      target[prefixLen + 0u] = '(';
      target[prefixLen + 1u] = (char)('0' + off % 4u);
      target[prefixLen + 2u] = ')';
      target[prefixLen + 3u] = '(';
      target[prefixLen + 4u] = (char)('0' + off / 4u);
      target[prefixLen + 5u] = ')';
      target[prefixLen + 6u] = '\0';
      dae_w_xml(w, target, true);
      return true;
    }
  }

  if (transformTarget
      && channel->target
      && channel->resolvedTarget
      && channel->resolvedTarget->isPartial
      && (transformTarget->type == AKT_TRANSLATE
          || transformTarget->type == AKT_SCALE
          || transformTarget->type == AKT_ROTATE)
      && channel->resolvedTarget->off < 3u) {
    const char *dot;

    /* Coordinate conversion may remap a partial transform channel to a
       different axis while retaining the source target string as provenance.
       Serialize the resolved slot so roundtrip behavior matches runtime
       consumers of ak_channelTarget(). */
    dot = strrchr(channel->target, '.');
    if (dot) {
      static const char axes[] = {'X', 'Y', 'Z'};
      size_t            prefixLen;
      char             *target;

      prefixLen = (size_t)(dot - channel->target + 1);
      target = dae_scratch(st, prefixLen + 2u);
      if (!target) {
        w->result = AK_ENOMEM;
        return false;
      }
      memcpy(target, channel->target, prefixLen);
      target[prefixLen]     = axes[channel->resolvedTarget->off];
      target[prefixLen + 1] = '\0';
      dae_w_xml(w, target, true);
      return true;
    }
  }

  if (transformTarget && !channel->target) {
    AkNode *node;

    node = dae_node_for_transform(st, transformTarget);
    if (!node || !dae_w_node_id_ref(st, node))
      return false;

    dae_w_ch(w, '/');
    dae_w_transform_sid(w, transformTarget);
    return true;
  }

  dae_w_xml(w, channel->target, true);
  return true;
}

static
bool
dae_write_animation_channel_target(DAEExpState * __restrict st,
                                   AkChannel   * __restrict channel) {
  return dae_write_animation_channel_target_at(st, channel, UINT32_MAX);
}

static
void
dae_write_animation_one(DAEExpState * __restrict st,
                        AkAnimation * __restrict anim,
                        uint32_t                 animIdx,
                        uint32_t    * __restrict nextAnimIdx) {
  DAEExpWriter  *w;
  AkAnimSampler *sampler;
  AkChannel     *channel;
  AkAnimation   *sub;
  uint32_t       samplerIdx;

  w = &st->w;
  dae_w_lit(w, "<animation id=\"");
  dae_w_anim_id(w, animIdx);
  if (anim->name) {
    dae_w_lit(w, "\" name=\"");
    dae_w_xml(w, anim->name, true);
  }
  dae_w_lit(w, "\">");

  samplerIdx = 0;
  for (sampler = anim->sampler;
       sampler;
       sampler = (AkAnimSampler *)sampler->base.next, samplerIdx++) {
    uint32_t splitCount;

    splitCount = dae_anim_sampler_morph_split_count(st, anim, sampler);
    if (splitCount == UINT32_MAX) {
      w->result = AK_EINVAL;
      return;
    }

    if (splitCount > 0) {
      uint32_t weightIdx;

      if (!dae_write_anim_morph_split_common_sources(st,
                                                     sampler,
                                                     animIdx,
                                                     samplerIdx)) {
        if (w->result == AK_OK)
          w->result = AK_EINVAL;
        return;
      }

      for (weightIdx = 0; weightIdx < splitCount; weightIdx++) {
        if (!dae_write_anim_morph_split_sampler(st,
                                                sampler,
                                                animIdx,
                                                samplerIdx,
                                                weightIdx,
                                                splitCount)) {
          if (w->result == AK_OK)
            w->result = AK_EINVAL;
          return;
        }
      }
    } else {
      if (!dae_write_anim_sampler(st, anim, sampler, animIdx, samplerIdx)) {
        if (w->result == AK_OK)
          w->result = AK_EINVAL;
        return;
      }
    }
  }

  for (channel = anim->channel; channel; channel = channel->next) {
    AkAnimSampler *channelSampler;
    uint32_t       channelSamplerIdx;
    uint32_t       splitCount;

    channelSampler    = ak_getObjectByUrl(&channel->source);
    channelSamplerIdx = dae_anim_sampler_index(anim, channelSampler);
    if (channelSamplerIdx == UINT32_MAX) {
      w->result = AK_EINVAL;
      return;
    }

    splitCount = dae_anim_morph_split_count(channel,
                                            channelSampler,
                                            dae_anim_morph_target_count(st,
                                                                        channel));
    if (splitCount > 0) {
      uint32_t weightIdx;

      for (weightIdx = 0; weightIdx < splitCount; weightIdx++) {
        dae_w_lit(w, "<channel source=\"#");
        dae_w_anim_sampler_id_variant(w,
                                      animIdx,
                                      channelSamplerIdx,
                                      weightIdx);
        dae_w_lit(w, "\" target=\"");
        if (!dae_write_animation_channel_target_at(st, channel, weightIdx)) {
          w->result = AK_EINVAL;
          return;
        }
        dae_w_lit(w, "\"/>");
      }
    } else {
      dae_w_lit(w, "<channel source=\"#");
      dae_w_anim_sampler_id(w, animIdx, channelSamplerIdx);
      dae_w_lit(w, "\" target=\"");
      if (!dae_write_animation_channel_target(st, channel)) {
        w->result = AK_EINVAL;
        return;
      }
      dae_w_lit(w, "\"/>");
    }
  }

  for (sub = anim->animation; sub; sub = sub->next)
    dae_write_animation(st, sub, nextAnimIdx);

  dae_write_extra(w, anim->extra);
  dae_w_lit(w, "</animation>");
}

static
void
dae_write_animation(DAEExpState * __restrict st,
                    AkAnimation * __restrict anim,
                    uint32_t    * __restrict animIdx) {
  AkAnimation *sub;
  uint32_t current;

  if (!dae_animation_supported(st, anim)) {
    for (sub = anim ? anim->animation : NULL; sub; sub = sub->next)
      dae_write_animation(st, sub, animIdx);
    return;
  }

  current = (*animIdx)++;
  dae_write_animation_one(st, anim, current, animIdx);
}

AK_HIDE
void
dae_write_library_animations(DAEExpState * __restrict st) {
  DAEExpWriter *w;
  AkAnimation  *anim;
  uint32_t      animIdx;

  if (!st
      || !st->doc
      || !dae_animation_tree_has_supported(st,
                                           st->doc->lib.animations.first))
    return;

  w = &st->w;
  dae_w_lit(w, "<library_animations>");
  animIdx = 0;
  for (anim = st->doc->lib.animations.first; anim; anim = anim->next)
    dae_write_animation(st, anim, &animIdx);
  dae_w_lit(w, "</library_animations>\n");
}
