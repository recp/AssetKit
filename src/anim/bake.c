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
#include "accessor.h"
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

  float        *keyTimes;
  size_t        keyCount;
  float        *keyValues;  /* contiguous; valueStride floats per frame   */
  size_t        valueStride;

  AkInterpolationType *interpolations; /* keyCount × valueStride */
  float               *inTangents;     /* keyCount × valueStride */
  float               *outTangents;    /* keyCount × valueStride */
  float               *inTangentTimes; /* optional, same layout  */
  float               *outTangentTimes;
  AkSamplerBehavior     pre;
  AkSamplerBehavior     post;

  float         saved[16];  /* snapshot of target->pData at function entry */
  size_t        savedLen;   /* number of floats valid in saved[]           */
} ChannelBind;

static void
bake_bindDestroy(ChannelBind *bind) {
  if (!bind)
    return;
  free(bind->keyTimes);
  free(bind->keyValues);
  free(bind->interpolations);
  free(bind->inTangents);
  free(bind->outTangents);
  free(bind->inTangentTimes);
  free(bind->outTangentTimes);
  memset(bind, 0, sizeof(*bind));
}

static void
bake_bindsDestroy(ChannelBind *binds, int count) {
  int i;

  for (i = 0; i < count; i++)
    bake_bindDestroy(&binds[i]);
}

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

static AkInput*
bake_samplerInput(AkAnimSampler *sampler, AkInputSemantic semantic) {
  AkInput *input;

  if (!sampler)
    return NULL;
  switch (semantic) {
    case AK_INPUT_INPUT:
      if (sampler->inputInput) return sampler->inputInput;
      break;
    case AK_INPUT_OUTPUT:
      if (sampler->outputInput) return sampler->outputInput;
      break;
    case AK_INPUT_INTERPOLATION:
      if (sampler->interpInput) return sampler->interpInput;
      break;
    case AK_INPUT_IN_TANGENT:
      if (sampler->inTangentInput) return sampler->inTangentInput;
      break;
    case AK_INPUT_OUT_TANGENT:
      if (sampler->outTangentInput) return sampler->outTangentInput;
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

static bool
bake_decodeFloatAccessor(AkAccessor *accessor,
                         float     **valuesOut,
                         size_t     *countOut,
                         size_t     *componentsOut) {
  AkAnimAccessorView view;
  float             *values;
  size_t             valueCount, row, component;

  if (!valuesOut || !countOut || !componentsOut
      || !ak_animAccessorFloatView(accessor, &view)
      || view.count == 0u)
    return false;
  if ((size_t)view.count > SIZE_MAX / view.components)
    return false;
  valueCount = (size_t)view.count * view.components;
  if (valueCount > SIZE_MAX / sizeof(*values))
    return false;
  values = malloc(valueCount * sizeof(*values));
  if (!values)
    return false;

  for (row = 0u; row < view.count; row++) {
    for (component = 0u; component < view.components; component++) {
      float value;
      if (!ak_animAccessorReadFloat(&view,
                                    (uint32_t)row,
                                    (uint32_t)component,
                                    &value)
          || !isfinite(value)) {
        free(values);
        return false;
      }
      values[row * view.components + component] = value;
    }
  }

  *valuesOut     = values;
  *countOut      = view.count;
  *componentsOut = view.components;
  return true;
}

static bool
bake_decodeInterpolations(AkAccessor          *accessor,
                          size_t               keyCount,
                          size_t               valueStride,
                          AkInterpolationType **valuesOut) {
  AkAnimAccessorView  view;
  AkInterpolationType *values;
  size_t               row, component, sourceComponent, valueCount;

  if (!accessor || !valuesOut
      || !ak_animAccessorUByteView(accessor, &view)
      || view.count != keyCount
      || (view.components != 1u && view.components != valueStride)
      || keyCount > SIZE_MAX / valueStride)
    return false;
  valueCount = keyCount * valueStride;
  if (valueCount > SIZE_MAX / sizeof(*values))
    return false;
  values = malloc(valueCount * sizeof(*values));
  if (!values)
    return false;

  for (row = 0u; row < keyCount; row++) {
    for (component = 0u; component < valueStride; component++) {
      uint8_t raw;
      sourceComponent = view.components == 1u ? 0u : component;
      if (!ak_animAccessorReadUByte(&view,
                                   (uint32_t)row,
                                   (uint32_t)sourceComponent,
                                   &raw)
          || (raw != AK_INTERPOLATION_LINEAR
              && raw != AK_INTERPOLATION_STEP
              && raw != AK_INTERPOLATION_BEZIER
              && raw != AK_INTERPOLATION_HERMITE)) {
        free(values);
        return false;
      }
      values[row * valueStride + component] = (AkInterpolationType)raw;
    }
  }
  *valuesOut = values;
  return true;
}

static bool
bake_decodeTangents(AkAccessor *accessor,
                    size_t      keyCount,
                    size_t      valueStride,
                    float     **valuesOut,
                    float     **timesOut) {
  float  *source, *values, *times;
  size_t  count, components, valueCount, row, component;
  bool    paired, sharedTime;

  source = values = times = NULL;
  if (!bake_decodeFloatAccessor(accessor, &source, &count, &components)
      || count != keyCount
      || keyCount > SIZE_MAX / valueStride)
    goto fail;

  paired = valueStride <= SIZE_MAX / 2u
           && components == valueStride * 2u;
  sharedTime = valueStride < SIZE_MAX
               && components == valueStride + 1u;
  if (components != valueStride && !paired && !sharedTime)
    goto fail;

  valueCount = keyCount * valueStride;
  if (valueCount > SIZE_MAX / sizeof(*values))
    goto fail;
  values = malloc(valueCount * sizeof(*values));
  if (!values)
    goto fail;
  if (paired || sharedTime) {
    times = malloc(valueCount * sizeof(*times));
    if (!times)
      goto fail;
  }

  for (row = 0u; row < keyCount; row++) {
    for (component = 0u; component < valueStride; component++) {
      size_t sourceBase = row * components;
      if (paired) {
        times[row * valueStride + component]
          = source[sourceBase + component * 2u];
        values[row * valueStride + component]
          = source[sourceBase + component * 2u + 1u];
      } else if (sharedTime) {
        times[row * valueStride + component] = source[sourceBase];
        values[row * valueStride + component]
          = source[sourceBase + component + 1u];
      } else {
        values[row * valueStride + component]
          = source[sourceBase + component];
      }
    }
  }

  free(source);
  *valuesOut = values;
  *timesOut  = times;
  return true;

fail:
  free(source);
  free(values);
  free(times);
  return false;
}

static float
bake_cubicBezier(float p0, float p1, float p2, float p3, float u) {
  float oneMinus, a, b;

  oneMinus = 1.0f - u;
  a = oneMinus * oneMinus;
  b = u * u;
  return a * oneMinus * p0
         + 3.0f * a * u * p1
         + 3.0f * oneMinus * b * p2
         + b * u * p3;
}

static float
bake_cubicHermite(float p0, float t0, float p1, float t1, float u) {
  float u2, u3;

  u2 = u * u;
  u3 = u2 * u;
  return (2.0f * u3 - 3.0f * u2 + 1.0f) * p0
         + (u3 - 2.0f * u2 + u) * t0
         + (-2.0f * u3 + 3.0f * u2) * p1
         + (u3 - u2) * t1;
}

static float
bake_hermiteParameterForTime(float t0,
                             float tangent0,
                             float t1,
                             float tangent1,
                             float time) {
  float delta, a, b, derivative, critical;
  float lo, hi, mid, value;
  int   i;

  /* COLLADA's paired HERMITE tangents are two-dimensional vectors, not
     absolute control points. Their first component parameterizes time.
     Reject a time curve that is not monotonic because it does not define a
     single animation value for every requested time in this segment. */
  if (tangent0 < 0.0f || tangent1 < 0.0f)
    return NAN;
  delta = t1 - t0;
  a = 3.0f * delta - 2.0f * tangent0 - tangent1;
  b = -2.0f * delta + tangent0 + tangent1;
  if (b > 0.0f) {
    critical = -a / (3.0f * b);
    if (critical > 0.0f && critical < 1.0f) {
      derivative = tangent0
                   + 2.0f * a * critical
                   + 3.0f * b * critical * critical;
      if (derivative < -1e-6f)
        return NAN;
    }
  }

  lo = 0.0f;
  hi = 1.0f;
  for (i = 0; i < 24; i++) {
    mid = (lo + hi) * 0.5f;
    value = bake_cubicHermite(t0, tangent0, t1, tangent1, mid);
    if (value < time)
      lo = mid;
    else
      hi = mid;
  }
  return (lo + hi) * 0.5f;
}

static float
bake_bezierParameterForTime(float t0,
                            float c0,
                            float c1,
                            float t1,
                            float time) {
  float lo, hi, mid, value;
  int   i;

  /* Valid animation curves are monotonic in the time dimension. Clamp bad
     producer handles to the segment so bisection stays deterministic. */
  if (c0 < t0) c0 = t0;
  if (c0 > t1) c0 = t1;
  if (c1 < t0) c1 = t0;
  if (c1 > t1) c1 = t1;
  lo = 0.0f;
  hi = 1.0f;
  for (i = 0; i < 24; i++) {
    mid = (lo + hi) * 0.5f;
    value = bake_cubicBezier(t0, c0, c1, t1, mid);
    if (value < time)
      lo = mid;
    else
      hi = mid;
  }
  return (lo + hi) * 0.5f;
}

static float
bake_segmentSlope(const ChannelBind *b,
                  size_t             key,
                  size_t             component,
                  bool               outgoing) {
  size_t idx;
  float  tangent, tangentTime, denom;

  idx = key * b->valueStride + component;
  tangent = outgoing ? b->outTangents[idx] : b->inTangents[idx];
  if (!(outgoing ? b->outTangentTimes : b->inTangentTimes))
    return tangent;
  tangentTime = outgoing ? b->outTangentTimes[idx]
                         : b->inTangentTimes[idx];
  denom = tangentTime;
  return fabsf(denom) > 1e-20f ? tangent / denom : 0.0f;
}

static float
bake_sampleInside(const ChannelBind *b,
                  size_t             segment,
                  size_t             component,
                  float              t) {
  size_t idx0, idx1;
  float  t0, t1, v0, v1, alpha, dt, u;
  AkInterpolationType interpolation;

  idx0 = segment * b->valueStride + component;
  idx1 = (segment + 1u) * b->valueStride + component;
  t0 = b->keyTimes[segment];
  t1 = b->keyTimes[segment + 1u];
  v0 = b->keyValues[idx0];
  v1 = b->keyValues[idx1];
  dt = t1 - t0;
  alpha = dt > 0.0f ? (t - t0) / dt : 0.0f;
  if (alpha < 0.0f) alpha = 0.0f;
  if (alpha > 1.0f) alpha = 1.0f;
  interpolation = b->interpolations[idx0];

  switch (interpolation) {
    case AK_INTERPOLATION_STEP:
      return v0;
    case AK_INTERPOLATION_BEZIER: {
      float c0, c1;
      c0 = b->outTangents[idx0];
      c1 = b->inTangents[idx1];
      u = alpha;
      if (b->outTangentTimes && b->inTangentTimes) {
        u = bake_bezierParameterForTime(t0,
                                       b->outTangentTimes[idx0],
                                       b->inTangentTimes[idx1],
                                       t1,
                                       t);
      }
      return bake_cubicBezier(v0, c0, c1, v1, u);
    }
    case AK_INTERPOLATION_HERMITE: {
      float m0, m1, u2, u3;
      if (b->outTangentTimes && b->inTangentTimes) {
        u = bake_hermiteParameterForTime(t0,
                                        b->outTangentTimes[idx0],
                                        t1,
                                        b->inTangentTimes[idx1],
                                        t);
        if (!isfinite(u))
          return NAN;
        return bake_cubicHermite(v0,
                                 b->outTangents[idx0],
                                 v1,
                                 b->inTangents[idx1],
                                 u);
      }
      m0 = bake_segmentSlope(b, segment, component, true);
      m1 = bake_segmentSlope(b, segment + 1u, component, false);
      u2 = alpha * alpha;
      u3 = u2 * alpha;
      return (2.0f * u3 - 3.0f * u2 + 1.0f) * v0
             + (u3 - 2.0f * u2 + alpha) * dt * m0
             + (-2.0f * u3 + 3.0f * u2) * v1
             + (u3 - u2) * dt * m1;
    }
    case AK_INTERPOLATION_LINEAR:
    default:
      return v0 * (1.0f - alpha) + v1 * alpha;
  }
}

static float
bake_boundarySlope(const ChannelBind *b,
                   size_t             component,
                   bool               post) {
  size_t key, idx0, idx1;
  float  dt;
  AkInterpolationType interpolation;

  if (b->keyCount < 2u)
    return 0.0f;
  key = post ? b->keyCount - 1u : 0u;
  interpolation = b->interpolations[(post ? key - 1u : key)
                                    * b->valueStride + component];
  if (interpolation == AK_INTERPOLATION_HERMITE)
    return bake_segmentSlope(b, key, component, !post);
  if (interpolation == AK_INTERPOLATION_BEZIER) {
    size_t idx = key * b->valueStride + component;
    float tangent = post ? b->inTangents[idx] : b->outTangents[idx];
    float value = b->keyValues[idx];
    float tangentTime;
    if (post ? b->inTangentTimes : b->outTangentTimes) {
      tangentTime = post ? b->inTangentTimes[idx]
                         : b->outTangentTimes[idx];
      dt = tangentTime - b->keyTimes[key];
      if (fabsf(dt) > 1e-20f)
        return (tangent - value) / dt;
    }
    if (post) {
      dt = b->keyTimes[key] - b->keyTimes[key - 1u];
      return dt > 0.0f ? 3.0f * (value - tangent) / dt : 0.0f;
    }
    dt = b->keyTimes[1u] - b->keyTimes[0u];
    return dt > 0.0f ? 3.0f * (tangent - value) / dt : 0.0f;
  }
  idx0 = (post ? b->keyCount - 2u : 0u) * b->valueStride + component;
  idx1 = (post ? b->keyCount - 1u : 1u) * b->valueStride + component;
  dt = b->keyTimes[post ? b->keyCount - 1u : 1u]
       - b->keyTimes[post ? b->keyCount - 2u : 0u];
  return dt > 0.0f ? (b->keyValues[idx1] - b->keyValues[idx0]) / dt : 0.0f;
}

static float
bake_sampleScalar(const ChannelBind *b, size_t componentIdx, float t) {
  size_t ki, last;
  float  firstTime, lastTime, duration, local, cycleOffset, firstValue;
  bool   outsidePost;
  AkSamplerBehavior behavior;

  if (b->keyCount == 0) return 0.0f;
  last       = b->keyCount - 1u;
  firstTime  = b->keyTimes[0];
  lastTime   = b->keyTimes[last];
  firstValue = b->keyValues[componentIdx];
  if (b->keyCount == 1u)
    return firstValue;

  outsidePost = t > lastTime;
  if (t < firstTime || outsidePost) {
    behavior = outsidePost ? b->post : b->pre;
    if (behavior == AK_SAMPLER_BEHAVIOR_UNDEFINED)
      behavior = AK_SAMPLER_BEHAVIOR_CONSTANT;
    if (behavior == AK_SAMPLER_BEHAVIOR_CONSTANT)
      return b->keyValues[(outsidePost ? last : 0u) * b->valueStride
                          + componentIdx];
    if (behavior == AK_SAMPLER_BEHAVIOR_GRADIENT) {
      size_t boundary = outsidePost ? last : 0u;
      float value = b->keyValues[boundary * b->valueStride + componentIdx];
      return value + (t - b->keyTimes[boundary])
                     * bake_boundarySlope(b, componentIdx, outsidePost);
    }

    duration = lastTime - firstTime;
    if (!(duration > 0.0f) || !isfinite(duration))
      return firstValue;
    local = (t - firstTime) / duration;
    if (!isfinite(local))
      return firstValue;
    if (behavior == AK_SAMPLER_BEHAVIOR_OSCILLATE) {
      float phase = fmodf(local, 2.0f);
      if (phase < 0.0f) phase += 2.0f;
      if (phase > 1.0f) phase = 2.0f - phase;
      t = firstTime + phase * duration;
    } else {
      float phase = local - floorf(local);
      t = firstTime + phase * duration;
    }
    cycleOffset = 0.0f;
    if (behavior == AK_SAMPLER_BEHAVIOR_CYCLE_RELATIVE) {
      float delta = b->keyValues[last * b->valueStride + componentIdx]
                    - firstValue;
      float signedCycles = floorf(local);
      cycleOffset = signedCycles * delta;
    }
  } else {
    cycleOffset = 0.0f;
  }

  if (t <= firstTime)
    return firstValue + cycleOffset;
  if (t >= lastTime)
    return b->keyValues[last * b->valueStride + componentIdx] + cycleOffset;

  for (ki = 1u; ki < b->keyCount && b->keyTimes[ki] < t; ki++) { }
  if (b->keyTimes[ki] == t)
    return b->keyValues[ki * b->valueStride + componentIdx] + cycleOffset;
  return bake_sampleInside(b, ki - 1u, componentIdx, t) + cycleOffset;
}

static bool
bake_animationIsExactClipMember(AkDoc       * __restrict doc,
                                AkAnimation * __restrict animation) {
  AkAnimationClip       *clip;
  AkAnimationClipMember *member;

  if (!doc || !animation)
    return false;
  for (clip = doc->animationClips.first; clip; clip = clip->next) {
    for (member = clip->members; member; member = member->next) {
      if (member->animation == animation)
        return true;
    }
  }
  return false;
}

typedef struct BakeAnimationWalk {
  AkAnimation **pending;
  AkAnimation **seen;
  size_t        pendingCount;
  size_t        pendingCapacity;
  size_t        seenCount;
  size_t        seenCapacity;
  bool          failed;
} BakeAnimationWalk;

static bool
bake_animationArrayPush(AkAnimation ***array,
                        size_t        *count,
                        size_t        *capacity,
                        AkAnimation   *animation) {
  AkAnimation **items;
  size_t        newCapacity;

  if (*count == *capacity) {
    newCapacity = *capacity ? *capacity * 2u : 64u;
    if (newCapacity < *capacity
        || newCapacity > SIZE_MAX / sizeof(**array))
      return false;
    items = realloc(*array, newCapacity * sizeof(**array));
    if (!items)
      return false;
    *array    = items;
    *capacity = newCapacity;
  }
  (*array)[(*count)++] = animation;
  return true;
}

static bool
bake_animationWalkHasSeen(BakeAnimationWalk *walk,
                          AkAnimation       *animation) {
  size_t i;

  for (i = 0; i < walk->seenCount; i++) {
    if (walk->seen[i] == animation)
      return true;
  }
  return false;
}

static void
bake_animationWalkDestroy(BakeAnimationWalk *walk) {
  if (!walk)
    return;
  free(walk->pending);
  free(walk->seen);
  memset(walk, 0, sizeof(*walk));
}

static bool
bake_bindSampler(AkAnimSampler * __restrict samp,
                 size_t                      valueStride,
                 ChannelBind   * __restrict bind) {
  AkInput         *inputInput, *outputInput, *interpInput;
  AkInput         *inTangentInput, *outTangentInput;
  float           *decodedOutput;
  size_t           inputCount, inputComponents;
  size_t           outputCount, outputComponents;
  size_t           outputValueCount, packedValueCount;
  size_t           valueCount, row, component;
  AkInterpolationType uniform;
  bool             needsTangents, cubicPacked;
  if (!samp || !bind || !valueStride
      || samp->pre > AK_SAMPLER_BEHAVIOR_CYCLE_RELATIVE
      || samp->post > AK_SAMPLER_BEHAVIOR_CYCLE_RELATIVE)
    return false;
  memset(bind, 0, sizeof(*bind));
  bind->pre         = samp->pre;
  bind->post        = samp->post;
  bind->valueStride = valueStride;
  decodedOutput     = NULL;

  inputInput      = bake_samplerInput(samp, AK_INPUT_INPUT);
  outputInput     = bake_samplerInput(samp, AK_INPUT_OUTPUT);
  interpInput     = bake_samplerInput(samp, AK_INPUT_INTERPOLATION);
  inTangentInput  = bake_samplerInput(samp, AK_INPUT_IN_TANGENT);
  outTangentInput = bake_samplerInput(samp, AK_INPUT_OUT_TANGENT);
  if (!inputInput || !outputInput)
    goto fail;

  if (!bake_decodeFloatAccessor(inputInput->accessor,
                                &bind->keyTimes,
                                &inputCount,
                                &inputComponents)
      || inputComponents != 1u)
    goto fail;
  bind->keyCount = inputCount;
  for (row = 1u; row < bind->keyCount; row++) {
    if (!(bind->keyTimes[row] > bind->keyTimes[row - 1u]))
      goto fail;
  }

  if (!bake_decodeFloatAccessor(outputInput->accessor,
                                &decodedOutput,
                                &outputCount,
                                &outputComponents))
    goto fail;
  if (bind->keyCount > SIZE_MAX / valueStride)
    goto fail;
  valueCount = bind->keyCount * valueStride;
  if (valueCount > SIZE_MAX / sizeof(float))
    goto fail;
  if (outputCount > SIZE_MAX / outputComponents)
    goto fail;
  outputValueCount = outputCount * outputComponents;
  cubicPacked = samp->uniInterpolation == AK_INTERPOLATION_HERMITE
                && !inTangentInput && !outTangentInput
                && valueCount <= SIZE_MAX / 3u
                && outputValueCount == valueCount * 3u;
  packedValueCount = cubicPacked ? valueCount * 3u : valueCount;
  if (outputValueCount != packedValueCount)
    goto fail;
  bind->keyValues = malloc(valueCount * sizeof(float));
  if (!bind->keyValues)
    goto fail;

  if (cubicPacked) {
    bind->inTangents  = malloc(valueCount * sizeof(float));
    bind->outTangents = malloc(valueCount * sizeof(float));
    if (!bind->inTangents || !bind->outTangents)
      goto fail;
    for (row = 0u; row < bind->keyCount; row++) {
      for (component = 0u; component < valueStride; component++) {
        size_t dst = row * valueStride + component;
        bind->inTangents[dst]
          = decodedOutput[(row * 3u) * valueStride + component];
        bind->keyValues[dst]
          = decodedOutput[(row * 3u + 1u) * valueStride + component];
        bind->outTangents[dst]
          = decodedOutput[(row * 3u + 2u) * valueStride + component];
      }
    }
  } else {
    memcpy(bind->keyValues, decodedOutput, valueCount * sizeof(float));
  }
  free(decodedOutput);
  decodedOutput = NULL;

  if (interpInput) {
    if (!bake_decodeInterpolations(interpInput->accessor,
                                   bind->keyCount,
                                   valueStride,
                                   &bind->interpolations))
      goto fail;
  } else {
    uniform = samp->uniInterpolation;
    if (uniform == AK_INTERPOLATION_UNKNOWN)
      uniform = AK_INTERPOLATION_LINEAR;
    if (uniform != AK_INTERPOLATION_LINEAR
        && uniform != AK_INTERPOLATION_STEP
        && uniform != AK_INTERPOLATION_BEZIER
        && uniform != AK_INTERPOLATION_HERMITE)
      goto fail;
    bind->interpolations = malloc(valueCount
                                  * sizeof(*bind->interpolations));
    if (!bind->interpolations)
      goto fail;
    for (row = 0u; row < valueCount; row++)
      bind->interpolations[row] = uniform;
  }

  needsTangents = false;
  for (row = 0u; row + 1u < bind->keyCount && !needsTangents; row++) {
    for (component = 0u; component < valueStride; component++) {
      AkInterpolationType interpolation
        = bind->interpolations[row * valueStride + component];
      if (interpolation == AK_INTERPOLATION_BEZIER
          || interpolation == AK_INTERPOLATION_HERMITE) {
        needsTangents = true;
        break;
      }
    }
  }
  if (needsTangents && !cubicPacked) {
    if (!inTangentInput || !outTangentInput
        || !bake_decodeTangents(inTangentInput->accessor,
                                bind->keyCount,
                                valueStride,
                                &bind->inTangents,
                                &bind->inTangentTimes)
        || !bake_decodeTangents(outTangentInput->accessor,
                                bind->keyCount,
                                valueStride,
                                &bind->outTangents,
                                &bind->outTangentTimes))
      goto fail;
  }
  return true;

fail:
  free(decodedOutput);
  bake_bindDestroy(bind);
  return false;
}

AK_EXPORT
size_t
ak_animationSamplerSample(AkAnimSampler * __restrict sampler,
                          float                       time,
                          float         * __restrict values,
                          size_t                      capacity) {
  AkInput           *inputInput, *outputInput;
  AkInput           *inTangentInput, *outTangentInput;
  AkAnimAccessorView inputView, outputView;
  ChannelBind        bind;
  size_t             component, componentCount, divisor, outputValueCount;
  bool               cubicPacked;

  if (!sampler || !isfinite(time))
    return 0u;
  inputInput  = bake_samplerInput(sampler, AK_INPUT_INPUT);
  outputInput = bake_samplerInput(sampler, AK_INPUT_OUTPUT);
  if (!inputInput || !inputInput->accessor
      || !outputInput || !outputInput->accessor
      || !ak_animAccessorFloatView(inputInput->accessor, &inputView)
      || inputView.components != 1u
      || inputView.count == 0u
      || !ak_animAccessorFloatView(outputInput->accessor, &outputView)
      || outputView.count == 0u
      || (size_t)outputView.count > SIZE_MAX / outputView.components)
    return 0u;
  outputValueCount = (size_t)outputView.count * outputView.components;
  inTangentInput  = bake_samplerInput(sampler, AK_INPUT_IN_TANGENT);
  outTangentInput = bake_samplerInput(sampler, AK_INPUT_OUT_TANGENT);
  cubicPacked = sampler->uniInterpolation == AK_INTERPOLATION_HERMITE
                && !inTangentInput && !outTangentInput;
  divisor = inputView.count;
  if (cubicPacked) {
    if (divisor > SIZE_MAX / 3u)
      return 0u;
    divisor *= 3u;
  }
  if (!divisor || outputValueCount % divisor != 0u)
    return 0u;
  componentCount = outputValueCount / divisor;
  if (!componentCount
      || !bake_bindSampler(sampler, componentCount, &bind))
    return 0u;
  for (component = 0u; component < componentCount; component++) {
    float sample = bake_sampleScalar(&bind, component, time);
    if (!isfinite(sample)) {
      bake_bindDestroy(&bind);
      return 0u;
    }
    if (values && capacity >= componentCount)
      values[component] = sample;
  }
  bake_bindDestroy(&bind);
  return componentCount;
}

static void
bake_collectAnimationChannels(AkAnimation  * __restrict anim,
                              AkContext    * __restrict actx,
                              AkObject    ** __restrict xformObjects,
                              int                       nXform,
                              ChannelBind  * __restrict binds,
                              int          * __restrict nBinds,
                              bool         * __restrict overflow) {
  AkChannel       *ch;
  AkAnimSampler   *samp;
  AkResolvedTarget rt;
  ChannelBind     *bind;
  size_t           savedLen, valueStride;
  int              j;
  bool             isOurs;

  for (ch = anim ? anim->channel : NULL; ch; ch = ch->next) {
    rt = ak_channelTarget(actx, ch);
    if (!rt.target)
      continue;
    isOurs = false;
    for (j = 0; j < nXform; j++) {
      if (rt.target == xformObjects[j]) {
        isOurs = true;
        break;
      }
    }
    if (!isOurs)
      continue;
    if (*nBinds >= BAKE_MAX_CHANNELS) {
      *overflow = true;
      return;
    }

    savedLen = bake_payloadFloatCount(rt.target);
    if (!savedLen || (rt.isPartial && rt.off >= savedLen)) {
      *overflow = true;
      return;
    }
    valueStride = rt.isPartial ? 1u : savedLen;
    samp = ak_getObjectByUrl(&ch->source);
    bind = &binds[*nBinds];
    if (!bake_bindSampler(samp, valueStride, bind)) {
      *overflow = true;
      return;
    }
    bind->target    = rt.target;
    bind->off       = rt.off;
    bind->isPartial = rt.isPartial;
    bind->savedLen  = savedLen;
    (*nBinds)++;
  }
}

static void
bake_collectAnimationTree(AkDoc       * __restrict doc,
                          AkAnimation * __restrict animation,
                          AkContext   * __restrict actx,
                          AkObject   ** __restrict xformObjects,
                          int                      nXform,
                          ChannelBind * __restrict binds,
                          int         * __restrict nBinds,
                          BakeAnimationWalk * __restrict walk,
                          bool                     skipClipMembers) {
  AkAnimation *current, *child;
  size_t       firstChild, lastChild;
  bool         overflow;

  if (!animation || walk->failed)
    return;
  if (!bake_animationArrayPush(&walk->pending,
                               &walk->pendingCount,
                               &walk->pendingCapacity,
                               animation)) {
    walk->failed = true;
    return;
  }

  while (walk->pendingCount > 0 && !walk->failed) {
    current = walk->pending[--walk->pendingCount];
    if (bake_animationWalkHasSeen(walk, current))
      continue;
    if (!bake_animationArrayPush(&walk->seen,
                                 &walk->seenCount,
                                 &walk->seenCapacity,
                                 current)) {
      walk->failed = true;
      break;
    }
    if (skipClipMembers && bake_animationIsExactClipMember(doc, current))
      continue;

    overflow = false;
    bake_collectAnimationChannels(current, actx,
                                  xformObjects, nXform, binds, nBinds,
                                  &overflow);
    if (overflow) {
      walk->failed = true;
      break;
    }

    /* Push descendant siblings in reverse so LIFO traversal preserves the
     * authored linked-list order.  A root's own `next` is never followed. */
    firstChild = walk->pendingCount;
    for (child = current->animation; child; child = child->next) {
      if (!bake_animationArrayPush(&walk->pending,
                                   &walk->pendingCount,
                                   &walk->pendingCapacity,
                                   child)) {
        walk->failed = true;
        break;
      }
    }
    lastChild = walk->pendingCount;
    while (lastChild > firstChild + 1u) {
      AkAnimation *tmp;
      tmp = walk->pending[firstChild];
      walk->pending[firstChild++] = walk->pending[--lastChild];
      walk->pending[lastChild] = tmp;
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

typedef enum BakeAnimationSelection {
  BAKE_ANIMATION_DOCUMENT,
  BAKE_ANIMATION_SINGLE,
  BAKE_ANIMATION_CLIP,
  BAKE_ANIMATION_UNCLIPPED
} BakeAnimationSelection;

static AkBakedAnimation *
bake_nodeAnimation(AkDoc                  * __restrict doc,
                   AkNode                 * __restrict node,
                   BakeAnimationSelection              selection,
                   AkAnimation            * __restrict animation,
                   AkAnimationClip        * __restrict clip) {
  AkObject         *xformObjects[BAKE_MAX_XFORM_OBJECTS];
  AkObject         *it;
  AkAnimation      *root;
  AkAnimationClipMember *member;
  BakeAnimationWalk walk;
  AkBakedAnimation *out = NULL;
  float            *allTimes = NULL, *dense = NULL;
  ChannelBind       binds[BAKE_MAX_CHANNELS];
  AkContext         actx;
  size_t            totalTimes, allocatedTimes, uniqueCount;
  size_t            denseCount, subdivs, d, s;
  size_t            i, k, t_idx;
  float             t0, t1, gap, rangeStart, rangeEnd;
  int               nXform, nBinds, j;
  bool              bounded;

  if (!doc || !node || !node->transform)
    return NULL;
  if (selection == BAKE_ANIMATION_SINGLE && !animation)
    return NULL;
  if (selection == BAKE_ANIMATION_CLIP && !clip)
    return NULL;

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
  if (it)
    return NULL;

  for (it = node->transform->item;
       it && nXform < BAKE_MAX_XFORM_OBJECTS;
       it = it->next) {
    xformObjects[nXform++] = it;
  }
  if (it)
    return NULL;

  if (nXform == 0) return NULL;

  /* 2. Walk every animation channel; bind those whose resolved target
        is one of our transform AkObjects. */
  memset(&actx, 0, sizeof(actx));

  actx.doc = doc;
  nBinds   = 0;
  memset(&walk, 0, sizeof(walk));
  bounded  = false;
  rangeStart = 0.0f;
  rangeEnd   = 0.0f;

  switch (selection) {
    case BAKE_ANIMATION_DOCUMENT:
    case BAKE_ANIMATION_UNCLIPPED:
      for (root = doc->lib.animations.first;
           root && !walk.failed;
           root = root->next) {
        bake_collectAnimationTree(doc, root, &actx,
                                  xformObjects, nXform, binds, &nBinds,
                                  &walk,
                                  selection == BAKE_ANIMATION_UNCLIPPED);
      }
      break;
    case BAKE_ANIMATION_SINGLE:
      bake_collectAnimationTree(doc, animation, &actx,
                                xformObjects, nXform, binds, &nBinds,
                                &walk, false);
      break;
    case BAKE_ANIMATION_CLIP:
      for (member = clip->members;
           member && !walk.failed;
           member = member->next) {
        bake_collectAnimationTree(doc, member->animation, &actx,
                                  xformObjects, nXform, binds, &nBinds,
                                  &walk, false);
      }
      bounded = ak_animationClipTimeRange(clip, &rangeStart, &rangeEnd);
      if (!bounded)
        walk.failed = true;
      break;
  }

  if (walk.failed) {
    bake_animationWalkDestroy(&walk);
    bake_bindsDestroy(binds, nBinds);
    return NULL;
  }
  bake_animationWalkDestroy(&walk);

  if (nBinds == 0) return NULL;

  /* 3. Snapshot animated AkObject payloads so we can restore them on
        exit. This lets the caller continue using node->transform for
        bind-pose composition without observing the mutations we make
        per sample. */
  for (j = 0; j < nBinds; j++) {
    if (binds[j].savedLen > 0)
      memcpy(binds[j].saved, binds[j].target->pData,
             binds[j].savedLen * sizeof(float));
  }

  /* 4. Build the union time grid: concat all channels' keyTimes, sort,
        dedupe in place. Allocates total then shrinks via uniqueCount. */
  totalTimes = 0;
  for (j = 0; j < nBinds; j++) {
    if (binds[j].keyCount > SIZE_MAX - totalTimes)
      goto restore_and_fail;
    totalTimes += binds[j].keyCount;
  }

  if (totalTimes == 0) {
    /* restore + bail */
    for (j = 0; j < nBinds; j++) {
      if (binds[j].savedLen > 0)
        memcpy(binds[j].target->pData, binds[j].saved,
               binds[j].savedLen * sizeof(float));
    }
    bake_bindsDestroy(binds, nBinds);
    return NULL;
  }

  allocatedTimes = totalTimes;
  if (bounded) {
    if (allocatedTimes > SIZE_MAX - 2u)
      goto restore_and_fail;
    allocatedTimes += 2u;
  }
  if (allocatedTimes > SIZE_MAX / sizeof(float))
    goto restore_and_fail;

  allTimes = ak_calloc(NULL, sizeof(float) * allocatedTimes);
  if (!allTimes)
    goto restore_and_fail;
  k = 0;
  for (j = 0; j < nBinds; j++) {
    for (i = 0; i < binds[j].keyCount; i++) {
      float keyTime = binds[j].keyTimes[i];
      if (!isfinite(keyTime))
        continue;
      if (!bounded || (keyTime >= rangeStart && keyTime <= rangeEnd))
        allTimes[k++] = keyTime;
    }
  }
  if (bounded) {
    allTimes[k++] = rangeStart;
    if (rangeEnd > rangeStart)
      allTimes[k++] = rangeEnd;
  }
  if (k == 0) {
    goto restore_and_fail;
  }
  qsort(allTimes, k, sizeof(float), bake_floatcmp);

  uniqueCount = 0;
  for (i = 0; i < k; i++) {
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
      float subdivisionCount = ceilf(gap / BAKE_MAX_STEP);
      if (!isfinite(subdivisionCount)
          || subdivisionCount > (float)UINT32_MAX
          || (size_t)subdivisionCount > SIZE_MAX - denseCount) {
        goto restore_and_fail;
      }
      denseCount += (size_t)subdivisionCount;
    } else {
      if (denseCount == SIZE_MAX) {
        goto restore_and_fail;
      }
      denseCount += 1;
    }
  }

  if (denseCount > UINT32_MAX || denseCount > SIZE_MAX / sizeof(float)) {
    goto restore_and_fail;
  }

  if (denseCount > uniqueCount) {
    dense      = ak_calloc(NULL, sizeof(float) * denseCount);
    if (!dense)
      goto restore_and_fail;
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
  if (uniqueCount > SIZE_MAX / (16u * sizeof(float))) {
    goto restore_and_fail;
  }
  out           = ak_calloc(NULL, sizeof(*out));
  if (!out)
    goto restore_and_fail;
  out->count    = (uint32_t)uniqueCount;
  out->matrices = ak_calloc(out, sizeof(float) * 16 * uniqueCount);
  out->times    = ak_calloc(out, sizeof(float) * uniqueCount);
  if (!out->matrices || !out->times)
    goto restore_and_fail;

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
        if (b->off < n) {
          float sample = bake_sampleScalar(b, 0, t);
          if (!isfinite(sample))
            goto restore_and_fail;
          dst[b->off] = sample;
        }
      } else {
        /* Whole-target animation: OUTPUT has valueStride components
           per keyframe; one-to-one with target->pData. */
        size_t lim = b->valueStride < n ? b->valueStride : n;
        for (k = 0; k < lim; k++) {
          float sample = bake_sampleScalar(b, k, t);
          if (!isfinite(sample))
            goto restore_and_fail;
          dst[k] = sample;
        }
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
  bake_bindsDestroy(binds, nBinds);
  return out;

restore_and_fail:
  if (allTimes)
    ak_free(allTimes);
  if (out)
    ak_free(out);
  for (j = 0; j < nBinds; j++) {
    if (binds[j].savedLen > 0)
      memcpy(binds[j].target->pData, binds[j].saved,
             binds[j].savedLen * sizeof(float));
  }
  bake_bindsDestroy(binds, nBinds);
  return NULL;
}

AK_EXPORT
AkBakedAnimation *
ak_nodeBakeAnimation(AkDoc  * __restrict doc,
                     AkNode * __restrict node) {
  return bake_nodeAnimation(doc, node, BAKE_ANIMATION_DOCUMENT, NULL, NULL);
}

AK_EXPORT
AkBakedAnimation *
ak_nodeBakeAnimationForAnimation(AkDoc       * __restrict doc,
                                 AkNode      * __restrict node,
                                 AkAnimation * __restrict animation) {
  return bake_nodeAnimation(doc, node, BAKE_ANIMATION_SINGLE, animation, NULL);
}

AK_EXPORT
AkBakedAnimation *
ak_nodeBakeAnimationForClip(AkDoc           * __restrict doc,
                            AkNode          * __restrict node,
                            AkAnimationClip * __restrict clip) {
  return bake_nodeAnimation(doc, node, BAKE_ANIMATION_CLIP, NULL, clip);
}

AK_EXPORT
AkBakedAnimation *
ak_nodeBakeUnclippedAnimation(AkDoc  * __restrict doc,
                              AkNode * __restrict node) {
  return bake_nodeAnimation(doc, node, BAKE_ANIMATION_UNCLIPPED, NULL, NULL);
}
