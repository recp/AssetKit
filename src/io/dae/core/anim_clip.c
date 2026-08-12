/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "anim_clip.h"

#include <math.h>
#include <limits.h>
#include <stdlib.h>

typedef enum DAEAnimationSearchResult {
  DAE_ANIMATION_SEARCH_NOT_FOUND = 0,
  DAE_ANIMATION_SEARCH_FOUND,
  DAE_ANIMATION_SEARCH_FAILED
} DAEAnimationSearchResult;

static bool
dae_animationPtrPush(AkAnimation ***items,
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

static DAEAnimationSearchResult
dae_animationTreeContains(AkAnimation *root, AkAnimation *wanted) {
  AkAnimation **pending, **seen;
  AkAnimation  *animation, *item;
  size_t        pendingCount, pendingCapacity, seenCount, seenCapacity, i;
  DAEAnimationSearchResult result;

  if (!root || !wanted)
    return DAE_ANIMATION_SEARCH_NOT_FOUND;

  pending = NULL;
  seen = NULL;
  pendingCount = pendingCapacity = seenCount = seenCapacity = 0;
  result = DAE_ANIMATION_SEARCH_NOT_FOUND;

  for (item = root; item; item = item->next) {
    if (!dae_animationPtrPush(&pending, &pendingCount, &pendingCapacity, item)) {
      result = DAE_ANIMATION_SEARCH_FAILED;
      goto done;
    }
  }

  while (pendingCount > 0) {
    animation = pending[--pendingCount];
    for (i = 0; i < seenCount; i++) {
      if (seen[i] == animation)
        break;
    }
    if (i < seenCount)
      continue;
    if (!dae_animationPtrPush(&seen, &seenCount, &seenCapacity, animation)) {
      result = DAE_ANIMATION_SEARCH_FAILED;
      goto done;
    }
    if (animation == wanted) {
      result = DAE_ANIMATION_SEARCH_FOUND;
      goto done;
    }
    for (item = animation->animation; item; item = item->next) {
      if (!dae_animationPtrPush(&pending,
                                &pendingCount,
                                &pendingCapacity,
                                item)) {
        result = DAE_ANIMATION_SEARCH_FAILED;
        goto done;
      }
    }
  }

done:
  free(pending);
  free(seen);
  return result;
}

static bool
dae_docContainsAnimation(AkDoc *doc, AkAnimation *animation) {
  return doc && animation
         && dae_animationTreeContains(doc->lib.animations.first, animation)
              == DAE_ANIMATION_SEARCH_FOUND;
}

static bool
dae_animationClipAttributes(xml_t *xml,
                            float *start,
                            float *end,
                            bool  *hasStart,
                            bool  *hasEnd) {
  xml_attr_t *attribute;

  *hasStart = false;
  *hasEnd   = false;
  *start    = 0.0f;
  *end      = 0.0f;

  if ((attribute = DAE_XMLA(xml, start))) {
    *start    = xmla_float(attribute, NAN);
    *hasStart = true;
    if (!isfinite(*start))
      return false;
  }

  if ((attribute = DAE_XMLA8(xml, end))) {
    *end    = xmla_float(attribute, NAN);
    *hasEnd = true;
    if (!isfinite(*end))
      return false;
  }

  return !(*hasStart && *hasEnd && *end < *start);
}

static AkAnimationClip*
dae_animationClip(DAEState * __restrict dst,
                  xml_t    * __restrict xml) {
  AkAnimationClip       *clip;
  AkAnimationClipMember *member;
  AkHeap                *heap;
  xml_t                 *child;
  xml_attr_t            *url;
  uint32_t               memberCount;
  float                  start, end;
  bool                   hasStart, hasEnd;

  /* Validate the complete member list before publishing or queueing a single
   * URL.  A malformed late member therefore cannot expose a partial clip. */
  if (!dae_animationClipAttributes(xml, &start, &end, &hasStart, &hasEnd))
    return NULL;

  memberCount = 0;
  for (child = xml->val; child; child = child->next) {
    if (!DAE_XML_TAG_EQ(child, instance_animation))
      continue;
    url = DAE_XMLA4(child, url);
    if (!url || !url->val || url->valsize == 0 || memberCount == UINT32_MAX)
      return NULL;
    memberCount++;
  }

  if (memberCount == 0)
    return NULL;

  heap           = dst->heap;
  clip           = ak_heap_calloc(heap, dst->doc, sizeof(*clip));
  clip->name     = DAE_XMLA_STRDUP8(xml, heap, name, clip);
  clip->start    = start;
  clip->end      = end;
  clip->hasStart = hasStart;
  clip->hasEnd   = hasEnd;
  xmla_setid(xml, heap, clip);

  for (child = xml->val; child; child = child->next) {
    if (DAE_XML_TAG_EQ(child, instance_animation)) {
      member = ak_heap_calloc(heap, clip, sizeof(*member));
      DAE_URL_SET(dst, child, url, member, &member->source);

      if (clip->lastMember)
        clip->lastMember->next = member;
      else
        clip->members = member;
      clip->lastMember = member;
      clip->memberCount++;
    } else if (DAE_XML_TAG_EQ8(child, extra)) {
      clip->extra = tree_fromxml(heap, clip, child);
    }
  }

  return clip;
}

AK_HIDE
void
dae_animationClips(DAEState * __restrict dst,
                   xml_t    * __restrict xml) {
  AkAnimationClip *clip;
  AkAnimationClipLib *library;

  if (!dst || !dst->doc)
    return;
  library = &dst->doc->animationClips;

  for (xml = xml->val; xml; xml = xml->next) {
    if (!DAE_XML_TAG_EQ(xml, animation_clip)
        || !(clip = dae_animationClip(dst, xml)))
      continue;

    if (library->last)
      library->last->next = clip;
    else
      library->first = clip;
    library->last = clip;
    library->count++;
  }
}

AK_HIDE
void
dae_fixupAnimationClips(DAEState * __restrict dst) {
  AkAnimationClip       *clip, *next, *previous;
  AkAnimationClipMember *member;
  AkAnimation           *animation;
  AkDoc                 *sourceDoc;
  bool                   valid;

  if (!dst || !dst->doc)
    return;

  previous = NULL;
  clip     = dst->doc->animationClips.first;
  while (clip) {
    next  = clip->next;
    valid = clip->memberCount > 0;

    /* First pass only validates.  Do not publish half-resolved membership if
     * a later URL is missing or resolves to a non-animation object. */
    for (member = clip->members; valid && member; member = member->next) {
      animation = ak_getObjectByUrl(&member->source);
      sourceDoc = member->source.doc ? member->source.doc : dst->doc;
      if (!dae_docContainsAnimation(sourceDoc, animation))
        valid = false;
    }

    if (valid) {
      for (member = clip->members; member; member = member->next)
        member->animation = ak_getObjectByUrl(&member->source);
      previous = clip;
      clip     = next;
      continue;
    }

    if (previous)
      previous->next = next;
    else
      dst->doc->animationClips.first = next;
    if (dst->doc->animationClips.last == clip)
      dst->doc->animationClips.last = previous;
    if (dst->doc->animationClips.count > 0)
      dst->doc->animationClips.count--;
    ak_free(clip);
    clip = next;
  }
}
