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
 * Animation conflict detection
 * ============================
 *
 * Two animations conflict iff any pair of their channels resolves to the
 * SAME animatable slot — i.e. they would write to the same memory at
 * playback time. Runtimes that play multiple animations in parallel use
 * this to decide which clips can co-exist; overlapping writes otherwise
 * produce undefined ordering.
 *
 * Resolution semantics (matches `ak_channelTarget`):
 *
 *   target  : the struct being animated (AkTransform component, AkInstanceMorph, ...)
 *   off     : LOGICAL slot/component index within target
 *   isPartial: whether the channel writes a single component (true) or the
 *              whole target value (false)
 *
 * Conflict rules between two resolved channels (rta, rtb):
 *
 *   1. rta.target  != rtb.target              → no conflict (disjoint memory)
 *   2. !rta.isPartial OR !rtb.isPartial       → CONFLICT
 *      (a whole-target write covers every slot, so any companion write
 *       on the same target is overlapped)
 *   3. both partial AND rta.off == rtb.off    → CONFLICT (same slot)
 *   4. both partial AND different off         → no conflict
 *
 * This is exhaustive at the AssetKit semantic layer. Bridges may layer
 * further keyPath-level checks on top when their target mapping fans
 * multiple AkObjects to the same engine-side property (e.g. AKT_MATRIX
 * and AKT_TRANSLATE both affect a single node transform).
 */

#include "../common.h"

AK_EXPORT
bool
ak_animationsConflict(AkContext   * __restrict ctx,
                      AkAnimation * __restrict a,
                      AkAnimation * __restrict b) {
  AkChannel        *cha, *chb;
  AkResolvedTarget  rta, rtb;

  if (!a || !b || a == b) return false;

  for (cha = a->channel; cha; cha = cha->next) {
    rta = ak_channelTarget(ctx, cha);
    if (!rta.target) continue;

    for (chb = b->channel; chb; chb = chb->next) {
      rtb = ak_channelTarget(ctx, chb);
      if (!rtb.target || rtb.target != rta.target) continue;

      /* at least one side writes the whole target → overlap */
      if (!rta.isPartial || !rtb.isPartial) return true;

      /* both partial — overlap iff same slot */
      if (rta.off == rtb.off) return true;
    }
  }

  return false;
}

AK_EXPORT
size_t
ak_animationsCompatibleSet(AkContext         * __restrict ctx,
                           AkAnimation       * __restrict primary,
                           AkAnimation      ** __restrict candidates,
                           size_t                         candidatesCount,
                           AkAnimation      ** __restrict outCompatible) {
  size_t selected, i, j;
  bool   conflicts;

  if (!outCompatible) return 0;

  selected = 0;
  if (primary) {
    outCompatible[selected++] = primary;
  }

  if (!candidates || candidatesCount == 0) return selected;

  for (i = 0; i < candidatesCount; i++) {
    if (!candidates[i] || candidates[i] == primary) continue;

    conflicts = false;
    for (j = 0; j < selected; j++) {
      if (ak_animationsConflict(ctx, candidates[i], outCompatible[j])) {
        conflicts = true;
        break;
      }
    }
    if (!conflicts) {
      outCompatible[selected++] = candidates[i];
    }
  }

  return selected;
}

AK_EXPORT
size_t
ak_animationsCount(AkDoc * __restrict doc) {
  AkAnimation *anim;
  size_t       count;

  if (!doc) return 0;

  count = 0;
  for (anim = doc->lib.animations.first; anim; anim = anim->next)
    count++;

  return count;
}

AK_EXPORT
size_t
ak_animationsCompatibleSetFromDoc(AkContext     * __restrict ctx,
                                  AkDoc         * __restrict doc,
                                  AkAnimation   * __restrict primary,
                                  AkAnimation  ** __restrict outCompatible) {
  AkAnimation  *anim;
  AkAnimation **candidates;
  size_t        count, i;

  if (!doc || !outCompatible) {
    /* still honor primary even with no candidates */
    if (primary && outCompatible) {
      outCompatible[0] = primary;
      return 1;
    }
    return 0;
  }

  count = ak_animationsCount(doc);
  if (count == 0) {
    if (primary) { outCompatible[0] = primary; return 1; }
    return 0;
  }

  candidates = AK_ALLOCA(sizeof(*candidates) * count);
  i = 0;
  for (anim = doc->lib.animations.first; anim; anim = anim->next)
    candidates[i++] = anim;

  return ak_animationsCompatibleSet(ctx, primary,
                                    candidates, count, outCompatible);
}
