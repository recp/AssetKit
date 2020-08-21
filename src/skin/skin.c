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

#include "../common.h"

AK_EXPORT
size_t
ak_skinInterleave(AkBoneWeights * __restrict source,
                  uint32_t                   maxJoint,
                  uint32_t                   itemCount,
                  void         ** __restrict buff) {
  AkBoneWeight *bw;
  char         *tmp, *item;
  float        *pWeight;
  uint32_t     *pJoint;
  size_t        size, szt, i, k;
  uint32_t      iterCount;

#ifdef DEBUG
  assert(buff);
#endif

  szt  = maxJoint * (sizeof(uint32_t) + sizeof(float));
  size = source->nVertex * szt;

  if (!(tmp = *buff))
    tmp = *buff = ak_calloc(NULL, size);

  for (i = 0; i < source->nVertex; i++) {
    iterCount = GLM_MIN(source->counts[i], GLM_MIN(maxJoint, itemCount));
    item      = tmp + szt * i;

    pJoint    = (uint32_t *)item;
    pWeight   = (float *)(void *)(item + sizeof(uint32_t) * itemCount);

    for (k = 0; k < iterCount; k++) {
      bw         = &source->weights[source->indexes[i] + k];
      pJoint[k]  = bw->joint;
      pWeight[k] = bw->weight;
    }
  }

  return size;
}
