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

#include "channel.h"
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------------
 * Channel target parsing.
 *
 * Accepted forms (DAE COLLADA 1.4 §5.3.2 + real-world exporter quirks):
 *   "id(N)"                       — bare source id + index
 *   "prefix/.../id(N)"            — SID-style path, last segment is "id(N)"
 *
 * `outIdLen`/`outId` point into the input buffer (no allocation); caller
 * uses ak_heap_strndup or ak_getObjectById_n if it needs to keep it.
 *--------------------------------------------------------------------------*/

static
bool
dae_parseChannelTargetIndexed(const char  *target,
                              const char **outIdStart,
                              size_t      *outIdLen,
                              uint32_t    *outIdx) {
  const char *open, *close, *idStart, *p;
  long        idx;
  char       *end;

  if (!target) return false;

  /* Trailing "(N)" must be present and well-formed. */
  if (!(open = strrchr(target, '('))
      || !(close = strchr(open + 1, ')'))
      || close == open + 1)
    return false;

  idx = strtol(open + 1, &end, 10);
  if (end != close || idx < 0)
    return false;

  /* Id portion: everything from the last '/' (or start) up to '('. */
  if ((idStart = strrchr(target, '/')) && idStart < open)
    idStart++;
  else
    idStart = target;

  if (idStart >= open) return false; /* "(N)" with empty id */

  /* Matrix element targets use forms like "matrix(0)(0)"; those are not
     morph weight arrays and must stay on the normal SID resolver path. */
  for (p = idStart; p < open; p++) {
    if (*p == '(' || *p == ')') return false;
  }

  *outIdStart = idStart;
  *outIdLen   = (size_t)(open - idStart);
  *outIdx     = (uint32_t)idx;
  return true;
}

/*----------------------------------------------------------------------------
 * Topology lookups.
 *--------------------------------------------------------------------------*/

/* Walk visual scene tree until we find an AkInstanceMorph that wraps the
   given AkMorph. Returns first match (multi-instance disambiguation is a
   spec gray area; first-found is what most authoring tools imply). */
static
AkInstanceMorph *
dae_findInstanceMorph_node(AkNode * __restrict node, AkMorph *morph) {
  AkInstanceGeometry *instGeom;
  AkInstanceMorph    *found;
  AkNode             *child;

  for (; node; node = (AkNode *)node->next) {
    for (instGeom = node->geometry; instGeom;
         instGeom = (AkInstanceGeometry *)instGeom->base.next) {
      if (instGeom->morpher && instGeom->morpher->morph == morph)
        return instGeom->morpher;
    }
    if ((child = node->chld)
        && (found = dae_findInstanceMorph_node(child, morph)))
      return found;
  }
  return NULL;
}

static
AkInstanceMorph *
dae_findInstanceMorph(AkDoc * __restrict doc, AkMorph *morph) {
  AkVisualScene   *vscn;
  AkInstanceMorph *found;

  if (!morph || !doc->lib.visualScenes) return NULL;

  for (vscn = (void *)doc->lib.visualScenes->chld;
       vscn;
       vscn = (void *)vscn->base.next) {
    if ((found = dae_findInstanceMorph_node(vscn->node, morph)))
      return found;
  }
  return NULL;
}

/* Find the morph controller whose morphdae->source chain contains src.
   Linear scan over doc->lib.controllers — fine for typical asset sizes;
   if multi-controller perf becomes a real workload we'd build a
   source→controller map up front. */
static
AkController *
dae_findControllerForSource(AkDoc * __restrict doc, AkSource *src) {
  AkController *ctlr;
  AkMorph      *morph;
  AkMorphDAE   *morphdae;
  AkSource     *s;

  if (!src || !doc->lib.controllers) return NULL;

  for (ctlr = (AkController *)doc->lib.controllers->chld;
       ctlr;
       ctlr = (AkController *)ctlr->base.next) {
    if (ctlr->type != AK_CONTROLLER_MORPH)              continue;
    if (!(morph = ctlr->data))                          continue;
    if (!(morphdae = ak_userData(morph)))               continue;

    for (s = morphdae->source; s; s = s->next) {
      if (s == src) return ctlr;
    }
  }
  return NULL;
}

static
AkInstanceMorph *
dae_resolveMorpher(AkDoc      * __restrict doc,
                   const char *idStart,
                   size_t      idLen) {
  void           *element;
  AkController   *ctlr;
  char            idbuf[256];

  /* Bounded inline copy to NUL-terminate the id slice. Source ids in DAE
     are typically short (<64 chars); the buffer is generous. Rather than
     allocate, we just bail if it's longer than expected. */
  if (idLen == 0 || idLen >= sizeof(idbuf)) return NULL;
  memcpy(idbuf, idStart, idLen);
  idbuf[idLen] = '\0';

  /* Doc-wide id lookup (hash table — O(1)). The "id" segment of a DAE
     channel target is typically the SID inside a controller scope; many
     real-world exporters set the source's id to the same string as the
     SID, so the doc id table catches the common case. */
  if (!(element = ak_getObjectById(doc, idbuf))) return NULL;
  if (ak_typeid(element) != AKT_SOURCE)          return NULL;

  if (!(ctlr = dae_findControllerForSource(doc, (AkSource *)element)))
    return NULL;

  return dae_findInstanceMorph(doc, (AkMorph *)ctlr->data);
}

static
void
dae_fixup_channel_walk(DAEState    * __restrict dst,
                       AkAnimation * __restrict anim) {
  AkAnimation      *sub;
  AkChannel        *ch;
  AkResolvedTarget *rt;
  AkInstanceMorph  *morpher;
  const char       *idStart;
  size_t            idLen;
  uint32_t          idx;

  for (; anim; anim = (AkAnimation *)anim->base.next) {
    for (ch = anim->channel; ch; ch = ch->next) {
      /* Skip already-resolved channels (defensive). */
      if (ch->resolvedTarget) continue;

      /* Only the indexed-array form is handled here. SID-with-attribute
         channels ("node/translate.X") still go through ak_channelTarget's
         SID fallback. */
      if (!dae_parseChannelTargetIndexed(ch->target, &idStart, &idLen, &idx))
        continue;

      /* Today the only DAE consumer of "(N)" we recognize is morph weights. */
      morpher = dae_resolveMorpher(dst->doc, idStart, idLen);
      if (!morpher) continue;

      rt                 = ak_heap_calloc(dst->heap, ch, sizeof(*rt));
      rt->target         = morpher;
      rt->off            = idx;
      rt->isPartial      = true;
      ch->resolvedTarget = rt;
      ch->targetType     = AK_TARGET_WEIGHTS;
    }

    if ((sub = anim->animation))
      dae_fixup_channel_walk(dst, sub);
  }
}

AK_HIDE
void
dae_fixup_channel(DAEState * __restrict dst) {
  AkAnimation *anim;

  if (!dst->doc->lib.animations) return;

  anim = (AkAnimation *)dst->doc->lib.animations->chld;
  dae_fixup_channel_walk(dst, anim);
}
