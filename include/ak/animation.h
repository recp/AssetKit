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

#ifndef assetkit_animation_h
#define assetkit_animation_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
  
#include <stdint.h>
#include <stdbool.h>
#include "type.h"
#include "source.h"
#include "url.h"

typedef enum AkSamplerBehavior {
  AK_SAMPLER_BEHAVIOR_UNDEFINED      = 0,
  AK_SAMPLER_BEHAVIOR_CONSTANT       = 1,
  AK_SAMPLER_BEHAVIOR_GRADIENT       = 2,
  AK_SAMPLER_BEHAVIOR_CYCLE          = 3,
  AK_SAMPLER_BEHAVIOR_OSCILLATE      = 4,
  AK_SAMPLER_BEHAVIOR_CYCLE_RELATIVE = 5
} AkSamplerBehavior;

typedef enum AkTargetPropertyType {
  AK_TARGET_UNKNOWN  = 0,
  AK_TARGET_X        = 1,
  AK_TARGET_Y        = 2,
  AK_TARGET_Z        = 3,
  AK_TARGET_XY       = 4,
  AK_TARGET_XYZ      = 5,
  AK_TARGET_ANGLE    = 6,
  AK_TARGET_POSITION = 7,
  AK_TARGET_SCALE    = 8,
  AK_TARGET_ROTATE   = 9,
  AK_TARGET_QUAT     = 10,
  AK_TARGET_WEIGHTS  = 11,
  AK_TARGET_FLOAT    = 12,
  AK_TARGET_VEC2     = 13,
  AK_TARGET_VEC3     = 14,
  AK_TARGET_VEC4     = 15,
  AK_TARGET_COLOR    = 16,
  AK_TARGET_BOOL     = 17
} AkTargetPropertyType;

typedef enum AkInterpolationType {
  AK_INTERPOLATION_UNKNOWN  = 0,
  AK_INTERPOLATION_LINEAR   = 1,
  AK_INTERPOLATION_BEZIER   = 2,
  AK_INTERPOLATION_CARDINAL = 3,
  AK_INTERPOLATION_HERMITE  = 4,
  AK_INTERPOLATION_BSPLINE  = 5,
  AK_INTERPOLATION_STEP     = 6,

  AK_INTERPOLATION_MAXLEN   = 255
} AkInterpolationType;

typedef struct AkAnimSampler {
  AkOneWayIterBase      base;
  AkInput              *input;

  AkInput              *inputInput;
  AkInput              *outputInput;
  AkInput              *interpInput;
  AkInput              *inTangentInput;
  AkInput              *outTangentInput;

  AkInterpolationType   uniInterpolation;
  AkSamplerBehavior     pre;
  AkSamplerBehavior     post;
} AkAnimSampler;

typedef struct AkResolvedTarget {
  void    *target;
  uint32_t off;
  bool     isPartial;
} AkResolvedTarget;

typedef struct AkChannel {
  struct AkChannel    *next;
  const char          *target;
  AkResolvedTarget    *resolvedTarget;
  AkURL                source;
  AkTargetPropertyType targetType;
} AkChannel;

typedef struct AkAnimation {
  struct AkAnimation *next;
  struct AkAnimation *animation; /* subanimation */
  AkAnimSampler      *sampler;
  AkChannel          *channel;
  const char         *name;
  AkTree             *extra;
} AkAnimation;

/*!
 * Per-frame transform sequence produced by ak_nodeBakeAnimation().
 * `matrices` is `count × 16` floats, column-major (cglm convention),
 * each block mapping a node-local point into its parent's space.
 * `times` is `count` floats, parallel to the matrix array.
 *
 * Free with ak_free(out) — the inner buffers were sub-allocated under
 * the struct and cascade in the AssetKit heap.
 */
typedef struct AkBakedAnimation {
  float    *matrices;  /* count × 16 floats, column-major */
  float    *times;     /* count floats                    */
  uint32_t  count;
} AkBakedAnimation;

AK_INLINE
const char*
ak_channelTargetDot(const char *target) {
  if (!target)
    return NULL;

  while (*target) {
    if (*target == '.')
      return target;
    target++;
  }

  return NULL;
}

AK_INLINE
bool
ak_channelTargetIsPartial(const AkChannel *ch) {
  return ch->target && ak_channelTargetDot(ch->target) != NULL;
}

/**
 * returns NULL if no attribute (whole target animation),
 * otherwise pointer into ch->target after the '.'
 * result lifetime tied to ch->target — do NOT free
 */
AK_INLINE
const char *
ak_channelTargetAttr(const AkChannel *ch) {
  const char *dot;
  if (!ch->target || !(dot = ak_channelTargetDot(ch->target))) return NULL;
  return dot + 1;
}

AK_INLINE
AkResolvedTarget
ak_channelTarget(AkContext * __restrict ctx,
                 AkChannel * __restrict ch) {
  const char      *sidAttrib;
  AkResolvedTarget resolved = {NULL, 0, false};
  uint32_t         attrOff;

  /** Importers may provide a pre-resolved target. Honor it first — the
      SID/string path in ch->target is then optional and used only as a
      debug/lookup hint.

      Whole-target example: target => "translation"
        ch->resolvedTarget = AkResolvedTarget* with target = AkTranslate*
        and off = 0.

      Indexed-target example for morph weights: target => "morph-weights(0)"
        fixup resolves the source-and-(idx) pattern and writes
        ch->resolvedTarget = AkResolvedTarget* with target = AkInstanceMorph*,
        off = idx, isPartial = true. */
  if (ch->resolvedTarget)
    return *ch->resolvedTarget;

  /** SID path: "node1/translate.Y"
      resolved.target = AkTranslate*, sidAttrib = "Y" */
  if (ch->target) {
    if ((resolved.target = ak_sid_resolve(ctx, ch->target, &sidAttrib))
        && (attrOff = ak_sid_attr_offset(sidAttrib)) != UINT32_MAX) {
      resolved.isPartial = sidAttrib != NULL;
      resolved.off       = attrOff;
    } else {
      resolved.target    = NULL;
    }
  }

  return resolved;
}

#define ak_inputBegin(INP, T) (*(T*)INP->data)
#define ak_inputEnd(INP, T)   (*(T*)((char*)INP->data + INP->len - sizeof(T)))

/*!
 * @brief Test whether two animations would write to any of the same
 *        animatable slot. Two channels conflict iff they resolve (via
 *        ak_channelTarget) to the same target pointer AND either at least
 *        one is a whole-target write, or they share the same partial slot
 *        offset.
 *
 *        Useful for runtime players that pick which animations may run in
 *        parallel — overlapping writes otherwise produce undefined ordering.
 *
 * @param ctx resolution context (used to evaluate SID-targeted channels)
 * @param a   first animation
 * @param b   second animation
 * @return    true iff any pair of channels conflicts
 */
AK_EXPORT
bool
ak_animationsConflict(AkContext   * __restrict ctx,
                      AkAnimation * __restrict a,
                      AkAnimation * __restrict b);

/*!
 * @brief Build the maximal conflict-free set anchored at `primary`.
 *
 *        `primary` is always selected (it's the animation the caller wants
 *        to activate). Then each candidate is tested against everything
 *        already selected — added if it doesn't conflict with any of them.
 *
 *        First-fit greedy: candidate iteration order decides which side
 *        of a conflict wins. Pass candidates in the priority order you
 *        want (typically: doc order with `primary` excluded).
 *
 *        Use case: a UI like "user clicked Animation 2 — what other
 *        animations can stay enabled in parallel?"
 *
 * @param ctx              resolution context
 * @param primary          anchor animation (must be in result, may be NULL)
 * @param candidates       array of candidate AkAnimation* pointers
 * @param candidatesCount  length of candidates
 * @param outCompatible    pre-allocated buffer of at least
 *                         `candidatesCount + 1` AkAnimation* slots
 * @return count of selected animations (= written into outCompatible)
 */
AK_EXPORT
size_t
ak_animationsCompatibleSet(AkContext         * __restrict ctx,
                           AkAnimation       * __restrict primary,
                           AkAnimation      ** __restrict candidates,
                           size_t                         candidatesCount,
                           AkAnimation      ** __restrict outCompatible);

/*!
 * @brief Total number of animations across every animation library on the
 *        document. Useful for sizing buffers passed to the *FromDoc
 *        compatible-set helper.
 */
AK_EXPORT
size_t
ak_animationsCount(struct AkDoc * __restrict doc);

/*!
 * @brief Compute the authored time range covered by an animation and its
 *        child animation tree.
 *
 *        The function scans INPUT sampler accessors only; it does not allocate
 *        and it does not resolve channel targets. Returns false when no valid
 *        key time accessor is reachable.
 */
AK_EXPORT
bool
ak_animationTimeRange(AkAnimation * __restrict anim,
                      float       * __restrict outStart,
                      float       * __restrict outEnd);

/*!
 * @brief Convenience over `ak_animationsCompatibleSet` that walks the
 *        document's animation libraries itself — so callers don't have to
 *        materialise a `candidates[]` array.
 *
 * @param ctx           resolution context
 * @param doc           the AssetKit document
 * @param primary       anchor animation (must be in result, may be NULL)
 * @param outCompatible pre-allocated buffer of at least
 *                      `ak_animationsCount(doc) + 1` slots
 * @return count of selected animations
 */
AK_EXPORT
size_t
ak_animationsCompatibleSetFromDoc(AkContext     * __restrict ctx,
                                  struct AkDoc  * __restrict doc,
                                  AkAnimation   * __restrict primary,
                                  AkAnimation  ** __restrict outCompatible);


/*!
 * @brief Hint that the node's animation should be baked rather than
 *        driven via per-property channels.
 *
 *        Returns true when the node's transform chain holds 2+ rotate
 *        elements — the canonical case is a Maya joint with
 *        jointOrient{XYZ} + rotate{XYZ} (six <rotate> elements in one
 *        chain, only three Euler slots in any decomposed-property
 *        animation API: SCNNode.eulerAngles, three.js Object3D.rotation,
 *        Filament TransformManager rotation).
 *
 *        Renderers that animate via decomposed properties MUST bake
 *        these nodes — partial rotates clobber each other in the
 *        Euler slot. Matrix-driven runtimes that compose joint world matrices
 *        on the CPU per frame don't need this and can keep their per-channel
 *        walk.
 */
AK_EXPORT
bool
ak_nodeNeedsBaking(struct AkNode * __restrict node);

/*!
 * @brief Sample every animation channel that targets any AkObject in
 *        `node->transform` (translate / rotate / scale / matrix /
 *        skew / quat) on a shared time grid and emit a stream of 4×4
 *        local matrices.
 *
 *        Time grid is the union of the involved channels' keyframe
 *        times (no resampling — every original keyframe is preserved
 *        exactly; channels that lack a value at some t are linearly
 *        interpolated). STEP interpolation is honored; BEZIER /
 *        HERMITE fall back to LINEAR (the bake is keyframe-aligned,
 *        so engine-side interpolation can refine the curve).
 *
 *        Static AkObjects in the chain (those with no targeting
 *        channel) keep their authored values — bind pose is preserved
 *        between animated frames. The function snapshot/restores
 *        animated AkObject state so callers can keep using
 *        node->transform for bind-pose composition afterwards.
 *
 *        Output AkBakedAnimation is heap-allocated; caller frees with
 *        ak_free(out). Returns NULL when the node has no transform or
 *        no channel targets any element of its chain.
 */
AK_EXPORT
AkBakedAnimation*
ak_nodeBakeAnimation(struct AkDoc  * __restrict doc,
                     struct AkNode * __restrict node);

/*!
 * @brief Same as ak_nodeBakeAnimation(), but only samples channels
 *        reachable from `animation` and its child animation tree.
 *
 *        This is useful for runtimes that expose independent animation
 *        clips: a baked multi-rotate joint still needs a matrix track,
 *        but the track must not be built from every clip in the document.
 *
 *        Output AkBakedAnimation is heap-allocated; caller frees with
 *        ak_free(out). Returns NULL when the node has no transform, the
 *        animation is NULL, or no channel in that animation targets any
 *        element of the node's transform chain.
 */
AK_EXPORT
AkBakedAnimation*
ak_nodeBakeAnimationForAnimation(struct AkDoc       * __restrict doc,
                                 struct AkNode      * __restrict node,
                                 struct AkAnimation * __restrict animation);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_animation_h */
