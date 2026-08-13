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

#include "angle.h"
#include "../../../accessor.h"

/*
 COLLADA uses degrees for all angles; convert degrees to radians. They may
 appear in these places:

 1. Rotate element - fixed in place
 2. Perspective (xfov, yfov) - fixed in place
 3. Light (fallofAngle) - fixed in place
 4. Skew element - fixed in place
 5. Animation output and tangent data - fixed below

 */

static
void
dae_cvtAnglesAt(AkAccessor * __restrict acc,
                AkBuffer   * __restrict buff,
                uint32_t                paramIndex) {
  AkAccessorDAE *accdae;
  unsigned char *base;
  size_t         i, count, st, off, last, lastByte;

  if (!acc || !buff || !buff->data || !(accdae = ak_userData(acc)))
    return;

  acc->componentType = (AkTypeId)(uintptr_t)ak_userData(buff);
  if (acc->componentType != AKT_FLOAT)
    return;

  st = accdae->stride ? accdae->stride : 1;
  if (paramIndex >= st)
    return;

  if ((size_t)accdae->offset > (size_t)-1 - paramIndex)
    return;
  off   = (size_t)accdae->offset + paramIndex;
  count = acc->count;
  if (count == 0u)
    return;

  if ((count - 1u) > ((size_t)-1 - off) / st)
    return;
  last = off + (count - 1u) * st;
  if (last > ((size_t)-1 - sizeof(float)) / sizeof(float))
    return;
  lastByte = last * sizeof(float);
  if (lastByte > buff->length
      || sizeof(float) > buff->length - lastByte)
    return;

  base = buff->data;
  for (i = 0; i < count; i++) {
    unsigned char *valuePtr;
    float          value;

    valuePtr = base + (off + i * st) * sizeof(float);
    memcpy(&value, valuePtr, sizeof(value));
    value = glm_rad(value);
    memcpy(valuePtr, &value, sizeof(value));
  }
}

AK_HIDE
void
dae_cvtAngles(AkAccessor * __restrict acc,
              AkBuffer   * __restrict buff,
              const char * __restrict paramName) {
  AkAccessorDAE *accdae;
  AkDataParam   *param;
  uint32_t       index;
  
  if (!(accdae = ak_userData(acc)))
    return;

  index = 0;
  param = accdae->param;
  while (param) {
    if (param->name && strcasecmp(param->name, paramName) == 0)
      dae_cvtAnglesAt(acc, buff, index);

    index++;
    param = param->next;
  }
}

static
bool
dae_angleAccessorSeen(DAEState   * __restrict dst,
                      AkAccessor * __restrict acc,
                      bool                    mark) {
  FListItem *item;

  if (!dst || !acc)
    return true;

  for (item = dst->radianAccessors; item; item = item->next) {
    if (item->data == acc)
      return true;
  }

  if (mark)
    flist_sp_insert(&dst->radianAccessors, acc);
  return false;
}

static
void
dae_fixAngleTangent(DAEState * __restrict dst,
                    AkInput  * __restrict inp,
                    uint32_t              outputAngleIndex,
                    uint32_t              outputStride) {
  AkAccessor    *acc;
  AkAccessorDAE *accdae;
  AkBuffer      *buff;
  uint32_t       st;
  uint32_t       idx;

  if (!inp
      || !(acc = inp->accessor)
      || dae_angleAccessorSeen(dst, acc, false)
      || !(accdae = ak_userData(acc))
      || !(buff = ak_getObjectByUrl(&accdae->source)))
    return;

  st = accdae->stride ? accdae->stride : 1;

  if (st == outputStride) {
    idx = outputAngleIndex;
  } else if (st >= outputStride * 2
             && outputAngleIndex * 2 + 1 < st) {
    /* Bezier-style tangents are usually (time, value) pairs. Only the
       value component is angular; the time component stays seconds. */
    idx = outputAngleIndex * 2 + 1;
  } else if (st == 1) {
    idx = 0;
  } else if (outputAngleIndex < st) {
    idx = outputAngleIndex;
  } else {
    return;
  }

  dae_cvtAnglesAt(acc, buff, idx);
  dae_angleAccessorSeen(dst, acc, true);
}

/* TODO: This works for BERZIER but HERMITE?? */
AK_HIDE
void
dae_fixAngles(DAEState * __restrict dst) {
  /* TODO: */
  FListItem     *item;
  AkAnimSampler *sampler;
  AkDataParam   *param;
  AkAccessor    *acc;
  AkBuffer      *buff;
  AkAccessorDAE *accdae;
  uint32_t       index, outStride;
  
  item = dst->toRadiansSampelers;
  while (item) {
    sampler = item->data;
    acc     = NULL;
    buff    = NULL;

    if ((acc = sampler->outputInput->accessor)
        && (accdae = ak_userData(acc))
        && (buff = ak_getObjectByUrl(&accdae->source))) {
      bool foundAngle;

      foundAngle = false;
      index      = 0;
      outStride  = accdae->stride ? accdae->stride : 1;

      if ((param = accdae->param)) {
        do {
          if (param->name && strcasecmp(param->name, _s_dae_angle) == 0) {
            foundAngle = true;
            break;
          }
          
          index++;
        } while ((param = param->next));
      }

      if (!foundAngle)
        goto nxt_sampler;

      if (!dae_angleAccessorSeen(dst, acc, false)) {
        dae_cvtAngles(acc, buff, _s_dae_angle);
        dae_angleAccessorSeen(dst, acc, true);
      }

      dae_fixAngleTangent(dst, sampler->inTangentInput,  index, outStride);
      dae_fixAngleTangent(dst, sampler->outTangentInput, index, outStride);
    }

  nxt_sampler:
    item = item->next;
  }

  flist_sp_destroy(&dst->toRadiansSampelers);
}

static
AkSpotLight*
dae_spotFalloffChannel(DAEState  * __restrict dst,
                       AkChannel * __restrict channel) {
  AkContext   context;
  AkLight    *light;
  const char *targetSid;
  const char *slash;
  const char *attribute;
  void       *target;

  if (!dst || !dst->doc || !channel || !channel->target)
    return NULL;

  slash     = strrchr(channel->target, '/');
  if (!slash || slash == channel->target)
    return NULL;
  targetSid = slash + 1;
  {
    char   *id;
    size_t  idLen;
    void   *lightObject;

    idLen = (size_t)(slash - channel->target);
    id    = malloc(idLen + 1u);
    if (!id)
      return NULL;
    memcpy(id, channel->target, idLen);
    id[idLen] = '\0';
    lightObject = ak_getObjectById(dst->doc, id);
    free(id);
    if (!lightObject || ak_typeid(lightObject) != AKT_LIGHT)
      return NULL;
  }

  context     = AkContextZeroed();
  context.doc = dst->doc;
  attribute = NULL;
  target    = ak_sid_resolve(&context, channel->target, &attribute);
  if (attribute)
    ak_free((void *)attribute);
  if (!target)
    return NULL;

  for (light = dst->doc->lib.lights.first; light; light = light->next) {
    AkSpotLight *spot;
    const char  *sid;

    if (!light->data || light->data->type != AK_LIGHT_TYPE_SPOT)
      continue;
    spot = (AkSpotLight *)light->data;
    sid  = ak_sid_geta(spot, &spot->outerConeAngle);
    if (target == spot && sid && strcmp(targetSid, sid) == 0)
      return spot;
  }

  return NULL;
}

static
void
dae_spotSamplerUsageWalk(DAEState     * __restrict dst,
                         AkAnimation  * __restrict anim,
                         AkAnimSampler * __restrict sampler,
                         uint32_t     * __restrict spotCount,
                         uint32_t     * __restrict otherCount) {
  for (; anim; anim = anim->next) {
    AkChannel *channel;

    for (channel = anim->channel; channel; channel = channel->next) {
      if (ak_getObjectByUrl(&channel->source) != sampler)
        continue;
      if (dae_spotFalloffChannel(dst, channel))
        (*spotCount)++;
      else
        (*otherCount)++;
    }
    if (anim->animation)
      dae_spotSamplerUsageWalk(dst,
                               anim->animation,
                               sampler,
                               spotCount,
                               otherCount);
  }
}

static
AkAccessor*
dae_spotCloneAccessor(void       * __restrict parent,
                      AkAccessor * __restrict source,
                      DAEState   * __restrict dst) {
  AkHeap     *heap;
  AkAccessor *clone;
  AkBuffer   *sourceBuffer;
  AkBuffer   *cloneBuffer;

  if (!parent
      || !source
      || !(heap = ak_heap_getheap(parent))
      || !(clone = ak_accessor_dup(source)))
    return NULL;

  ak_heap_setpm(clone, parent);
  clone->next = NULL;
  sourceBuffer = source->buffer;
  cloneBuffer  = NULL;
  if (sourceBuffer) {
    cloneBuffer = ak_heap_calloc(heap, clone, sizeof(*cloneBuffer));
    if (!cloneBuffer) {
      ak_free(clone);
      return NULL;
    }
    *cloneBuffer      = *sourceBuffer;
    cloneBuffer->next = NULL;
    cloneBuffer->data = NULL;
    if (sourceBuffer->data && sourceBuffer->length > 0u) {
      cloneBuffer->data = ak_heap_alloc(heap,
                                        cloneBuffer,
                                        sourceBuffer->length);
      if (!cloneBuffer->data) {
        ak_free(clone);
        return NULL;
      }
      memcpy(cloneBuffer->data,
             sourceBuffer->data,
             sourceBuffer->length);
    }
  }
  clone->buffer = cloneBuffer;
  if (dae_angleAccessorSeen(dst, source, false))
    dae_angleAccessorSeen(dst, clone, true);
  return clone;
}

static
AkAnimSampler*
dae_spotCloneSampler(DAEState      * __restrict dst,
                     AkAnimation   * __restrict anim,
                     AkChannel     * __restrict channel,
                     AkAnimSampler * __restrict sampler) {
  AkHeap        *heap;
  AkAnimSampler *clone;
  AkInput       *sourceInput;
  AkInput       *cloneInput;
  AkInput       *lastInput;

  if (!anim
      || !channel
      || !sampler
      || !(heap = ak_heap_getheap(channel)))
    return NULL;

  clone = ak_heap_calloc(heap, anim, sizeof(*clone));
  if (!clone)
    return NULL;
  *clone                 = *sampler;
  clone->input           = NULL;
  clone->inputInput      = NULL;
  clone->outputInput     = NULL;
  clone->interpInput     = NULL;
  clone->inTangentInput  = NULL;
  clone->outTangentInput = NULL;
  clone->base.next       = NULL;
  lastInput              = NULL;

  for (sourceInput = sampler->input;
       sourceInput;
       sourceInput = sourceInput->next) {
    AkAccessor *cloneAccessor;

    cloneInput = ak_heap_calloc(heap, clone, sizeof(*cloneInput));
    if (!cloneInput) {
      ak_free(clone);
      return NULL;
    }
    *cloneInput      = *sourceInput;
    cloneInput->next = NULL;
    cloneAccessor    = NULL;
    if (sourceInput->accessor
        && !(cloneAccessor = dae_spotCloneAccessor(cloneInput,
                                                   sourceInput->accessor,
                                                   dst))) {
      ak_free(clone);
      return NULL;
    }
    cloneInput->accessor = cloneAccessor;
    if (lastInput)
      lastInput->next = cloneInput;
    else
      clone->input = cloneInput;
    lastInput = cloneInput;

    if (sourceInput == sampler->inputInput)
      clone->inputInput = cloneInput;
    if (sourceInput == sampler->outputInput)
      clone->outputInput = cloneInput;
    if (sourceInput == sampler->interpInput)
      clone->interpInput = cloneInput;
    if (sourceInput == sampler->inTangentInput)
      clone->inTangentInput = cloneInput;
    if (sourceInput == sampler->outTangentInput)
      clone->outTangentInput = cloneInput;
  }

  clone->base.next     = (AkOneWayIterBase *)anim->sampler;
  anim->sampler        = clone;
  channel->source.ptr  = clone;
  return clone;
}

static
bool
dae_spotAccessorLayout(AkAccessor * __restrict accessor,
                       bool                    tangent,
                       size_t                 *strideOut,
                       size_t                 *valueOffsetOut) {
  size_t         stride;
  size_t         valueOffset;
  size_t         last;

  if (!accessor)
    return true;
  if (accessor->componentType != AKT_FLOAT
      || accessor->bytesPerComponent != sizeof(float)
      || !accessor->buffer
      || !accessor->buffer->data
      || accessor->componentCount == 0u
      || accessor->componentCount > 2u)
    return false;

  valueOffset = tangent && accessor->componentCount == 2u
                ? sizeof(float)
                : 0u;
  stride = accessor->byteStride
           ? accessor->byteStride
           : (size_t)accessor->componentCount * sizeof(float);
  if (stride < valueOffset + sizeof(float)
      || accessor->byteOffset > accessor->buffer->length
      || (accessor->count > 0u
          && (size_t)(accessor->count - 1u)
               > ((size_t)-1 - accessor->byteOffset) / stride))
    return false;

  last = accessor->byteOffset;
  if (accessor->count > 0u)
    last += (size_t)(accessor->count - 1u) * stride;
  if (last > accessor->buffer->length
      || (accessor->count > 0u
          && valueOffset + sizeof(float) > accessor->buffer->length - last))
    return false;

  if (strideOut)
    *strideOut = stride;
  if (valueOffsetOut)
    *valueOffsetOut = valueOffset;
  return true;
}

static
bool
dae_spotScaleAccessor(DAEState   * __restrict dst,
                      AkAccessor * __restrict accessor,
                      bool                    tangent) {
  unsigned char *base;
  size_t         stride;
  size_t         valueOffset;
  float          scale;
  uint32_t       i;

  if (!accessor)
    return true;
  if (!dae_spotAccessorLayout(accessor, tangent, &stride, &valueOffset))
    return false;

  /* Generic ANGLE accessors were already converted from degrees to radians.
     Nonstandard exporters sometimes name the scalar VALUE instead; convert
     those here based on the resolved light target. */
  scale = dae_angleAccessorSeen(dst, accessor, false)
          ? 0.5f
          : GLM_PI / 360.0f;
  base = (unsigned char *)accessor->buffer->data
         + accessor->byteOffset
         + valueOffset;
  for (i = 0u; i < accessor->count; i++) {
    float value;

    memcpy(&value, base + (size_t)i * stride, sizeof(value));
    value *= scale;
    memcpy(base + (size_t)i * stride, &value, sizeof(value));
  }
  dae_angleAccessorSeen(dst, accessor, true);
  return true;
}

static
bool
dae_spotScaleSampler(DAEState     * __restrict dst,
                     AkAnimSampler * __restrict sampler) {
  AkAccessor *output;
  AkAccessor *inTangent;
  AkAccessor *outTangent;

  if (!sampler)
    return false;
  output = sampler->outputInput ? sampler->outputInput->accessor : NULL;
  inTangent = sampler->inTangentInput
              ? sampler->inTangentInput->accessor
              : NULL;
  outTangent = sampler->outTangentInput
               ? sampler->outTangentInput->accessor
               : NULL;
  if (!dae_spotAccessorLayout(output, false, NULL, NULL)
      || !dae_spotAccessorLayout(inTangent, true, NULL, NULL)
      || !dae_spotAccessorLayout(outTangent, true, NULL, NULL))
    return false;
  return dae_spotScaleAccessor(dst, output, false)
         && dae_spotScaleAccessor(dst, inTangent, true)
         && dae_spotScaleAccessor(dst, outTangent, true);
}

static
bool
dae_spotDetachSamplerData(DAEState     * __restrict dst,
                          AkAnimSampler * __restrict sampler) {
  AkInput *inputs[3];
  size_t   i;

  if (!sampler)
    return false;
  inputs[0] = sampler->outputInput;
  inputs[1] = sampler->inTangentInput;
  inputs[2] = sampler->outTangentInput;
  for (i = 0u; i < AK_ARRAY_LEN(inputs); i++) {
    AkAccessor *clone;

    if (!inputs[i] || !inputs[i]->accessor)
      continue;
    clone = dae_spotCloneAccessor(inputs[i], inputs[i]->accessor, dst);
    if (!clone)
      return false;
    inputs[i]->accessor = clone;
  }
  return true;
}

static
void
dae_fixSpotFalloffAnglesWalk(DAEState    * __restrict dst,
                             AkAnimation * __restrict anim,
                             FListItem  ** __restrict processed) {
  for (; anim; anim = anim->next) {
    AkChannel *channel;

    for (channel = anim->channel; channel; channel = channel->next) {
      AkSpotLight   *spot;
      AkAnimSampler *sampler;
      FListItem     *item;
      uint32_t       spotCount;
      uint32_t       otherCount;

      spot = dae_spotFalloffChannel(dst, channel);
      if (!spot)
        continue;

      if (!channel->resolvedTarget) {
        AkResolvedTarget *resolved;

        resolved = ak_heap_calloc(dst->heap, channel, sizeof(*resolved));
        if (resolved) {
          resolved->target       = &spot->outerConeAngle;
          channel->resolvedTarget = resolved;
          channel->targetType     = AK_TARGET_FLOAT;
        }
      }

      sampler = ak_getObjectByUrl(&channel->source);
      if (!sampler)
        continue;

      spotCount  = 0u;
      otherCount = 0u;
      dae_spotSamplerUsageWalk(dst,
                               dst->doc->lib.animations.first,
                               sampler,
                               &spotCount,
                               &otherCount);
      if (otherCount > 0u) {
        sampler = dae_spotCloneSampler(dst, anim, channel, sampler);
        if (sampler)
          dae_spotScaleSampler(dst, sampler);
        continue;
      }

      for (item = *processed; item; item = item->next) {
        if (item->data == sampler)
          break;
      }
      if (item)
        continue;
      if (spotCount > 0u
          && dae_spotDetachSamplerData(dst, sampler)
          && dae_spotScaleSampler(dst, sampler))
        flist_sp_insert(processed, sampler);
    }

    if (anim->animation)
      dae_fixSpotFalloffAnglesWalk(dst, anim->animation, processed);
  }
}

AK_HIDE
void
dae_fixSpotFalloffAngles(DAEState * __restrict dst) {
  FListItem *processed;

  if (!dst || !dst->doc || !dst->doc->lib.animations.first)
    return;
  processed = NULL;
  dae_fixSpotFalloffAnglesWalk(dst,
                               dst->doc->lib.animations.first,
                               &processed);
  flist_sp_destroy(&processed);
}

static
void
dae_fixPartialRotateAngleAccessor(AkAccessor * __restrict acc,
                                  bool                     tangent) {
  unsigned char *base;
  size_t         stride, valueOffset, last;
  uint32_t       i;

  if (!acc
      || acc->componentType != AKT_FLOAT
      || acc->bytesPerComponent != sizeof(float)
      || !acc->buffer
      || !acc->buffer->data
      || acc->componentCount == 0u)
    return;

  valueOffset = tangent && acc->componentCount == 2u ? sizeof(float) : 0u;
  stride = acc->byteStride
           ? acc->byteStride
           : (size_t)acc->componentCount * sizeof(float);
  if (stride < valueOffset + sizeof(float)
      || acc->byteOffset > acc->buffer->length
      || (acc->count > 0u
          && (size_t)(acc->count - 1u)
               > ((size_t)-1 - acc->byteOffset) / stride))
    return;

  last = acc->byteOffset;
  if (acc->count > 0u)
    last += (size_t)(acc->count - 1u) * stride;
  if (last > acc->buffer->length
      || (acc->count > 0u
          && valueOffset + sizeof(float) > acc->buffer->length - last))
    return;

  base = (unsigned char *)acc->buffer->data + acc->byteOffset + valueOffset;
  for (i = 0; i < acc->count; i++) {
    float value;

    memcpy(&value, base + (size_t)i * stride, sizeof(value));
    value = glm_rad(value);
    memcpy(base + (size_t)i * stride, &value, sizeof(value));
  }
}

static
void
dae_fixPartialRotateAnglesWalk(DAEState   * __restrict dst,
                               AkAnimation * __restrict anim) {
  for (; anim; anim = anim->next) {
    AkChannel *channel;

    for (channel = anim->channel; channel; channel = channel->next) {
      AkResolvedTarget *resolved;
      AkObject         *target;
      AkAnimSampler    *sampler;

      resolved = channel->resolvedTarget;
      target   = resolved ? resolved->target : NULL;
      if (!resolved
          || !resolved->isPartial
          || resolved->off != 3u
          || !target
          || ak_typeid(target) != AKT_OBJECT
          || target->type != AKT_ROTATE)
        continue;

      sampler = ak_getObjectByUrl(&channel->source);
      if (!sampler)
        continue;

      {
        AkAccessor *acc;

        acc = sampler->outputInput ? sampler->outputInput->accessor : NULL;
        if (!dae_angleAccessorSeen(dst, acc, false)) {
          dae_fixPartialRotateAngleAccessor(acc, false);
          dae_angleAccessorSeen(dst, acc, true);
        }
        acc = sampler->inTangentInput
              ? sampler->inTangentInput->accessor : NULL;
        if (!dae_angleAccessorSeen(dst, acc, false)) {
          dae_fixPartialRotateAngleAccessor(acc, true);
          dae_angleAccessorSeen(dst, acc, true);
        }
        acc = sampler->outTangentInput
              ? sampler->outTangentInput->accessor : NULL;
        if (!dae_angleAccessorSeen(dst, acc, false)) {
          dae_fixPartialRotateAngleAccessor(acc, true);
          dae_angleAccessorSeen(dst, acc, true);
        }
      }
    }

    if (anim->animation)
      dae_fixPartialRotateAnglesWalk(dst, anim->animation);
  }
}

AK_HIDE
void
dae_fixPartialRotateAngles(DAEState * __restrict dst) {
  if (dst && dst->doc) {
    dae_fixPartialRotateAnglesWalk(dst, dst->doc->lib.animations.first);
    flist_sp_destroy(&dst->radianAccessors);
  }
}
