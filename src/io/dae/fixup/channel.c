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
#include <string.h>

typedef struct DAEMatrixAnimFix {
  struct DAEMatrixAnimFix *next;
  AkAccessor              *acc;
} DAEMatrixAnimFix;

static
bool
dae_parse_u32_between(const char * __restrict begin,
                      const char * __restrict end,
                      uint32_t   * __restrict out) {
  uint32_t value;

  if (!begin || !end || begin >= end)
    return false;

  value = 0;
  do {
    char c = *begin++;
    if (c < '0' || c > '9')
      return false;
    value = value * 10u + (uint32_t)(c - '0');
  } while (begin < end);

  *out = value;

  return true;
}

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
  uint32_t    idx;

  if (!target) return false;

  /* Trailing "(N)" must be present and well-formed. */
  if (!(open = strrchr(target, '('))
      || !(close = strchr(open + 1, ')'))
      || close == open + 1)
    return false;

  if (!dae_parse_u32_between(open + 1, close, &idx))
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
  *outIdx     = idx;
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
  AkNodeRef          *nodeRef;

  for (; node; node = (AkNode *)node->next) {
    for (instGeom = node->geometry; instGeom;
         instGeom = (AkInstanceGeometry *)instGeom->base.next) {
      if (instGeom->morpher && instGeom->morpher->morph == morph)
        return instGeom->morpher;
    }
    if ((child = node->chld)
        && (found = dae_findInstanceMorph_node(child, morph)))
      return found;

    for (nodeRef = node->nodeRefs; nodeRef; nodeRef = nodeRef->next) {
      child = ak_nodeRefTarget(nodeRef);
      if (child && (found = dae_findInstanceMorph_node(child, morph)))
        return found;
    }
  }
  return NULL;
}

static
AkInstanceMorph *
dae_findInstanceMorph(AkDoc * __restrict doc, AkMorph *morph) {
  AkScene         *vscn;
  AkInstanceMorph *found;

  if (!morph || !doc->lib.scenes.first) return NULL;

  for (vscn = doc->lib.scenes.first;
       vscn;
       vscn = vscn->next) {
    if ((found = dae_findInstanceMorph_node(vscn->node, morph)))
      return found;
  }
  return NULL;
}

/* Find the morph controller whose morphdae->source chain contains src.
   Linear scan over DAE-private controllers — fine for typical asset sizes;
   if multi-controller perf becomes a real workload we'd build a
   source→controller map up front. */
static
AkController *
dae_findControllerForSource(DAEState * __restrict dst, DaeSource *src) {
  AkController *ctlr;
  AkMorph      *morph;
  AkMorphDAE   *morphdae;
  DaeSource    *s;

  if (!src) return NULL;

  for (ctlr = dst->controllers;
       ctlr;
       ctlr = ctlr->next) {
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
dae_resolveMorpher(DAEState   * __restrict dst,
                   const char *idStart,
                   size_t      idLen) {
  AkDoc          *doc;
  void           *element;
  AkController   *ctlr;
  char            idbuf[256];

  /* Bounded inline copy to NUL-terminate the id slice. Source ids in DAE
     are typically short (<64 chars); the buffer is generous. Rather than
     allocate, we just bail if it's longer than expected. */
  doc = dst->doc;
  if (idLen == 0 || idLen >= sizeof(idbuf)) return NULL;
  memcpy(idbuf, idStart, idLen);
  idbuf[idLen] = '\0';

  /* Doc-wide id lookup (hash table — O(1)). The "id" segment of a DAE
     channel target is typically the SID inside a controller scope; many
     real-world exporters set the source's id to the same string as the
     SID, so the doc id table catches the common case. */
  if (!(element = ak_getObjectById(doc, idbuf))) return NULL;
  if (ak_typeid(element) != DAE_TYPE_SOURCE)     return NULL;

  if (!(ctlr = dae_findControllerForSource(dst, (DaeSource *)element)))
    return NULL;

  return dae_findInstanceMorph(doc, (AkMorph *)ctlr->data);
}

static
bool
dae_resolveMatrixElement(AkContext        * __restrict ctx,
                         const char       * __restrict target,
                         AkResolvedTarget * __restrict rt) {
  const char *seg, *open1, *close1, *open2, *close2, *attr;
  AkObject   *obj;
  uint32_t    a, b;
  char        base[512];
  size_t      n;
  bool        hasB;

  if (!ctx || !target || !rt)
    return false;

  if ((seg = strrchr(target, '/')))
    seg++;
  else
    seg = target;

  if (!(open1 = strchr(seg, '('))
      || !(close1 = strchr(open1 + 1, ')'))
      || close1 == open1 + 1)
    return false;

  if (!dae_parse_u32_between(open1 + 1, close1, &a))
    return false;

  open2 = close2 = NULL;
  b     = 0;
  hasB  = false;
  if (*(close1 + 1) == '(') {
    open2 = close1 + 1;
    if (!(close2 = strchr(open2 + 1, ')')) || close2 == open2 + 1)
      return false;

    if (!dae_parse_u32_between(open2 + 1, close2, &b) || close2[1] != '\0')
      return false;
    hasB = true;
  } else if (close1[1] != '\0') {
    return false;
  }

  n = (size_t)(open1 - target);
  if (n == 0 || n >= sizeof(base))
    return false;

  memcpy(base, target, n);
  base[n] = '\0';

  attr = NULL;
  obj  = ak_sid_resolve(ctx, base, &attr);
  if (!obj || attr || ak_typeid(obj) != AKT_OBJECT
      || (AkTypeId)obj->type != AKT_MATRIX)
    return false;

  if (hasB) {
    if (a >= 4 || b >= 4)
      return false;
    rt->off = (uint32_t)(b * 4 + a);
  } else {
    if (a >= 16)
      return false;
    rt->off = (uint32_t)((a % 4) * 4 + (a / 4));
  }

  rt->target    = obj;
  rt->isPartial = true;
  return true;
}

static
AkInput *
dae_animSamplerInput(AkAnimSampler    * __restrict samp,
                     AkInputSemantic               sem) {
  AkInput *inp;

  if (!samp) return NULL;

  switch (sem) {
    case AK_INPUT_INPUT:         if (samp->inputInput)      return samp->inputInput;      break;
    case AK_INPUT_OUTPUT:        if (samp->outputInput)     return samp->outputInput;     break;
    case AK_INPUT_IN_TANGENT:    if (samp->inTangentInput)  return samp->inTangentInput;  break;
    case AK_INPUT_OUT_TANGENT:   if (samp->outTangentInput) return samp->outTangentInput; break;
    case AK_INPUT_INTERPOLATION: if (samp->interpInput)     return samp->interpInput;     break;
    default: break;
  }

  for (inp = samp->input; inp; inp = inp->next) {
    if (inp->semantic == sem)
      return inp;
  }

  return NULL;
}

static
bool
dae_matrixAnimFixed(DAEMatrixAnimFix * __restrict it,
                    AkAccessor       * __restrict acc) {
  for (; it; it = it->next) {
    if (it->acc == acc)
      return true;
  }

  return false;
}

static
void
dae_matrixAnimMark(DAEState          * __restrict dst,
                   DAEMatrixAnimFix ** __restrict done,
                   AkAccessor        * __restrict acc) {
  DAEMatrixAnimFix *it;

  it      = ak_heap_calloc(dst->heap, dst->doc, sizeof(*it));
  it->acc = acc;
  it->next = *done;
  *done   = it;
}

static
bool
dae_transposeMat4Output(AkAccessor * __restrict acc) {
  char     *base;
  float    *m;
  size_t    st;
  uint32_t  i;

  if (!acc
      || !acc->buffer
      || !acc->buffer->data
      || acc->componentType != AKT_FLOAT
      || acc->componentCount != 16)
    return false;

  st = acc->byteStride;
  if (st == 0)
    st = sizeof(float) * 16;

  base = (char *)acc->buffer->data + acc->byteOffset;
  for (i = 0; i < acc->count; i++) {
    m = (float *)(base + st * i);
    glm_mat4_transpose((vec4 *)m);
  }

  return true;
}

static
void
dae_fixupMatrixAccessor(DAEState          * __restrict dst,
                        AkInput           * __restrict inp,
                        DAEMatrixAnimFix ** __restrict done) {
  AkAccessor *acc;

  if (!inp || !(acc = inp->accessor))
    return;

  if (dae_matrixAnimFixed(*done, acc))
    return;

  if (dae_transposeMat4Output(acc))
    dae_matrixAnimMark(dst, done, acc);
}

static
void
dae_fixupMatrixChannel(DAEState          * __restrict dst,
                       AkContext         * __restrict ctx,
                       AkChannel         * __restrict ch,
                       DAEMatrixAnimFix ** __restrict done) {
  AkResolvedTarget rt;
  AkAnimSampler   *samp;
  AkObject        *obj;

  rt = ak_channelTarget(ctx, ch);
  if (!rt.target || ak_typeid(rt.target) != AKT_OBJECT)
    return;

  obj = rt.target;
  if ((AkTypeId)obj->type != AKT_MATRIX)
    return;

  samp = ak_getObjectByUrl(&ch->source);

  /* DAE <matrix> values are authored row-major. Static node matrices are
     normalized to cglm/AssetKit column-major during node parse; animation
     OUTPUT matrices need the same one-time normalization. Tangents are not
     evaluated today, but if an exporter authors 16-float matrix tangents,
     keep them in the same convention for future Bezier/Hermite support. */
  dae_fixupMatrixAccessor(dst,
                          dae_animSamplerInput(samp, AK_INPUT_OUTPUT),
                          done);
  dae_fixupMatrixAccessor(dst,
                          dae_animSamplerInput(samp, AK_INPUT_IN_TANGENT),
                          done);
  dae_fixupMatrixAccessor(dst,
                          dae_animSamplerInput(samp, AK_INPUT_OUT_TANGENT),
                          done);
}

static
void
dae_fixup_channel_walk(DAEState          * __restrict dst,
                       AkAnimation       * __restrict anim,
                       AkContext         * __restrict ctx,
                       DAEMatrixAnimFix ** __restrict done) {
  AkAnimation      *sub;
  AkChannel        *ch;
  AkResolvedTarget *rt;
  AkResolvedTarget  mrt;
  AkInstanceMorph  *morpher;
  const char       *idStart;
  size_t            idLen;
  uint32_t          idx;

  for (; anim; anim = anim->next) {
    for (ch = anim->channel; ch; ch = ch->next) {
      if (!ch->resolvedTarget) {
        memset(&mrt, 0, sizeof(mrt));
        if (dae_resolveMatrixElement(ctx, ch->target, &mrt)) {
          rt                 = ak_heap_calloc(dst->heap, ch, sizeof(*rt));
          *rt                = mrt;
          ch->resolvedTarget = rt;
          ch->targetType     = AK_TARGET_FLOAT;
        } else if (dae_parseChannelTargetIndexed(ch->target,
                                                 &idStart,
                                                 &idLen,
                                                 &idx)
                   && (morpher = dae_resolveMorpher(dst,
                                                    idStart,
                                                    idLen))) {
          rt                 = ak_heap_calloc(dst->heap, ch, sizeof(*rt));
          rt->target         = morpher;
          rt->off            = idx;
          rt->isPartial      = true;
          ch->resolvedTarget = rt;
          ch->targetType     = AK_TARGET_WEIGHTS;
        }
      }

      dae_fixupMatrixChannel(dst, ctx, ch, done);
    }

    if ((sub = anim->animation))
      dae_fixup_channel_walk(dst, sub, ctx, done);
  }
}

AK_HIDE
void
dae_fixup_channel(DAEState * __restrict dst) {
  AkAnimation      *anim;
  DAEMatrixAnimFix *done;
  AkContext         ctx;

  if (!dst->doc->lib.animations.first) return;

  anim    = dst->doc->lib.animations.first;
  done    = NULL;
  memset(&ctx, 0, sizeof(ctx));
  ctx.doc = dst->doc;

  dae_fixup_channel_walk(dst, anim, &ctx, &done);
}
