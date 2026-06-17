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

typedef struct AkSkinInspectPrimitive {
  struct AkSkinInspectPrimitive *next;
  AkMeshPrimitive               *prim;
  AkSkinAccessorPair            *pairs;
  size_t                         vertexCount;
  uint32_t                       pairCount;
} AkSkinInspectPrimitive;

typedef struct AkSkinInspectView {
  AkSkinInspectPrimitive *primitive;
} AkSkinInspectView;

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
AkBoneWeights*
ak_skinWeightsForPrimitive(AkSkin          * __restrict skin,
                           AkMeshPrimitive * __restrict prim,
                           uint32_t                     primIdx) {
  AkMeshPrimitive *it;
  uint32_t         idx;

  if (!skin || !skin->weights || skin->nPrims == 0)
    return NULL;

  if (prim && prim->mesh) {
    idx = 0;
    for (it = prim->mesh->primitive; it; it = it->next, idx++) {
      if (it != prim)
        continue;

      if (idx < skin->nPrims && skin->weights[idx])
        return skin->weights[idx];

      break;
    }
  }

  if (primIdx < skin->nPrims)
    return skin->weights[primIdx];

  return NULL;
}

AK_INLINE
uint32_t
ak_skinAccessorPairCapacity(AkMeshPrimitive * __restrict prim) {
  AkInput *inp;
  uint32_t count;

  count = 0;
  for (inp = prim->input; inp; inp = inp->next) {
    if (inp->semantic == AK_INPUT_JOINT)
      count++;
  }

  return count;
}

AK_INLINE
uint32_t
ak_skinCollectAccessorPairs(AkMeshPrimitive    * __restrict prim,
                            AkSkinAccessorPair * __restrict pairs,
                            uint32_t                        pairCap,
                            size_t            * __restrict vCount) {
  AkSkinAccessorPair *pair;
  AkAccessor         *jointAcc, *weightAcc;
  AkBuffer           *jointBuf, *weightBuf;
  AkInput            *jointInp, *weightInp, *scan;
  uint32_t            pairCount;
  size_t              count;

  if (!prim || !pairs || pairCap == 0 || !vCount)
    return 0;

  count     = ak_skinPrimitiveVertexCount(prim);
  pairCount = 0;

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

    if (pairCount >= pairCap)
      break;

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

    if (count == 0 || jointAcc->count < count)
      count = jointAcc->count;
    if (weightAcc->count < count)
      count = weightAcc->count;

    pairCount++;
  }

  *vCount = count;
  return pairCount;
}

static
AkSkinInspectPrimitive*
ak_skinInspectPrimitive(AkSkin          * __restrict skin,
                        AkMeshPrimitive * __restrict prim) {
  AkSkinInspectView      *view;
  AkSkinInspectPrimitive *insp;
  AkSkinAccessorPair     *pairs;
  AkHeap                 *heap;
  uint32_t                pairCap, pairCount;
  size_t                  vCount;

  if (!skin || !prim)
    return NULL;

  view = skin->inspectResult;
  if (view) {
    for (insp = view->primitive; insp; insp = insp->next) {
      if (insp->prim == prim)
        return insp;
    }
  } else {
    heap = ak_heap_getheap(skin);
    view = ak_heap_calloc(heap, skin, sizeof(*view));
    if (!view)
      return NULL;
    skin->inspectResult = view;
  }

  pairCap = ak_skinAccessorPairCapacity(prim);
  if (pairCap == 0)
    return NULL;

  heap = ak_heap_getheap(view);
  pairs = ak_heap_calloc(heap, view, sizeof(*pairs) * pairCap);
  if (!pairs)
    return NULL;

  pairCount = ak_skinCollectAccessorPairs(prim, pairs, pairCap, &vCount);
  if (pairCount == 0 || vCount == 0)
    return NULL;

  insp              = ak_heap_calloc(heap, view, sizeof(*insp));
  insp->prim        = prim;
  insp->pairs       = pairs;
  insp->pairCount   = pairCount;
  insp->vertexCount = vCount;
  insp->next        = view->primitive;
  view->primitive   = insp;

  return insp;
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
ak_skinNormalizeWeights(float * __restrict weights, uint32_t maxJoint) {
  float    sum;
  float    inv;
  uint32_t k;

  sum = 0.0f;
  for (k = 0; k < maxJoint; k++)
    sum += weights[k];

  if (sum > 0.0f && isfinite(sum)) {
    inv = 1.0f / sum;
    for (k = 0; k < maxJoint; k++)
      weights[k] *= inv;
  }
}

AK_INLINE
void
ak_skinWriteInterleavedRow(char           * __restrict row,
                           const uint16_t * __restrict joints,
                           const float    * __restrict weights,
                           uint32_t                    maxJoint) {
  size_t jointBytes;

  jointBytes = sizeof(uint16_t) * maxJoint;
  memcpy(row, joints, jointBytes);
  memcpy(row + jointBytes, weights, sizeof(float) * maxJoint);
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
  AkBoneWeights      *bw;
  AkBoneWeight       *src;
  AkSkinInspectPrimitive *insp;
  AkSkinAccessorPair *pairs;
  AkSkinAccessorPair *pair;
  uint16_t           *idxScratch;
  uint16_t            joint;
  float              *wgtScratch;
  char               *out;
  const char         *jSrc, *wSrc;
  size_t              vCount, outBytes, v, posCount;
  size_t              rowBytes;
  uint32_t            k, pairCount, pairIdx, slotCount;

  if (!skin || !prim || !buff || maxJoint == 0)
    return 0;

  bw        = ak_skinWeightsForPrimitive(skin, prim, primIdx);
  pairs     = NULL;
  pairCount = 0;

  if (bw) {
    vCount   = bw->nVertex;
    posCount = ak_skinPrimitiveVertexCount(prim);
    if (posCount > 0 && posCount < vCount)
      vCount = posCount;
  } else {
    if (!(insp = ak_skinInspectPrimitive(skin, prim)))
      return 0;

    pairs     = insp->pairs;
    pairCount = insp->pairCount;
    vCount    = insp->vertexCount;
  }

  if (vCount == 0)
    return 0;

  rowBytes = maxJoint * (sizeof(uint16_t) + sizeof(float));
  outBytes = vCount * rowBytes;
  if (!(out = *buff))
    out = *buff = ak_calloc(NULL, outBytes);
  if (!out)
    return 0;

  idxScratch = AK_ALLOCA(sizeof(uint16_t) * maxJoint);
  wgtScratch = AK_ALLOCA(sizeof(float)    * maxJoint);

  for (v = 0; v < vCount; v++) {
    memset(idxScratch, 0, sizeof(uint16_t) * maxJoint);
    memset(wgtScratch, 0, sizeof(float)    * maxJoint);

    if (bw) {
      slotCount = bw->counts[v];
      for (k = 0; k < slotCount; k++) {
        src = &bw->weights[bw->indexes[v] + k];
        if (ak_skinJointToU16(src->joint, &joint))
          ak_skinKeepInfluence(idxScratch, wgtScratch,
                               maxJoint, joint, src->weight);
      }
    } else {
      for (pairIdx = 0; pairIdx < pairCount; pairIdx++) {
        pair = &pairs[pairIdx];
        jSrc = (const char *)pair->jointBuf->data
               + pair->jointAcc->byteOffset
               + (size_t)v * pair->jStride;
        wSrc = (const char *)pair->weightBuf->data
               + pair->weightAcc->byteOffset
               + (size_t)v * pair->wStride;

        slotCount = pair->slotCount;
        for (k = 0; k < slotCount; k++) {
          if (ak_skinReadJoint(jSrc,
                               pair->jointAcc->componentType,
                               (uint32_t)k,
                               &joint)) {
            ak_skinKeepInfluence(idxScratch, wgtScratch,
                                 maxJoint, joint,
                                 ak_skinReadWeight(wSrc,
                                                   pair->weightAcc->componentType,
                                                   (uint32_t)k));
          }
        }
      }
    }

    ak_skinNormalizeWeights(wgtScratch, maxJoint);
    ak_skinWriteInterleavedRow(out + v * rowBytes,
                               idxScratch,
                               wgtScratch,
                               maxJoint);
  }

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
  AkBoneWeights      *bw;
  AkBoneWeight       *src;
  AkSkinInspectPrimitive *insp;
  AkSkinAccessorPair *pairs, *pair;
  const char         *jSrc, *wSrc;
  size_t              vCount, v, k, posCount;
  size_t              outRow;
  uint32_t            pairCount, pairIdx;
  uint32_t            slotCount;
  uint16_t            joint;
  float               weight;

  if (!skin || !prim || !outIndices || !outWeights || maxJoint == 0)
    return 0;

  /*------------------------------------------------------------------*/
  /* DAE path: per-primitive CSR layout in skin->weights[primIdx].    */
  /* Variable joint count per vertex; pick top-N by weight, zero-pad  */
  /* missing slots, normalize so sum==1 (graceful degradation when N  */
  /* < authored joint count).                                         */
  /*------------------------------------------------------------------*/
  if ((bw = ak_skinWeightsForPrimitive(skin, prim, primIdx))) {
    vCount = bw->nVertex;
    posCount = ak_skinPrimitiveVertexCount(prim);
    if (posCount > 0 && posCount < vCount)
      vCount = posCount;

    memset(outIndices, 0, vCount * maxJoint * sizeof(uint16_t));
    memset(outWeights, 0, vCount * maxJoint * sizeof(float));

    for (v = 0; v < vCount; v++) {
      slotCount = bw->counts[v];
      outRow    = (size_t)v * maxJoint;

      for (k = 0; k < slotCount; k++) {
        src = &bw->weights[bw->indexes[v] + k];
        if (ak_skinJointToU16(src->joint, &joint)) {
          ak_skinKeepInfluence(&outIndices[outRow],
                               &outWeights[outRow],
                               maxJoint,
                               joint,
                               src->weight);
        }
      }

      ak_skinNormalizeWeights(&outWeights[outRow], maxJoint);
    }
    return vCount;
  }

  /*------------------------------------------------------------------*/
  /* Raw accessor path: collect every JOINTS_n / WEIGHTS_n pair, then */
  /* keep the top maxJoint influences per vertex and normalize.       */
  /*------------------------------------------------------------------*/
  if (!(insp = ak_skinInspectPrimitive(skin, prim))) {
    return 0;
  }

  pairs     = insp->pairs;
  pairCount = insp->pairCount;
  vCount    = insp->vertexCount;

  memset(outIndices, 0, vCount * maxJoint * sizeof(uint16_t));
  memset(outWeights, 0, vCount * maxJoint * sizeof(float));

  for (v = 0; v < vCount; v++) {
    outRow = (size_t)v * maxJoint;

    for (pairIdx = 0; pairIdx < pairCount; pairIdx++) {
      pair = &pairs[pairIdx];

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

    ak_skinNormalizeWeights(&outWeights[outRow], maxJoint);
  }

  return vCount;
}

AK_EXPORT
size_t
ak_skinVerticesForJoint(AkSkin          * __restrict skin,
                        AkMeshPrimitive * __restrict prim,
                        uint32_t                     primIdx,
                        uint32_t                     jointIdx,
                        uint32_t        * __restrict outVertices,
                        size_t                       capacity) {
  AkBoneWeights      *bw;
  AkBoneWeight       *src;
  AkSkinInspectPrimitive *insp;
  AkSkinAccessorPair *pairs, *pair;
  const char         *jSrc, *wSrc;
  size_t              vCount, posCount, v, k;
  size_t              found, written;
  uint32_t            pairCount, pairIdx;
  uint32_t            slotCount;
  uint16_t            joint;
  float               weight;
  bool                matched;

  if (!skin || !prim)
    return 0;

  found   = 0;
  written = 0;

  if ((bw = ak_skinWeightsForPrimitive(skin, prim, primIdx))) {
    vCount = bw->nVertex;
    posCount = ak_skinPrimitiveVertexCount(prim);
    if (posCount > 0 && posCount < vCount)
      vCount = posCount;

    for (v = 0; v < vCount; v++) {
      matched   = false;
      slotCount = bw->counts[v];

      for (k = 0; k < slotCount; k++) {
        src = &bw->weights[bw->indexes[v] + k];
        if (src->joint == jointIdx
            && src->weight > 0.0f
            && isfinite(src->weight)) {
          matched = true;
          break;
        }
      }

      if (!matched)
        continue;

      if (outVertices && written < capacity)
        outVertices[written++] = (uint32_t)v;
      found++;
    }

    return found;
  }

  if (jointIdx > UINT16_MAX)
    return 0;

  if (!(insp = ak_skinInspectPrimitive(skin, prim)))
    return 0;

  pairs     = insp->pairs;
  pairCount = insp->pairCount;
  vCount    = insp->vertexCount;

  for (v = 0; v < vCount; v++) {
    matched = false;

    for (pairIdx = 0; pairIdx < pairCount && !matched; pairIdx++) {
      pair = &pairs[pairIdx];

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
        if ((uint32_t)joint == jointIdx
            && weight > 0.0f
            && isfinite(weight)) {
          matched = true;
          break;
        }
      }
    }

    if (!matched)
      continue;

    if (outVertices && written < capacity)
      outVertices[written++] = (uint32_t)v;
    found++;
  }

  return found;
}
