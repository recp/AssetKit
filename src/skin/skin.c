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
  /* glTF: from JOINTS_n accessor on prim->input.                     */
  /*------------------------------------------------------------------*/
  vCount = 0;
  if (skin->weights && skin->weights[primIdx]) {
    vCount = skin->weights[primIdx]->nVertex;
  } else {
    for (inp = prim->input; inp; inp = inp->next) {
      if (inp->semantic == AK_INPUT_JOINT && inp->accessor) {
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

  written = ak_skinFillWeights(skin, prim, primIdx, maxJoint,
                               idxScratch, wgtScratch);
  if (written == 0) {
    ak_free(idxScratch);
    ak_free(wgtScratch);
    return 0;
  }

  /*------------------------------------------------------------------*/
  /* Pack into interleaved output: per-vertex                         */
  /*   [J0..J(N-1) (uint16)] [W0..W(N-1) (float)]                     */
  /* No padding between joint and weight blocks. N == maxJoint.       */
  /*------------------------------------------------------------------*/
  rowBytes = maxJoint * (sizeof(uint16_t) + sizeof(float));
  outBytes = vCount * rowBytes;
  if (!(out = *buff))
    out = *buff = ak_calloc(NULL, outBytes);

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
  AkInput       *inp, *jointInp, *weightInp;
  AkAccessor    *jointAcc, *weightAcc;
  AkBuffer      *jointBuf, *weightBuf;
  size_t         vCount, v, k;
  uint32_t       slotCount, jStride, wStride, kept, j;
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
    /* Clamp to prim's POSITION accessor count when smaller. DAE assets
       (e.g. Seymour) can leave skin->weights[primIdx]->nVertex larger
       than the mesh's actual vertex count after AssetKit's vertex
       dedup/expansion — extra weight entries have no corresponding
       mesh vertex and are silently dropped. The renderer's vertex
       array is sized from POSITION; that's what the caller's output
       buffers were allocated to match. */
    AkInput *posInp;
    vCount = bw->nVertex;
    for (posInp = prim->input; posInp; posInp = posInp->next) {
      if (posInp->semantic == AK_INPUT_POSITION
          && posInp->accessor
          && posInp->accessor->count < vCount) {
        vCount = posInp->accessor->count;
        break;
      }
    }
    memset(outIndices, 0, vCount * maxJoint * sizeof(uint16_t));
    memset(outWeights, 0, vCount * maxJoint * sizeof(float));

    for (v = 0; v < vCount; v++) {
      slotCount = bw->counts[v];
      outRow    = (size_t)v * maxJoint;
      kept      = 0;

      if (slotCount <= maxJoint) {
        /* fits — copy as-is */
        for (k = 0; k < slotCount; k++) {
          AkBoneWeight *src     = &bw->weights[bw->indexes[v] + k];
          outIndices[outRow + k] = (uint16_t)src->joint;
          outWeights[outRow + k] = src->weight;
        }
        kept = slotCount;
      } else {
        /* > maxJoint authored joints: keep top-N by weight magnitude */
        for (k = 0; k < slotCount; k++) {
          AkBoneWeight *src = &bw->weights[bw->indexes[v] + k];
          if (kept < maxJoint) {
            outIndices[outRow + kept] = (uint16_t)src->joint;
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
              outIndices[outRow + minIdx] = (uint16_t)src->joint;
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
        if (sum > 0.0f) {
          float inv = 1.0f / sum;
          for (k = 0; k < maxJoint; k++) outWeights[outRow + k] *= inv;
        }
      }
    }
    return vCount;
  }

  /*------------------------------------------------------------------*/
  /* glTF path: vec4 JOINTS_n + WEIGHTS_n accessors in prim->input.   */
  /* Already fixed-4 (or maxJoint when caller passes 4); copy through */
  /* with type widening on joints (UBYTE/USHORT → uint32). Multiset   */
  /* (JOINTS_1, ...) ignored for now — first set covers spec common.  */
  /*------------------------------------------------------------------*/
  jointInp = weightInp = NULL;
  for (inp = prim->input; inp; inp = inp->next) {
    if      (inp->semantic == AK_INPUT_JOINT  && !jointInp)  jointInp  = inp;
    else if (inp->semantic == AK_INPUT_WEIGHT && !weightInp) weightInp = inp;
  }
  if (!jointInp  || !(jointAcc  = jointInp->accessor)  || !(jointBuf  = jointAcc->buffer))
    return 0;
  if (!weightInp || !(weightAcc = weightInp->accessor) || !(weightBuf = weightAcc->buffer))
    return 0;

  vCount    = jointAcc->count;
  slotCount = jointAcc->componentCount;     /* typically 4 */
  if (slotCount > maxJoint) slotCount = maxJoint;

  jStride = (uint32_t)jointAcc->byteStride;
  wStride = (uint32_t)weightAcc->byteStride;
  if (!jStride) jStride = (uint32_t)jointAcc->fillByteSize;
  if (!wStride) wStride = (uint32_t)weightAcc->fillByteSize;

  memset(outIndices, 0, vCount * maxJoint * sizeof(uint16_t));
  memset(outWeights, 0, vCount * maxJoint * sizeof(float));

  for (v = 0; v < vCount; v++) {
    const char *jSrc = (const char *)jointBuf->data
                       + jointAcc->byteOffset
                       + (size_t)v * jStride;
    const char *wSrc = (const char *)weightBuf->data
                       + weightAcc->byteOffset
                       + (size_t)v * wStride;
    outRow = (size_t)v * maxJoint;

    /* glTF spec caps JOINTS_n at UBYTE or USHORT — both fit in uint16
       natively. UINT path is defensive (custom format, will truncate
       silently if a single asset exceeded 65k joints which no real
       authoring tool emits). */
    for (k = 0; k < slotCount; k++) {
      switch (jointAcc->componentType) {
        case AKT_UBYTE:
          outIndices[outRow + k] = ((const uint8_t  *)jSrc)[k];
          break;
        case AKT_USHORT:
          outIndices[outRow + k] = ((const uint16_t *)jSrc)[k];
          break;
        case AKT_UINT:
          outIndices[outRow + k] = (uint16_t)((const uint32_t *)jSrc)[k];
          break;
        default:
          outIndices[outRow + k] = 0;
          break;
      }
    }

    /* weights — usually float, sometimes normalized integer */
    for (k = 0; k < slotCount; k++) {
      switch (weightAcc->componentType) {
        case AKT_FLOAT:
          outWeights[outRow + k] = ((const float *)wSrc)[k];
          break;
        case AKT_UBYTE:
          outWeights[outRow + k] = (float)((const uint8_t  *)wSrc)[k] / 255.0f;
          break;
        case AKT_USHORT:
          outWeights[outRow + k] = (float)((const uint16_t *)wSrc)[k] / 65535.0f;
          break;
        default:
          outWeights[outRow + k] = 0.0f;
          break;
      }
    }
  }

  return vCount;
}
