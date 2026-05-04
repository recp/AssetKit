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
 COLLADA uses degress for all angles, convert desgress to radians. It may exists
 in these places as far as I know:

 1. Rotate element - fixed in place
 2. Perspective (xfov, yfov) - fixed in place
 3. Light (fallofAngle) - fixed in place
 4. Skew element - fixed in place
 5. Animation data (output, tangents...)!!!! - NEEDS TO BE FIXED?

 */

static
void
dae_cvtAnglesAt(AkAccessor * __restrict acc,
                AkBuffer   * __restrict buff,
                uint32_t                paramIndex) {
  AkAccessorDAE *accdae;
  float         *pbuff;
  size_t         i, count, st, off;

  if (!acc || !buff || !buff->data || !(accdae = ak_userData(acc)))
    return;

  acc->componentType = (AkTypeId)(uintptr_t)ak_userData(buff);
  if (acc->componentType != AKT_FLOAT)
    return;

  st = accdae->stride ? accdae->stride : 1;
  if (paramIndex >= st)
    return;

  off   = accdae->offset + paramIndex;
  count = acc->count;
  pbuff = buff->data;

  for (i = 0; i < count; i++)
    glm_make_rad(pbuff + off + i * st);
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
void
dae_fixAngleTangent(AkInput  * __restrict inp,
                    uint32_t              outputAngleIndex,
                    uint32_t              outputStride) {
  AkAccessor    *acc;
  AkAccessorDAE *accdae;
  AkBuffer      *buff;
  uint32_t       st;
  uint32_t       idx;

  if (!inp
      || !(acc = inp->accessor)
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

      dae_cvtAngles(acc, buff, _s_dae_angle);

      dae_fixAngleTangent(sampler->inTangentInput,  index, outStride);
      dae_fixAngleTangent(sampler->outTangentInput, index, outStride);
    }

  nxt_sampler:
    item = item->next;
  }

  flist_sp_destroy(&dst->toRadiansSampelers);
}
