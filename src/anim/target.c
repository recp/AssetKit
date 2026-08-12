/*
 * Copyright (C) 2026 Recep Aslantas
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

typedef struct AkChannelTargetScan {
  AkResolvedTarget  source;
  AkResolvedTarget *targets;
  AkMorph          *morph;
  AkInstanceMorph **seen;
  size_t            capacity;
  size_t            count;
  size_t            seenCount;
  size_t            seenCapacity;
  bool              failed;
} AkChannelTargetScan;

static bool
ak_channelResolvedTargets_addSeen(AkChannelTargetScan * __restrict scan,
                                  AkInstanceMorph     * __restrict morpher) {
  AkInstanceMorph **grown;
  size_t            i, newCapacity;

  for (i = 0; i < scan->seenCount; i++) {
    if (scan->seen[i] == morpher)
      return false;
  }

  if (scan->seenCount == scan->seenCapacity) {
    newCapacity = scan->seenCapacity ? scan->seenCapacity * 2u : 8u;
    if (newCapacity < scan->seenCapacity
        || newCapacity > SIZE_MAX / sizeof(*scan->seen)) {
      scan->failed = true;
      return false;
    }
    grown = realloc(scan->seen, newCapacity * sizeof(*scan->seen));
    if (!grown) {
      scan->failed = true;
      return false;
    }
    scan->seen         = grown;
    scan->seenCapacity = newCapacity;
  }

  scan->seen[scan->seenCount++] = morpher;
  return true;
}

static void
ak_channelResolvedTargets_scanNodes(AkChannelTargetScan * __restrict scan,
                                    AkNode              * __restrict node) {
  AkInstanceGeometry *inst;

  for (; node; node = node->next) {
    if (scan->failed)
      return;

    for (inst = node->geometry;
         inst;
         inst = (AkInstanceGeometry *)inst->base.next) {
      AkInstanceMorph *morpher;
      AkResolvedTarget target;

      morpher = inst->morpher;
      if (!morpher
          || morpher->morph != scan->morph)
        continue;
      if (!ak_channelResolvedTargets_addSeen(scan, morpher)) {
        if (scan->failed)
          return;
        continue;
      }
      if (scan->count == SIZE_MAX) {
        scan->failed = true;
        return;
      }

      target          = scan->source;
      target.target   = morpher;
      if (scan->targets && scan->count < scan->capacity)
        scan->targets[scan->count] = target;
      scan->count++;
    }

    if (node->chld)
      ak_channelResolvedTargets_scanNodes(scan, node->chld);
  }
}

AK_EXPORT
size_t
ak_channelResolvedTargets(AkContext        * __restrict ctx,
                          AkChannel        * __restrict ch,
                          AkResolvedTarget * __restrict targets,
                          size_t                         capacity) {
  AkChannelTargetScan scan;
  AkResolvedTarget    resolved;
  AkDoc              *doc;
  AkNode             *root;
  AkScene            *scene;

  if (!ch)
    return 0;

  resolved = ak_channelTarget(ctx, ch);
  if (!resolved.target)
    return 0;

  if (targets && capacity)
    targets[0] = resolved;

  doc = ctx ? ctx->doc : NULL;
  if (!doc
      || !doc->inf
      || doc->inf->ftype != AK_FILE_TYPE_COLLADA
      || ch->targetType != AK_TARGET_WEIGHTS
      || !ch->target
      || ak_typeid(resolved.target) != AKT_MORPH_INST)
    return 1;

  memset(&scan, 0, sizeof(scan));
  scan.source   = resolved;
  scan.targets  = targets;
  scan.capacity = capacity;
  scan.count    = 1;
  scan.morph    = ((AkInstanceMorph *)resolved.target)->morph;
  if (!scan.morph)
    return 1;
  if (!ak_channelResolvedTargets_addSeen(&scan, resolved.target)) {
    free(scan.seen);
    return SIZE_MAX;
  }

  /* Imported DAE roots, including <library_nodes>, are registered once in
   * doc->lib.nodes. Walking child edges covers every authored instance while
   * deliberately avoiding <instance_node> reference edges and their cycles. */
  for (root = doc->lib.nodes.first; root; root = root->docNext)
    ak_channelResolvedTargets_scanNodes(&scan, root);

  /* Programmatically-built COLLADA documents may omit the node library.
   * Fall back to visual-scene roots in that case. */
  if (!doc->lib.nodes.first) {
    for (scene = doc->lib.scenes.first; scene; scene = scene->next) {
      if (scene->node)
        ak_channelResolvedTargets_scanNodes(&scan, scene->node->chld);
    }
  }

  free(scan.seen);
  if (scan.failed)
    return SIZE_MAX;
  return scan.count;
}
