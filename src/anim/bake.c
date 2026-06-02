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

/*
 * Animation baking — per-node transform sampling
 * ==============================================
 *
 * Maya's joint convention emits each joint with up to six independent
 * <rotate> elements (jointOrient{XYZ} + rotate{XYZ}). The full local
 * matrix is only correct after all six are composed in document order
 * (handled by ak_transformCombine). Decomposed-property animation APIs
 * (Apple SCNNode.eulerAngles, three.js Object3D.rotation, Filament
 * TransformManager) cannot represent that — they offer three Euler
 * slots, period.
 *
 * Solution: bake. Sample every channel that targets any AkObject in
 * the node's transform chain on a shared time grid, run
 * ak_transformCombine per sample, emit a stream of 4×4 local matrices.
 * Bridges then attach a single transform-keyed animation per node.
 *
 * Matrix-driven runtimes that compose bone matrices CPU-side every frame can
 * skip the bake; they already iterate channels per frame. The helper is
 * opt-in; ak_nodeNeedsBaking() is the heuristic.
 */

#include "../common.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


/* Cap mostly to keep the per-node fixed-size scratch arrays small.
   A pathological node would have to authored ~64 transform elements
   or ~256 channels — neither shows up in real assets. */
#define BAKE_MAX_XFORM_OBJECTS 64
#define BAKE_MAX_CHANNELS      256
#define BAKE_MAX_ANIM_STACK    256

/* Maximum time gap between adjacent samples in the baked output.
   Reason: consumers (Apple SCNAnimation, three.js KeyframeTrack, ...)
   linearly interpolate the 4×4 matrix elements between adjacent
   samples. Linear lerp between rotation matrices is NOT slerp — for
   large angular deltas (e.g. a propeller spinning 348° in 1 second
   with only two authored keyframes) the lerp goes through the
   *interior* of the rotation manifold, producing a near-identity
   intermediate matrix and a visibly tiny wobble instead of a full
   spin. Inserting subdivisions every BAKE_MAX_STEP seconds bounds the
   per-pair angular delta so the lerp approximates a slerp closely
   enough for typical playback.

   30 Hz target (~33 ms). Fine balance: dense enough to handle 360°/s
   rotations without artifacts (max ~12° between samples), sparse
   enough to keep memory + decode cost low for long animations. */
#define BAKE_MAX_STEP 0.0333f

/* Per-channel binding: where to write the sampled value, and where to
   read it from. */
typedef struct ChannelBind {
  AkObject     *target;     /* AkObject wrapping AkRotate/AkTranslate/... */
  uint32_t      off;        /* logical slot in target (0..3 typical)      */
  bool          isPartial;  /* single component vs whole vector           */

  const float  *keyTimes;
  size_t        keyCount;
  const float  *keyValues;  /* contiguous; valueStride floats per frame   */
  size_t        valueStride;

  AkInterpolationType interp;

  float         saved[16];  /* snapshot of target->pData at function entry */
  size_t        savedLen;   /* number of floats valid in saved[]           */
} ChannelBind;

static int
bake_floatcmp(const void *a, const void *b) {
  float fa = *(const float *)a;
  float fb = *(const float *)b;
  return (fa > fb) - (fa < fb);
}

/* Snapshot length per AkObject type — how many floats live in pData. */
static size_t
bake_payloadFloatCount(AkObject *obj) {
  switch ((AkTypeId)obj->type) {
    case AKT_TRANSLATE: return 3;   /* AkTranslate.val[3]   */
    case AKT_SCALE:     return 3;   /* AkScale.val[3]       */
    case AKT_ROTATE:    return 4;   /* AkRotate.val[4]      */
    case AKT_QUATERNION:return 4;   /* AkQuaternion.val[4]  */
    case AKT_MATRIX:    return 16;  /* AkMatrix.val[4][4]   */
    case AKT_SKEW:      return 7;   /* angle + 2 vec3       */
    default:            return 0;
  }
}

/* Linear interpolation between adjacent keyframes. STEP honored.
   For BEZIER/HERMITE we fall back to LINEAR — the resulting bake is
   already keyframe-aligned so the caller's interpolation in the engine
   layer can handle the curve refinement (LINEAR between keyframes is
   indistinguishable from the original curve at sample times that
   equal a keyframe). */
static float
bake_sampleScalar(const ChannelBind *b, size_t componentIdx, float t) {
  size_t ki, valStride;
  float  t0, t1, v0, v1, alpha;

  if (b->keyCount == 0) return 0.0f;

  /* find first key with time >= t */
  for (ki = 0; ki < b->keyCount && b->keyTimes[ki] < t; ki++) { }

  valStride = b->valueStride;
  if (ki == 0) {
    return b->keyValues[componentIdx];
  }
  if (ki >= b->keyCount) {
    return b->keyValues[(b->keyCount - 1) * valStride + componentIdx];
  }

  v0 = b->keyValues[(ki - 1) * valStride + componentIdx];
  v1 = b->keyValues[ki       * valStride + componentIdx];
  if (b->interp == AK_INTERPOLATION_STEP) return v0;

  t0    = b->keyTimes[ki - 1];
  t1    = b->keyTimes[ki];
  alpha = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;
  return v0 * (1.0f - alpha) + v1 * alpha;
}

static void
bake_collectChannels(AkAnimation  * __restrict anim,
                     AkContext    * __restrict actx,
                     AkObject    ** __restrict xformObjects,
                     int                       nXform,
                     ChannelBind  * __restrict binds,
                     int          * __restrict nBinds) {
  AkAnimation     *stack[BAKE_MAX_ANIM_STACK], *next;
  AkChannel       *ch;
  AkAnimSampler   *samp;
  AkInput         *inp, *inputInput, *outputInput;
  AkResolvedTarget rt;
  int              j, top;
  bool             isOurs;

  top = 0;
  while (anim && *nBinds < BAKE_MAX_CHANNELS) {
    for (ch = anim->channel; ch && *nBinds < BAKE_MAX_CHANNELS; ch = ch->next) {
      rt = ak_channelTarget(actx, ch);
      if (!rt.target) continue;

      isOurs = false;
      for (j = 0; j < nXform; j++) {
        if (rt.target == xformObjects[j]) { isOurs = true; break; }
      }
      if (!isOurs) continue;

      samp = ak_getObjectByUrl(&ch->source);
      if (!samp) continue;

      inputInput = outputInput = NULL;
      for (inp = samp->input; inp; inp = inp->next) {
        if      (inp->semantic == AK_INPUT_INPUT  && !inputInput)  inputInput  = inp;
        else if (inp->semantic == AK_INPUT_OUTPUT && !outputInput) outputInput = inp;
      }

      if (!inputInput  || !inputInput->accessor  || !inputInput->accessor->buffer
          || !inputInput->accessor->buffer->data
          || !outputInput || !outputInput->accessor || !outputInput->accessor->buffer
          || !outputInput->accessor->buffer->data)
        continue;

      binds[*nBinds].target      = rt.target;
      binds[*nBinds].off         = rt.off;
      binds[*nBinds].isPartial   = rt.isPartial;
      binds[*nBinds].keyTimes    = (const float *)
        ((const char *)inputInput->accessor->buffer->data
         + inputInput->accessor->byteOffset);
      binds[*nBinds].keyCount    = inputInput->accessor->count;
      binds[*nBinds].keyValues   = (const float *)
        ((const char *)outputInput->accessor->buffer->data
         + outputInput->accessor->byteOffset);
      binds[*nBinds].valueStride = outputInput->accessor->componentCount;
      binds[*nBinds].interp      = samp->uniInterpolation;
      (*nBinds)++;
    }

    if (anim->animation) {
      next = anim->next;
      if (next && top < BAKE_MAX_ANIM_STACK)
        stack[top++] = next;
      anim = anim->animation;
    } else if (anim->next) {
      anim = anim->next;
    } else if (top > 0) {
      anim = stack[--top];
    } else {
      anim = NULL;
    }
  }
}

AK_EXPORT
bool
ak_nodeNeedsBaking(AkNode * __restrict node) {
  AkObject *it;
  uint32_t  rotateCount;

  if (!node || !node->transform) return false;
  
  rotateCount = 0;

  for (it = node->transform->base; it; it = it->next) {
    if ((AkTypeId)it->type == AKT_ROTATE && ++rotateCount > 1) return true;
  }

  for (it = node->transform->item; it; it = it->next) {
    if ((AkTypeId)it->type == AKT_ROTATE && ++rotateCount > 1) return true;
  }

  return false;
}

AK_EXPORT
AkBakedAnimation *
ak_nodeBakeAnimation(AkDoc  * __restrict doc,
                     AkNode * __restrict node) {
  AkAnimation      *animIt;
  AkObject         *xformObjects[BAKE_MAX_XFORM_OBJECTS];
  AkObject         *it;
  AkBakedAnimation *out;
  float            *allTimes, *dense;
  ChannelBind       binds[BAKE_MAX_CHANNELS];
  AkContext         actx;
  size_t            totalTimes, uniqueCount, denseCount, subdivs, d, s;
  size_t            i, k, t_idx;
  float             t0, t1, gap;
  int               nXform, nBinds, j;

  if (!doc || !node || !node->transform) return NULL;

  /* 1. Snapshot the AkObjects that make up the node's transform chain.
        Order in the array doesn't matter for matching, only ak_transformCombine
        cares about chain order — and that's read directly from node->transform
        each sample. */
  nXform = 0;
  for (it = node->transform->base;
       it && nXform < BAKE_MAX_XFORM_OBJECTS;
       it = it->next) {
    xformObjects[nXform++] = it;
  }

  for (it = node->transform->item;
       it && nXform < BAKE_MAX_XFORM_OBJECTS;
       it = it->next) {
    xformObjects[nXform++] = it;
  }

  if (nXform == 0) return NULL;

  /* 2. Walk every animation channel; bind those whose resolved target
        is one of our transform AkObjects. */
  memset(&actx, 0, sizeof(actx));

  actx.doc = doc;
  nBinds   = 0;

  for (animIt = doc->lib.animations.first; animIt; animIt = animIt->next) {
    bake_collectChannels(animIt, &actx,
                         xformObjects, nXform, binds, &nBinds);
    if (nBinds >= BAKE_MAX_CHANNELS) break;
  }

  if (nBinds == 0) return NULL;

  /* 3. Snapshot animated AkObject payloads so we can restore them on
        exit. This lets the caller continue using node->transform for
        bind-pose composition without observing the mutations we make
        per sample. */
  for (j = 0; j < nBinds; j++) {
    binds[j].savedLen = bake_payloadFloatCount(binds[j].target);
    if (binds[j].savedLen > 0)
      memcpy(binds[j].saved, binds[j].target->pData,
             binds[j].savedLen * sizeof(float));
  }

  /* 4. Build the union time grid: concat all channels' keyTimes, sort,
        dedupe in place. Allocates total then shrinks via uniqueCount. */
  totalTimes = 0;
  for (j = 0; j < nBinds; j++) totalTimes += binds[j].keyCount;

  if (totalTimes == 0) {
    /* restore + bail */
    for (j = 0; j < nBinds; j++) {
      if (binds[j].savedLen > 0)
        memcpy(binds[j].target->pData, binds[j].saved,
               binds[j].savedLen * sizeof(float));
    }
    return NULL;
  }

  allTimes = ak_calloc(NULL, sizeof(float) * totalTimes);
  k = 0;
  for (j = 0; j < nBinds; j++) {
    memcpy(allTimes + k, binds[j].keyTimes,
           binds[j].keyCount * sizeof(float));
    k += binds[j].keyCount;
  }
  qsort(allTimes, totalTimes, sizeof(float), bake_floatcmp);

  uniqueCount = 0;
  for (i = 0; i < totalTimes; i++) {
    if (uniqueCount == 0 || allTimes[i] > allTimes[uniqueCount - 1]) {
      allTimes[uniqueCount++] = allTimes[i];
    }
  }

  /* 4b. Densify: insert subdivisions between adjacent samples whose
        gap exceeds BAKE_MAX_STEP. Without this, sparse keyframes (e.g.
        2 frames spanning a 348° rotation) would force the consumer to
        linearly interpolate matrix elements across the gap — that
        misses the rotation manifold and renders as a near-identity
        wobble. Densifying bounds the per-pair angular delta.

        Use ceil rather than floor: a 50 ms gap with 33.3 ms step
        truncates to 1 subdivision (still 50 ms wide) under floor
        division, leaving the per-pair gap above the stated maximum.
        ceilf bounds it. */
  denseCount = 1;  /* first sample always emitted */
  for (i = 1; i < uniqueCount; i++) {
    gap = allTimes[i] - allTimes[i - 1];
    if (gap > BAKE_MAX_STEP) {
      denseCount += (size_t)ceilf(gap / BAKE_MAX_STEP);
    } else {
      denseCount += 1;
    }
  }

  if (denseCount > uniqueCount) {
    dense      = ak_calloc(NULL, sizeof(float) * denseCount);
    d          = 0;
    dense[d++] = allTimes[0];

    for (i = 1; i < uniqueCount; i++) {
      t0      = allTimes[i - 1];
      t1      = allTimes[i];
      gap     = t1 - t0;
      subdivs = (gap > BAKE_MAX_STEP) ? (size_t)ceilf(gap / BAKE_MAX_STEP) : 1;

      for (s = 1; s <= subdivs; s++) {
        dense[d++] = t0 + (gap * (float)s / (float)subdivs);
      }
    }

    ak_free(allTimes);
    allTimes    = dense;
    uniqueCount = denseCount;
  }

  /* 5. Allocate the result. The matrices/times buffers parent off `out`,
        so a single ak_free(out) cleans up everything. */
  out           = ak_calloc(NULL, sizeof(*out));
  out->count    = (uint32_t)uniqueCount;
  out->matrices = ak_calloc(out, sizeof(float) * 16 * uniqueCount);
  out->times    = ak_calloc(out, sizeof(float) * uniqueCount);

  /* 6. For each unique time, sample channels, mutate AkObjects in place,
        then run the standard combine over node->transform. The combiner
        sees current AkObject state, so the per-sample matrix reflects
        all rotates / translates / scales of the chain. */
  for (t_idx = 0; t_idx < uniqueCount; t_idx++) {
    float t = allTimes[t_idx];
    float matrix[16];

    for (j = 0; j < nBinds; j++) {
      ChannelBind *b   = &binds[j];
      float       *dst = (float *)b->target->pData;
      size_t       n   = b->savedLen;

      if (b->isPartial) {
        /* Single-component animation. The sampler's OUTPUT is scalar
           per keyframe (componentCount==1), so the component index
           into keyValues is 0. We write that into target->pData[off]. */
        if (b->off < n)
          dst[b->off] = bake_sampleScalar(b, 0, t);
      } else {
        /* Whole-target animation: OUTPUT has valueStride components
           per keyframe; one-to-one with target->pData. */
        size_t lim = b->valueStride < n ? b->valueStride : n;
        for (k = 0; k < lim; k++)
          dst[k] = bake_sampleScalar(b, k, t);
      }
    }

    matrix[0]  = 1; matrix[1]  = 0; matrix[2]  = 0; matrix[3]  = 0;
    matrix[4]  = 0; matrix[5]  = 1; matrix[6]  = 0; matrix[7]  = 0;
    matrix[8]  = 0; matrix[9]  = 0; matrix[10] = 1; matrix[11] = 0;
    matrix[12] = 0; matrix[13] = 0; matrix[14] = 0; matrix[15] = 1;
    ak_transformCombine(node->transform, matrix);

    memcpy(out->matrices + t_idx * 16, matrix, 16 * sizeof(float));
    out->times[t_idx] = t;
  }

  /* 7. Restore static AkObject payloads. Bind pose is now untouched. */
  for (j = 0; j < nBinds; j++) {
    if (binds[j].savedLen > 0)
      memcpy(binds[j].target->pData, binds[j].saved,
             binds[j].savedLen * sizeof(float));
  }

  ak_free(allTimes);
  return out;
}
