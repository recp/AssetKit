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
#include "accessor.h"

typedef struct AkAnimationTreeIter {
  AkAnimation **stack;
  AkAnimation **seen;
  size_t       stackCount;
  size_t       stackCapacity;
  size_t       seenCount;
  size_t       seenCapacity;
  bool         failed;
} AkAnimationTreeIter;

static bool
ak_animationTreeIterPush(AkAnimation ***items,
                         size_t        *count,
                         size_t        *capacity,
                         AkAnimation   *animation) {
  AkAnimation **grown;
  size_t        newCapacity;

  if (*count == *capacity) {
    newCapacity = *capacity ? *capacity * 2u : 64u;
    if (newCapacity < *capacity
        || newCapacity > SIZE_MAX / sizeof(**items))
      return false;
    grown = realloc(*items, newCapacity * sizeof(**items));
    if (!grown)
      return false;
    *items = grown;
    *capacity = newCapacity;
  }
  (*items)[(*count)++] = animation;
  return true;
}

static bool
ak_animationTreeIterInit(AkAnimationTreeIter *iter, AkAnimation *root) {
  memset(iter, 0, sizeof(*iter));
  if (root && !ak_animationTreeIterPush(&iter->stack,
                                        &iter->stackCount,
                                        &iter->stackCapacity,
                                        root))
    iter->failed = true;
  return !iter->failed;
}

static void
ak_animationTreeIterDestroy(AkAnimationTreeIter *iter) {
  if (!iter)
    return;
  free(iter->stack);
  free(iter->seen);
  memset(iter, 0, sizeof(*iter));
}

static AkAnimation*
ak_animationTreeIterNext(AkAnimationTreeIter *iter) {
  AkAnimation *animation, *child;
  size_t       i;

  while (iter->stackCount > 0) {
    animation = iter->stack[--iter->stackCount];
    for (i = 0; i < iter->seenCount; i++) {
      if (iter->seen[i] == animation)
        break;
    }
    if (i < iter->seenCount)
      continue;
    if (!ak_animationTreeIterPush(&iter->seen,
                                  &iter->seenCount,
                                  &iter->seenCapacity,
                                  animation)) {
      iter->failed = true;
      return NULL;
    }

    /* A root's `next` is owned by its caller.  Descendant sibling lists are
     * part of the root subtree and are therefore pushed here in full. */
    for (child = animation->animation; child; child = child->next) {
      if (!ak_animationTreeIterPush(&iter->stack,
                                    &iter->stackCount,
                                    &iter->stackCapacity,
                                    child)) {
        iter->failed = true;
        return NULL;
      }
    }
    return animation;
  }

  return NULL;
}

static bool
ak_channelTargets(AkContext        *ctx,
                  AkChannel        *channel,
                  AkResolvedTarget  local[1],
                  AkResolvedTarget **targetsOut,
                  size_t           *countOut) {
  AkResolvedTarget *targets;
  size_t            count, confirmed;

  *targetsOut = NULL;
  *countOut   = 0;
  count = ak_channelResolvedTargets(ctx, channel, NULL, 0);
  if (count == SIZE_MAX)
    return false;
  if (!count)
    return true;

  if (count == 1) {
    targets = local;
  } else {
    if (count > SIZE_MAX / sizeof(*targets))
      return false;
    targets = malloc(count * sizeof(*targets));
    if (!targets)
      return false;
  }

  confirmed = ak_channelResolvedTargets(ctx, channel, targets, count);
  if (confirmed != count) {
    if (targets != local)
      free(targets);
    return false;
  }

  *targetsOut = targets;
  *countOut   = count;
  return true;
}

static bool
ak_resolvedChannelsConflict(AkContext *ctx,
                            AkChannel *a,
                            AkChannel *b) {
  AkResolvedTarget  localA[1], localB[1];
  AkResolvedTarget *targetsA, *targetsB;
  size_t            countA, countB, i, j;
  bool              conflict;

  for (; a; a = a->next) {
    if (!ak_channelTargets(ctx, a, localA, &targetsA, &countA))
      return true;
    if (!countA)
      continue;

    for (AkChannel *other = b; other; other = other->next) {
      if (!ak_channelTargets(ctx, other, localB, &targetsB, &countB)) {
        if (targetsA != localA)
          free(targetsA);
        return true;
      }
      if (!countB)
        continue;

      conflict = false;
      for (i = 0; i < countA && !conflict; i++) {
        for (j = 0; j < countB; j++) {
          if (targetsA[i].target != targetsB[j].target)
            continue;
          if (!targetsA[i].isPartial
              || !targetsB[j].isPartial
              || targetsA[i].off == targetsB[j].off) {
            conflict = true;
            break;
          }
        }
      }
      if (targetsB != localB)
        free(targetsB);
      if (conflict) {
        if (targetsA != localA)
          free(targetsA);
        return true;
      }
    }
    if (targetsA != localA)
      free(targetsA);
  }

  return false;
}

static bool
ak_animationSamplerTimeRange(AkAnimSampler * __restrict sampler,
                             float         * __restrict outStart,
                             float         * __restrict outEnd) {
  AkInput *inp;

  if (!sampler) return false;

  for (inp = sampler->input; inp; inp = inp->next) {
    if (inp->semantic != AK_INPUT_INPUT)
      continue;
    if (ak_animAccessorFiniteFloatRange(inp->accessor, outStart, outEnd))
      return true;
  }

  return false;
}

AK_EXPORT
bool
ak_animationsConflict(AkContext   * __restrict ctx,
                      AkAnimation * __restrict a,
                      AkAnimation * __restrict b) {
  AkAnimationTreeIter aiter, biter;
  AkAnimation        *anode, *bnode;

  if (!a || !b || a == b) return false;

  if (!ak_animationTreeIterInit(&aiter, a))
    return true;
  while ((anode = ak_animationTreeIterNext(&aiter))) {
    if (!ak_animationTreeIterInit(&biter, b)) {
      ak_animationTreeIterDestroy(&aiter);
      return true;
    }
    while ((bnode = ak_animationTreeIterNext(&biter))) {
      if (ak_resolvedChannelsConflict(ctx, anode->channel, bnode->channel)) {
        ak_animationTreeIterDestroy(&biter);
        ak_animationTreeIterDestroy(&aiter);
        return true;
      }
    }
    if (biter.failed) {
      ak_animationTreeIterDestroy(&biter);
      ak_animationTreeIterDestroy(&aiter);
      return true;
    }
    ak_animationTreeIterDestroy(&biter);
  }

  if (aiter.failed) {
    ak_animationTreeIterDestroy(&aiter);
    return true;
  }
  ak_animationTreeIterDestroy(&aiter);
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
bool
ak_animationTimeRange(AkAnimation * __restrict anim,
                      float       * __restrict outStart,
                      float       * __restrict outEnd) {
  AkAnimationTreeIter iter;
  AkAnimation *animation;
  AkChannel   *ch;
  AkAnimSampler *sampler;
  float        start, end, minTime, maxTime;
  bool         found;

  if (!anim || !outStart || !outEnd)
    return false;

  found           = false;
  minTime         = 0.0f;
  maxTime         = 0.0f;

  if (!ak_animationTreeIterInit(&iter, anim))
    return false;
  while ((animation = ak_animationTreeIterNext(&iter))) {
    for (ch = animation->channel; ch; ch = ch->next) {
      sampler = ak_getObjectByUrl(&ch->source);
      if (!ak_animationSamplerTimeRange(sampler, &start, &end))
        continue;

      if (!found || start < minTime) minTime = start;
      if (!found || end   > maxTime) maxTime = end;
      found = true;
    }

  }

  if (iter.failed) {
    ak_animationTreeIterDestroy(&iter);
    return false;
  }
  ak_animationTreeIterDestroy(&iter);
  if (!found)
    return false;

  *outStart = minTime;
  *outEnd   = maxTime;
  return true;
}

AK_EXPORT
bool
ak_animationClipContainsAnimation(AkAnimationClip * __restrict clip,
                                  AkAnimation     * __restrict animation) {
  AkAnimationClipMember *member;
  AkAnimationTreeIter     iter;
  AkAnimation            *item;

  if (!clip || !animation)
    return false;

  for (member = clip->members; member; member = member->next) {
    if (!ak_animationTreeIterInit(&iter, member->animation))
      return true;
    while ((item = ak_animationTreeIterNext(&iter))) {
      if (item == animation) {
        ak_animationTreeIterDestroy(&iter);
        return true;
      }
    }
    if (iter.failed) {
      ak_animationTreeIterDestroy(&iter);
      return true;
    }
    ak_animationTreeIterDestroy(&iter);
  }
  return false;
}

AK_EXPORT
bool
ak_animationIsClipped(AkDoc       * __restrict doc,
                      AkAnimation * __restrict animation) {
  AkAnimationClip *clip;

  if (!doc || !animation)
    return false;
  for (clip = doc->animationClips.first; clip; clip = clip->next) {
    if (ak_animationClipContainsAnimation(clip, animation))
      return true;
  }
  return false;
}

AK_EXPORT
bool
ak_animationClipTimeRange(AkAnimationClip * __restrict clip,
                          float           * __restrict outStart,
                          float           * __restrict outEnd) {
  AkAnimationClipMember *member;
  float                  memberStart, memberEnd, derivedStart, derivedEnd;
  bool                   found;

  if (!clip || !outStart || !outEnd)
    return false;

  found        = false;
  derivedStart = 0.0f;
  derivedEnd   = 0.0f;
  for (member = clip->members; member; member = member->next) {
    if (!ak_animationTimeRange(member->animation, &memberStart, &memberEnd))
      continue;
    if (!found || memberStart < derivedStart)
      derivedStart = memberStart;
    if (!found || memberEnd > derivedEnd)
      derivedEnd = memberEnd;
    found = true;
  }

  if (!clip->hasStart && !found)
    return false;
  if (!clip->hasEnd && !found)
    return false;

  *outStart = clip->hasStart ? clip->start : derivedStart;
  *outEnd   = clip->hasEnd   ? clip->end   : derivedEnd;
  return isfinite(*outStart) && isfinite(*outEnd) && *outEnd >= *outStart;
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
