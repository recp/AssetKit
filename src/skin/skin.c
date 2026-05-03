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
#include <stdint.h>
#include <string.h>

typedef struct AkSkinAccessorPair {
  AkAccessor *jointAcc;
  AkAccessor *weightAcc;
  AkBuffer   *jointBuf;
  AkBuffer   *weightBuf;
  uint32_t    jStride;
  uint32_t    wStride;
  uint32_t    slotCount;
} AkSkinAccessorPair;

AK_INLINE
size_t
ak_skinPrimitiveVertexCount(AkMeshPrimitive * __restrict prim) {
  AkInput *inp;

  if (!prim) return 0;

  for (inp = prim->input; inp; inp = inp->next) {
    if (inp->semantic == AK_INPUT_POSITION && inp->accessor)
      return inp->accessor->count;
  }

  return 0;
}

AK_INLINE
bool
ak_skinJointToU16(uint32_t joint, uint16_t *dest) {
  if (joint > UINT16_MAX)
    return false;

  *dest = (uint16_t)joint;
  return true;
}

AK_INLINE
bool
ak_skinReadJoint(const char *src,
                 AkTypeId    componentType,
                 uint32_t    k,
                 uint16_t   *dest) {
  switch (componentType) {
    case AKT_UBYTE:
      *dest = ((const uint8_t *)src)[k];
      return true;
    case AKT_USHORT:
      *dest = ((const uint16_t *)src)[k];
      return true;
    case AKT_UINT:
      return ak_skinJointToU16(((const uint32_t *)src)[k], dest);
    default:
      return false;
  }
}

AK_INLINE
float
ak_skinReadWeight(const char *src, AkTypeId componentType, uint32_t k) {
  switch (componentType) {
    case AKT_FLOAT:
      return ((const float *)src)[k];
    case AKT_UBYTE:
      return (float)((const uint8_t  *)src)[k] / 255.0f;
    case AKT_USHORT:
      return (float)((const uint16_t *)src)[k] / 65535.0f;
    default:
      return 0.0f;
  }
}

AK_INLINE
void
ak_skinKeepInfluence(uint16_t * __restrict joints,
                     float    * __restrict weights,
                     uint32_t              maxJoint,
                     uint16_t              joint,
                     float                 weight) {
  uint32_t k, minIdx;
  float    minW;

  if (!(weight > 0.0f) || !isfinite(weight)) return;

  for (k = 0; k < maxJoint; k++) {
    if (weights[k] > 0.0f && joints[k] == joint) {
      weights[k] += weight;
      return;
    }
  }

  for (k = 0; k < maxJoint; k++) {
    if (weights[k] <= 0.0f) {
      joints[k]  = joint;
      weights[k] = weight;
      return;
    }
  }

  minIdx = 0;
  minW   = weights[0];
  for (k = 1; k < maxJoint; k++) {
    if (weights[k] < minW) {
      minW   = weights[k];
      minIdx = k;
    }
  }

  if (weight > minW) {
    joints[minIdx]  = joint;
    weights[minIdx] = weight;
  }
}

AK_EXPORT
size_t
ak_skinInterleave(AkSkin          * __restrict skin,
                  AkMeshPrimitive * __restrict prim,
                  uint32_t                     primIdx,
                  uint32_t                     maxJoint,
                  void           ** __restrict buff) {
  AkInput  *inp;
  uint16_t *idxScratch;
  float    *wgtScratch;
  char     *out;
  size_t    vCount, written, outBytes, v;
  uint32_t  k;
  size_t    rowBytes;

  if (!skin || !prim || !buff || maxJoint == 0)
    return 0;

  /*------------------------------------------------------------------*/
  /* Determine vCount up front so we can size output + scratch.       */
  /* DAE: skin->weights[primIdx]->nVertex.                            */
  /* raw accessors: from JOINTS_n accessor on prim->input.            */
  /*------------------------------------------------------------------*/
  vCount = 0;
  if (skin->weights && skin->weights[primIdx]) {
    vCount = skin->weights[primIdx]->nVertex;
    {
      size_t posCount = ak_skinPrimitiveVertexCount(prim);
      if (posCount > 0 && posCount < vCount)
        vCount = posCount;
    }
  } else {
    vCount = ak_skinPrimitiveVertexCount(prim);
    for (inp = prim->input; inp; inp = inp->next) {
      if (inp->semantic == AK_INPUT_JOINT && inp->accessor) {
        if (vCount == 0 || inp->accessor->count < vCount)
          vCount = inp->accessor->count;
        break;
      }
    }
  }
  if (vCount == 0)
    return 0;

  /*------------------------------------------------------------------*/
  /* Extract via shared core (top-N + normalize). Scratch arrays are  */
  /* the separate-output form ak_skinFillWeights expects.             */
  /*------------------------------------------------------------------*/
  idxScratch = ak_calloc(NULL, vCount * maxJoint * sizeof(uint16_t));
  wgtScratch = ak_calloc(NULL, vCount * maxJoint * sizeof(float));
  if (!idxScratch || !wgtScratch) {
    ak_free(idxScratch);
    ak_free(wgtScratch);
    return 0;
  }

  written = ak_skinFillWeights(skin, prim, primIdx, maxJoint,
                               idxScratch, wgtScratch);
  if (written == 0) {
    ak_free(idxScratch);
    ak_free(wgtScratch);
    return 0;
  }
  vCount = written;

  /*------------------------------------------------------------------*/
  /* Pack into interleaved output: per-vertex                         */
  /*   [J0..J(N-1) (uint16)] [W0..W(N-1) (float)]                     */
  /* No padding between joint and weight blocks. N == maxJoint.       */
  /*------------------------------------------------------------------*/
  rowBytes = maxJoint * (sizeof(uint16_t) + sizeof(float));
  outBytes = vCount * rowBytes;
  if (!(out = *buff))
    out = *buff = ak_calloc(NULL, outBytes);
  if (!out) {
    ak_free(idxScratch);
    ak_free(wgtScratch);
    return 0;
  }

  for (v = 0; v < vCount; v++) {
    char     *row = out + v * rowBytes;
    uint16_t *vJ  = (uint16_t *)row;
    float    *vW  = (float *)(vJ + maxJoint);
    for (k = 0; k < maxJoint; k++) {
      vJ[k] = idxScratch[v * maxJoint + k];
      vW[k] = wgtScratch[v * maxJoint + k];
    }
  }

  ak_free(idxScratch);
  ak_free(wgtScratch);
  return outBytes;
}

AK_EXPORT
size_t
ak_skinFillWeights(AkSkin          * __restrict skin,
                   AkMeshPrimitive * __restrict prim,
                   uint32_t                     primIdx,
                   uint32_t                     maxJoint,
                   uint16_t        * __restrict outIndices,
                   float           * __restrict outWeights) {
  AkBoneWeights *bw;
  AkInput       *inp, *jointInp, *weightInp, *scan;
  AkAccessor    *jointAcc, *weightAcc;
  AkBuffer      *jointBuf, *weightBuf;
  size_t         vCount, v, k;
  uint32_t       slotCount, kept, j;
  size_t         outRow;

  if (!skin || !prim || !outIndices || !outWeights || maxJoint == 0)
    return 0;

  /*------------------------------------------------------------------*/
  /* DAE path: per-primitive CSR layout in skin->weights[primIdx].    */
  /* Variable joint count per vertex; pick top-N by weight, zero-pad  */
  /* missing slots, normalize so sum==1 (graceful degradation when N  */
  /* < authored joint count).                                         */
  /*------------------------------------------------------------------*/
  if (skin->weights && (bw = skin->weights[primIdx])) {
    /* Clamp to primitive POSITION count when smaller. */
    size_t posCount;

    vCount = bw->nVertex;
    posCount = ak_skinPrimitiveVertexCount(prim);
    if (posCount > 0 && posCount < vCount)
      vCount = posCount;

    memset(outIndices, 0, vCount * maxJoint * sizeof(uint16_t));
    memset(outWeights, 0, vCount * maxJoint * sizeof(float));

    for (v = 0; v < vCount; v++) {
      slotCount = bw->counts[v];
      outRow    = (size_t)v * maxJoint;
      kept      = 0;

      if (slotCount <= maxJoint) {
        for (k = 0; k < slotCount; k++) {
          AkBoneWeight *src = &bw->weights[bw->indexes[v] + k];
          uint16_t      joint;

          if (!ak_skinJointToU16(src->joint, &joint)
              || !(src->weight > 0.0f)
              || !isfinite(src->weight))
            continue;

          outIndices[outRow + kept] = joint;
          outWeights[outRow + kept] = src->weight;
          kept++;
        }
      } else {
        /* > maxJoint authored joints: keep top-N by weight magnitude */
        for (k = 0; k < slotCount; k++) {
          AkBoneWeight *src = &bw->weights[bw->indexes[v] + k];
          uint16_t      joint;

          if (!ak_skinJointToU16(src->joint, &joint)
              || !(src->weight > 0.0f)
              || !isfinite(src->weight))
            continue;

          if (kept < maxJoint) {
            outIndices[outRow + kept] = joint;
            outWeights[outRow + kept] = src->weight;
            kept++;
          } else {
            /* find current min in kept set */
            uint32_t minIdx = 0;
            float    minW   = outWeights[outRow + 0];
            for (j = 1; j < maxJoint; j++) {
              if (outWeights[outRow + j] < minW) {
                minW = outWeights[outRow + j];
                minIdx = j;
              }
            }
            if (src->weight > minW) {
              outIndices[outRow + minIdx] = joint;
              outWeights[outRow + minIdx] = src->weight;
            }
          }
        }
      }

      /* renormalize so weights sum to 1 (handles both truncation and
         authored asymmetric weights — common after top-N selection) */
      {
        float sum = 0.0f;
        for (k = 0; k < maxJoint; k++) sum += outWeights[outRow + k];
        if (sum > 0.0f && isfinite(sum)) {
          float inv = 1.0f / sum;
          for (k = 0; k < maxJoint; k++) outWeights[outRow + k] *= inv;
        }
      }
    }
    return vCount;
  }

  /*------------------------------------------------------------------*/
  /* Raw accessor path: collect every JOINTS_n / WEIGHTS_n pair, then */
  /* keep the top maxJoint influences per vertex and normalize.       */
  /*------------------------------------------------------------------*/
  {
    AkSkinAccessorPair *pairs, *pair;
    uint32_t            pairCount, pairIdx, pairCap;

    pairCap = 0;
    for (inp = prim->input; inp; inp = inp->next) {
      if (inp->semantic == AK_INPUT_JOINT)
        pairCap++;
    }
    if (pairCap == 0)
      return 0;

    pairCount = 0;
    pairs     = ak_calloc(NULL, sizeof(*pairs) * pairCap);
    if (!pairs)
      return 0;

    vCount = ak_skinPrimitiveVertexCount(prim);

    for (jointInp = prim->input; jointInp; jointInp = jointInp->next) {
      if (jointInp->semantic != AK_INPUT_JOINT)
        continue;

      weightInp = NULL;
      for (scan = prim->input; scan; scan = scan->next) {
        if (scan->semantic == AK_INPUT_WEIGHT && scan->set == jointInp->set) {
          weightInp = scan;
          break;
        }
      }
      if (!weightInp)
        continue;

      jointAcc  = jointInp->accessor;
      weightAcc = weightInp->accessor;
      if (!jointAcc || !weightAcc)
        continue;
      jointBuf  = jointAcc->buffer;
      weightBuf = weightAcc->buffer;
      if (!jointBuf || !jointBuf->data || !weightBuf || !weightBuf->data)
        continue;

      pair            = &pairs[pairCount];
      pair->jointAcc  = jointAcc;
      pair->weightAcc = weightAcc;
      pair->jointBuf  = jointBuf;
      pair->weightBuf = weightBuf;
      pair->jStride   = (uint32_t)jointAcc->byteStride;
      pair->wStride   = (uint32_t)weightAcc->byteStride;
      if (!pair->jStride) pair->jStride = (uint32_t)jointAcc->fillByteSize;
      if (!pair->wStride) pair->wStride = (uint32_t)weightAcc->fillByteSize;

      pair->slotCount = jointAcc->componentCount;
      if (weightAcc->componentCount < pair->slotCount)
        pair->slotCount = weightAcc->componentCount;
      if (pair->slotCount == 0)
        continue;

      if (vCount == 0 || jointAcc->count < vCount)
        vCount = jointAcc->count;
      if (weightAcc->count < vCount)
        vCount = weightAcc->count;

      pairCount++;
    }

    if (pairCount == 0 || vCount == 0) {
      ak_free(pairs);
      return 0;
    }

    memset(outIndices, 0, vCount * maxJoint * sizeof(uint16_t));
    memset(outWeights, 0, vCount * maxJoint * sizeof(float));

    for (v = 0; v < vCount; v++) {
      outRow = (size_t)v * maxJoint;

      for (pairIdx = 0; pairIdx < pairCount; pairIdx++) {
        const char *jSrc, *wSrc;
        uint16_t    joint;
        float       weight;

        pair = &pairs[pairIdx];
        if (v >= pair->jointAcc->count || v >= pair->weightAcc->count)
          continue;

        jSrc = (const char *)pair->jointBuf->data
               + pair->jointAcc->byteOffset
               + (size_t)v * pair->jStride;
        wSrc = (const char *)pair->weightBuf->data
               + pair->weightAcc->byteOffset
               + (size_t)v * pair->wStride;

        slotCount = pair->slotCount;
        for (k = 0; k < slotCount; k++) {
          if (!ak_skinReadJoint(jSrc,
                                pair->jointAcc->componentType,
                                (uint32_t)k,
                                &joint))
            continue;

          weight = ak_skinReadWeight(wSrc,
                                     pair->weightAcc->componentType,
                                     (uint32_t)k);
          ak_skinKeepInfluence(&outIndices[outRow],
                               &outWeights[outRow],
                               maxJoint,
                               joint,
                               weight);
        }
      }

      {
        float sum = 0.0f;
        for (k = 0; k < maxJoint; k++)
          sum += outWeights[outRow + k];
        if (sum > 0.0f && isfinite(sum)) {
          float inv = 1.0f / sum;
          for (k = 0; k < maxJoint; k++)
            outWeights[outRow + k] *= inv;
        }
      }
    }

    ak_free(pairs);
    return vCount;
  }
}
