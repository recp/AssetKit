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
#include <string.h>
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
  AK_TARGET_WEIGHTS  = 11
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
  AkOneWayIterBase    base;
  struct AkAnimation *animation; /* subanimation */
  AkAnimSampler      *sampler;
  AkChannel          *channel;
  const char         *name;
  AkTree             *extra;
  
  /* TODO: WILL BE DELETED */
  AkSource           *source;
} AkAnimation;

AK_INLINE
bool
ak_channelTargetIsPartial(const AkChannel *ch) {
  return ch->target && strchr(ch->target, '.') != NULL;
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
  if (!ch->target || !(dot = strchr(ch->target, '.'))) return NULL;
  return dot + 1;
}

AK_INLINE
AkResolvedTarget
ak_channelTarget(AkContext * __restrict ctx,
                 AkChannel * __restrict ch) {
  const char      *sidAttrib;
  AkResolvedTarget resolved = {0};
  uint32_t         attrOff;

  /** glTF (and DAE post-fixup) provide a pre-resolved target. Honor it
      first — the SID string in ch->target is then optional and used only
      as a debug/lookup hint.

      glTF example: target => "translation"
        ch->resolvedTarget = AkResolvedTarget* with target = AkTranslate*
        and off = 0 (glTF animates the whole transform element).

      DAE example for morph weights: target => "morph-weights(0)"
        dae_fixup_channel resolves the source-and-(idx) pattern at fixup
        time and writes ch->resolvedTarget = AkResolvedTarget* with target
        = AkInstanceMorph*, off = idx, isPartial = true. */
  if (ch->resolvedTarget)
    return *ch->resolvedTarget;

  /** DAE SID path: "node1/translate.Y"
      resolved.target = AkTranslate*, sidAttrib = "Y" */
  if (ch->target) {
    if ((resolved.target = ak_sid_resolve(ctx, ch->target, &sidAttrib))
        && (attrOff = ak_sid_attr_offset(sidAttrib)) != UINT32_MAX) {
      resolved.isPartial = sidAttrib != NULL;
      resolved.off       = attrOff;
      /* for invalid attribute we skip channel for now */
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

#ifdef __cplusplus
}
#endif
#endif /* assetkit_animation_h */
