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
