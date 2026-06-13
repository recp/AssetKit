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
 Resources:
   https://all3dp.com/1/obj-file-format-3d-printing-cad/
   http://paulbourke.net/dataformats/obj/
   http://paulbourke.net/dataformats/mtl/
   https://en.wikipedia.org/wiki/Wavefront_.obj_file
*/

#include "obj.h"
#include "common.h"
#include "group.h"
#include "mtl.h"
#include "util.h"
#include "../common/postscript.h"
#include "../../id.h"
#include "../../data.h"
#include "../../string_fast.h"
#include "../../../include/ak/path.h"

static
void
wobj_prepareMissingDefaults(WOState * __restrict wst);

AK_INLINE
char*
wobj_skip_inline_space(char * __restrict p);

#define wobj_data_append_slot  ak_data_append_slot
#define wobj_data_append_slots ak_data_append_slots

#define WOBJ_KW_MTLL AK_STR_PACK4_CHARS('m', 't', 'l', 'l')
#define WOBJ_KW_USEM AK_STR_PACK4_CHARS('u', 's', 'e', 'm')

#define WOBJ_FACE_FAST_UNKNOWN 0u
#define WOBJ_FACE_FAST_TRI     1u
#define WOBJ_FACE_FAST_TRI_VN  2u
#define WOBJ_FACE_FAST_DISABLED 255u
#define WOBJ_FACE_FAST_MISS_LIMIT 8u

#define WOBJ_TOKEN_SEP(CH)                                                    \
  ((CH) == ' ' || (CH) == '\t' || (CH) == '\f' || (CH) == '\v'                \
   || (CH) == '\n' || (CH) == '\r')

AK_INLINE
char*
wobj_parse_face_index(char  * __restrict p,
                      AkInt * __restrict dest) {
  AkUInt value;

  if (!ak_str_isdigit_fast(*p))
    return ak_strtoi_one_fast(p, dest);

  value = 0;
  do {
    value = value * 10u + (AkUInt)(*p++ - '0');
  } while (ak_str_isdigit_fast(*p));

  *dest = (AkInt)value;

  return p;
}

static
void
wobj_prepareMissingDefaults(WOState * __restrict wst) {
  WOObject *obj;
  WOPrim   *prim;
  uint32_t  texDefault, norDefault;
  size_t    texCount, norCount;
  bool      needTexDefault, needNorDefault;

  needTexDefault = false;
  needNorDefault = false;
  texCount        = wst->dc_tex->itemcount;
  norCount        = wst->dc_nor->itemcount;

  for (obj = wst->obj; obj; obj = obj->next) {
    for (prim = obj->prim; prim; prim = prim->next) {
      needTexDefault |= prim->hasTexture
                        && (prim->missingTexture
                            || texCount == 0);
      needNorDefault |= prim->hasNormal
                        && (prim->missingNormal
                            || norCount == 0);
    }
  }

  texDefault = 0;
  norDefault = 0;

  if (needTexDefault) {
    texDefault = (uint32_t)wst->dc_tex->itemcount;
    if (wst->texCompSize == AK_COMPONENT_SIZE_VEC3) {
      vec3 zero = {0.0f, 0.0f, 0.0f};
      memcpy(wobj_data_append_slot(wst->dc_tex), zero, sizeof(zero));
    } else {
      vec2 zero = {0.0f, 0.0f};
      memcpy(wobj_data_append_slot(wst->dc_tex), zero, sizeof(zero));
    }
  }

  if (needNorDefault) {
    vec3 zero = {0.0f, 0.0f, 0.0f};

    norDefault = (uint32_t)wst->dc_nor->itemcount;
    memcpy(wobj_data_append_slot(wst->dc_nor), zero, sizeof(zero));
  }

  for (obj = wst->obj; obj; obj = obj->next) {
    for (prim = obj->prim; prim; prim = prim->next) {
      if (prim->hasTexture
          && (prim->missingTexture || texCount == 0)) {
        prim->defaultTexIndex   = texDefault;
        prim->useDefaultTexture = true;
      }

      if (prim->hasNormal
          && (prim->missingNormal || norCount == 0)) {
        prim->defaultNorIndex  = norDefault;
        prim->useDefaultNormal = true;
      }
    }
  }
}

typedef struct WODataMark {
  AkDataChunk *last;
  size_t       size;
  size_t       usedsize;
  size_t       itemcount;
  size_t       lastUsedsize;
} WODataMark;

AK_INLINE
WODataMark
wobj_data_mark(AkDataContext * __restrict dctx) {
  WODataMark mark;

  mark.last         = dctx->last;
  mark.size         = dctx->size;
  mark.usedsize     = dctx->usedsize;
  mark.itemcount    = dctx->itemcount;
  mark.lastUsedsize = dctx->last ? dctx->last->usedsize : 0;

  return mark;
}

AK_INLINE
void
wobj_data_rollback(AkDataContext * __restrict dctx,
                   WODataMark    * __restrict mark) {
  dctx->last      = mark->last;
  dctx->size      = mark->size;
  dctx->usedsize  = mark->usedsize;
  dctx->itemcount = mark->itemcount;

  if (mark->last) {
    mark->last->usedsize = mark->lastUsedsize;
    mark->last->next     = NULL;
  } else {
    dctx->data = NULL;
  }
}

AK_INLINE
bool
wobj_index_valid(AkInt idx, size_t count) {
  if (idx > 0)
    return (AkUInt)idx <= (AkUInt)count;
  if (idx < 0)
    return (AkInt)count + idx >= 0;
  return false;
}

AK_INLINE
AkUInt
wobj_index_real_unchecked(AkInt idx, size_t count) {
  return idx > 0 ? (AkUInt)idx - 1u : (AkUInt)((AkInt)count + idx);
}

AK_INLINE
bool
wobj_seen_index(AkUInt * __restrict seen, uint32_t count, AkUInt value) {
  uint32_t i;

  for (i = 0; i < count; i++) {
    if (seen[i] == value)
      return true;
  }

  return false;
}

#define WOBJ_FACE_STACK_LIMIT 128

static
bool
wobj_compact_duplicate_face(AkDataContext * __restrict dctx,
                            WODataMark    * __restrict mark,
                            size_t                      posCount,
                            uint32_t      * __restrict  vc) {
  AkDataChunk *chunk;
  AkInt       *face;
  AkUInt       seen[WOBJ_FACE_STACK_LIMIT];
  uint32_t     keep[WOBJ_FACE_STACK_LIMIT];
  uint32_t     seenCount, keepCount, i;

  if (*vc > WOBJ_FACE_STACK_LIMIT)
    return false;

  chunk = mark->last ? mark->last : dctx->data;
  if (!chunk || dctx->last != chunk)
    return false;

  face = (AkInt *)(void *)(chunk->data
                           + (mark->last ? mark->lastUsedsize : 0));
  seenCount = 0;
  keepCount = 0;
  for (i = 0; i < *vc; i++) {
    AkUInt realIndex;

    if (!wobj_index_valid(face[i * 3], posCount))
      return false;

    realIndex = wobj_index_real_unchecked(face[i * 3], posCount);
    if (wobj_seen_index(seen, seenCount, realIndex))
      continue;

    seen[seenCount++] = realIndex;
    keep[keepCount++] = i;
  }

  if (keepCount < 3)
    return false;

  wobj_data_rollback(dctx, mark);
  for (i = 0; i < keepCount; i++) {
    AkInt *slot;

    slot = wobj_data_append_slot(dctx);
    memcpy(slot, &face[keep[i] * 3], sizeof(ivec3));
  }

  *vc = keepCount;

  return true;
}

AK_INLINE
bool
wobj_is_freeform_keyword(const char * __restrict p) {
  uint32_t key;

  key = ak_str_pack4_fast(p, 4);
  switch (key) {
    case AK_STR_PACK4_CHARS('c', 'u', 'r', 'v'):
    case AK_STR_PACK4_CHARS('s', 'u', 'r', 'f'):
    case AK_STR_PACK4_CHARS('p', 'a', 'r', 'm'):
    case AK_STR_PACK4_CHARS('t', 'r', 'i', 'm'):
    case AK_STR_PACK4_CHARS('h', 'o', 'l', 'e'):
    case AK_STR_PACK4_CHARS('s', 'p', 'e', 'c'):
    case AK_STR_PACK4_CHARS('d', 'e', 'g', ' '):
      return true;
    default:
      break;
  }

  return ak_str_pack8_fast(p, 6)
         == AK_STR_PACK8_CHARS('c', 's', 't', 'y', 'p', 'e', 0, 0);
}

static
uint32_t
wobj_parse_float_line(char     * __restrict p,
                      float    * __restrict values,
                      uint32_t               cap,
                      uint32_t               min_count,
                      bool     * __restrict ok,
                      char    ** __restrict endp) {
  float    ignored;
  uint32_t count;

  count = 0;
  if (ok)
    *ok = false;

  while (p && p[0] != '\0' && p[0] != '\n' && p[0] != '\r') {
    while (p[0] != '\0'
           && (p[0] == ' ' || p[0] == '\t' || p[0] == '\f' || p[0] == '\v')) {
      p++;
    }

    if (p[0] == '\0' || p[0] == '\n' || p[0] == '\r' || p[0] == '#')
      break;

    if (count < min_count) {
      char *tok;

      tok = p;
      if (tok[0] == '+' || tok[0] == '-')
        tok++;
      if (!(ak_str_isdigit_fast(tok[0]) || tok[0] == '.')) {
        if (endp)
          *endp = p;
        return count;
      }
    }

    if (count < cap)
      p = ak_str_parse_float_fast(p, NULL, &values[count]);
    else
      p = ak_str_parse_float_fast(p, NULL, &ignored);
    count++;

    while (p[0] != '\0' && !WOBJ_TOKEN_SEP(p[0]) && p[0] != '#')
      p++;
  }

  if (ok)
    *ok = count >= min_count;
  if (endp)
    *endp = p;

  return count;
}

AK_INLINE
bool
wobj_float_token_start(char c) {
  return ak_str_isdigit_fast(c) || c == '-' || c == '+' || c == '.';
}

static
bool
wobj_parse_float3_exact_line(char * __restrict p,
                             float             values[3],
                             char           ** __restrict endp) {
  uint32_t i;

  for (i = 0; i < 3; i++) {
    p = wobj_skip_inline_space(p);
    if (!wobj_float_token_start(p[0]))
      return false;

    p = ak_str_parse_float_fast(p, NULL, &values[i]);
    if (!WOBJ_TOKEN_SEP(p[0]) && p[0] != '#')
      return false;
  }

  p = wobj_skip_inline_space(p);
  if (endp)
    *endp = p;

  return p[0] == '\0' || p[0] == '\n' || p[0] == '\r' || p[0] == '#';
}

static
void
wobj_promote_vec3_to_vec4(WOState         * __restrict wst,
                          AkDataContext  ** __restrict dctxp,
                          float                         w) {
  AkDataContext *old, *newctx;
  AkDataChunk   *chunk;
  size_t         i, n;

  old    = *dctxp;
  newctx = ak_data_new(wst->tmp, WOBJ_DATA_NODE_ITEMS, sizeof(vec4), NULL);

  for (chunk = old->data; chunk; chunk = chunk->next) {
    vec3 *src;

    src = (vec3 *)(void *)chunk->data;
    n   = chunk->usedsize / sizeof(vec3);
    for (i = 0; i < n; i++) {
      vec4 v;

      v[0] = src[i][0];
      v[1] = src[i][1];
      v[2] = src[i][2];
      v[3] = w;
      memcpy(wobj_data_append_slot(newctx), v, sizeof(v));
    }
  }

  *dctxp = newctx;
}

static
void
wobj_promote_vec2_to_vec3(WOState         * __restrict wst,
                          AkDataContext  ** __restrict dctxp,
                          float                         z) {
  AkDataContext *old, *newctx;
  AkDataChunk   *chunk;
  size_t         i, n;

  old    = *dctxp;
  newctx = ak_data_new(wst->tmp, WOBJ_DATA_NODE_ITEMS, sizeof(vec3), NULL);

  for (chunk = old->data; chunk; chunk = chunk->next) {
    vec2 *src;

    src = (vec2 *)(void *)chunk->data;
    n   = chunk->usedsize / sizeof(vec2);
    for (i = 0; i < n; i++) {
      vec3 v;

      v[0] = src[i][0];
      v[1] = src[i][1];
      v[2] = z;
      memcpy(wobj_data_append_slot(newctx), v, sizeof(v));
    }
  }

  *dctxp = newctx;
}

static
void
wobj_ensure_color_context(WOState * __restrict wst, size_t existingCount) {
  size_t i, count;

  if (wst->dc_col)
    return;

  wst->dc_col = ak_data_new(wst->tmp, WOBJ_DATA_NODE_ITEMS, sizeof(vec4), NULL);
  count       = existingCount;

  for (i = 0; i < count; i++) {
    vec4 white = {1.0f, 1.0f, 1.0f, 1.0f};
    memcpy(wobj_data_append_slot(wst->dc_col), white, sizeof(white));
  }
}

static
void
wobj_append_position(WOState  * __restrict wst,
                     float    * __restrict values,
                     bool                  hasW) {
  if (hasW && wst->posCompSize != AK_COMPONENT_SIZE_VEC4) {
    wobj_promote_vec3_to_vec4(wst, &wst->dc_pos, 1.0f);
    wst->posCompSize = AK_COMPONENT_SIZE_VEC4;
  }

  if (wst->posCompSize == AK_COMPONENT_SIZE_VEC4) {
    vec4 pos;

    pos[0] = values[0];
    pos[1] = values[1];
    pos[2] = values[2];
    pos[3] = hasW ? values[3] : 1.0f;
    memcpy(wobj_data_append_slot(wst->dc_pos), pos, sizeof(pos));
  } else {
    vec3 pos;

    pos[0] = values[0];
    pos[1] = values[1];
    pos[2] = values[2];
    memcpy(wobj_data_append_slot(wst->dc_pos), pos, sizeof(pos));
  }
}

static
void
wobj_append_color(WOState  * __restrict wst,
                  float    * __restrict values,
                  uint32_t              start,
                  bool                  hasColor,
                  bool                  hasAlpha) {
  vec4 color;

  if (!hasColor) {
    if (!wst->dc_col)
      return;

    color[0] = 1.0f;
    color[1] = 1.0f;
    color[2] = 1.0f;
    color[3] = 1.0f;
  } else {
    wobj_ensure_color_context(wst,
                              wst->dc_pos->itemcount > 0
                              ? wst->dc_pos->itemcount - 1
                              : 0);
    color[0] = values[start];
    color[1] = values[start + 1];
    color[2] = values[start + 2];
    color[3] = hasAlpha ? values[start + 3] : 1.0f;
    if (hasAlpha)
      wst->hasColorAlpha = true;
  }

  memcpy(wobj_data_append_slot(wst->dc_col), color, sizeof(color));
}

static
void
wobj_append_texcoord(WOState  * __restrict wst,
                     float    * __restrict values,
                     uint32_t              count) {
  bool hasW;

  hasW = count >= 3;
  if (hasW && wst->texCompSize != AK_COMPONENT_SIZE_VEC3) {
    wobj_promote_vec2_to_vec3(wst, &wst->dc_tex, 0.0f);
    wst->texCompSize = AK_COMPONENT_SIZE_VEC3;
  }

  if (wst->texCompSize == AK_COMPONENT_SIZE_VEC3) {
    vec3 tex;

    tex[0] = count > 0 ? values[0] : 0.0f;
    tex[1] = count > 1 ? values[1] : 0.0f;
    tex[2] = hasW ? values[2] : 0.0f;
    memcpy(wobj_data_append_slot(wst->dc_tex), tex, sizeof(tex));
  } else {
    vec2 tex;

    tex[0] = count > 0 ? values[0] : 0.0f;
    tex[1] = count > 1 ? values[1] : 0.0f;
    memcpy(wobj_data_append_slot(wst->dc_tex), tex, sizeof(tex));
  }
}

static
WOPrim*
wobj_prepare_prim_kind(WOState             * __restrict wst,
                       WOPrim              * __restrict prim,
                       AkMeshPrimitiveType              kind) {
  WOPrim *it;

  if (prim->kind == 0) {
    for (it = wst->obj ? wst->obj->prim : NULL; it; it = it->next) {
      if (it == prim
          || it->kind != kind
          || it->maxVC == 0
          || it->smooth != prim->smooth)
        continue;
      if (it->mtlname == prim->mtlname
          || (it->mtlname && prim->mtlname
              && strcmp(it->mtlname, prim->mtlname) == 0))
        return it;
    }

    prim->kind = kind;
    return prim;
  }

  if (prim->kind == kind)
    return prim;

  for (it = wst->obj ? wst->obj->prim : NULL; it; it = it->next) {
    if (it->kind != kind || it->maxVC == 0 || it->smooth != prim->smooth)
      continue;
    if (it->mtlname == prim->mtlname
        || (it->mtlname && prim->mtlname
            && strcmp(it->mtlname, prim->mtlname) == 0))
      return it;
  }

  prim       = wobj_switchPrim(wst, prim->mtlname);
  prim->kind = kind;

  return prim;
}

static
bool
wobj_parse_smooth(char * __restrict p) {
  AkInt smooth;

  while (p[0] != '\0'
         && (p[0] == ' ' || p[0] == '\t' || p[0] == '\f' || p[0] == '\v'))
    p++;

  if (p[0] == '0'
      || (p[0] == 'o' && p[1] == 'f' && p[2] == 'f')
      || (p[0] == 'n' && p[1] == 'u' && p[2] == 'l' && p[3] == 'l'))
    return false;

  if (p[0] == 'o' && p[1] == 'n')
    return true;

  smooth = 0;
  ak_strtoi_one_fast(p, &smooth);

  return smooth != 0;
}

static
char*
wobj_parse_position_index_token(char  * __restrict p,
                                AkInt * __restrict idx) {
  p = wobj_parse_face_index(p, idx);
  while (p && p[0] != '\0' && !WOBJ_TOKEN_SEP(p[0]))
    p++;

  return p;
}

AK_INLINE
char*
wobj_skip_inline_space(char * __restrict p) {
  while (p[0] == ' ' || p[0] == '\t' || p[0] == '\f' || p[0] == '\v')
    p++;
  return p;
}

AK_INLINE
bool
wobj_parse_positive_index_fast(char  ** __restrict pp,
                               AkInt  * __restrict out) {
  char   *p;
  AkUInt  value;

  p = *pp;
  if (!ak_str_isdigit_fast(*p))
    return false;

  value = 0;
  do {
    value = value * 10u + (AkUInt)(*p++ - '0');
    if (value > (AkUInt)INT32_MAX)
      return false;
  } while (ak_str_isdigit_fast(*p));

  if (value == 0)
    return false;

  *out = (AkInt)value;
  *pp  = p;
  return true;
}

AK_INLINE
bool
wobj_face_first_token_vn_fast(char * __restrict p) {
  p = wobj_skip_inline_space(p);
  if (!ak_str_isdigit_fast(*p))
    return false;

  do {
    p++;
  } while (ak_str_isdigit_fast(*p));

  return p[0] == '/' && p[1] == '/';
}

static
bool
wobj_try_parse_tri_vn_fast(WOPrim  * __restrict prim,
                           char   ** __restrict pp,
                           size_t                posCount,
                           size_t                norCount) {
  char  *p;
  AkInt *faces;
  AkInt  vals[9];
  AkUInt p0, p1, p2;
  uint32_t i;

  p = *pp;
  for (i = 0; i < 3; i++) {
    AkInt posIdx, norIdx;

    p = wobj_skip_inline_space(p);
    if (!wobj_parse_positive_index_fast(&p, &posIdx))
      return false;
    if (p[0] != '/' || p[1] != '/')
      return false;
    p += 2;
    if (!wobj_parse_positive_index_fast(&p, &norIdx))
      return false;
    if (!WOBJ_TOKEN_SEP(p[0]) && p[0] != '#')
      return false;
    if ((AkUInt)posIdx > (AkUInt)posCount
        || (AkUInt)norIdx > (AkUInt)norCount)
      return false;

    vals[i * 3 + 0] = posIdx;
    vals[i * 3 + 1] = 0;
    vals[i * 3 + 2] = norIdx;
  }

  p = wobj_skip_inline_space(p);
  if (p[0] != '\0' && p[0] != '\n' && p[0] != '\r' && p[0] != '#')
    return false;

  p0 = (AkUInt)vals[0] - 1u;
  p1 = (AkUInt)vals[3] - 1u;
  p2 = (AkUInt)vals[6] - 1u;
  if (p0 == p1 || p0 == p2 || p1 == p2)
    return false;

  faces = wobj_data_append_slots(prim->dc_face, 3);
  for (i = 0; i < 3; i++) {
    AkInt *face;

    face    = &faces[i * 3];
    face[0] = vals[i * 3 + 0];
    face[1] = vals[i * 3 + 1];
    face[2] = vals[i * 3 + 2];
  }

  prim->hasNormal      = true;
  prim->missingTexture = true;
  prim->maxVC          = GLM_MAX(prim->maxVC, 3u);
  *(int32_t *)wobj_data_append_slot(prim->dc_vcount) = 3;
  *pp = p;
  return true;
}

static
bool
wobj_try_parse_tri_fast(WOPrim  * __restrict prim,
                        char   ** __restrict pp,
                        size_t                posCount,
                        size_t                texCount,
                        size_t                norCount) {
  char  *p;
  AkInt *faces;
  AkInt  vals[9];
  AkUInt p0, p1, p2;
  int    mode;
  uint32_t i;

  p = *pp;
  mode = -1;
  for (i = 0; i < 3; i++) {
    AkInt posIdx, texIdx, norIdx;
    int   tokenMode;

    p = wobj_skip_inline_space(p);
    if (!wobj_parse_positive_index_fast(&p, &posIdx))
      return false;
    texIdx = 0;
    norIdx = 0;
    tokenMode = 0;
    if (p[0] == '/') {
      p++;
      if (p[0] == '/') {
        p++;
        if (!wobj_parse_positive_index_fast(&p, &norIdx))
          return false;
        tokenMode = 2;
      } else {
        if (!wobj_parse_positive_index_fast(&p, &texIdx))
          return false;
        tokenMode = 1;
        if (p[0] == '/') {
          p++;
          if (!WOBJ_TOKEN_SEP(p[0]) && p[0] != '#') {
            if (!wobj_parse_positive_index_fast(&p, &norIdx))
              return false;
            tokenMode = 3;
          }
        }
      }
    }

    if (mode < 0)
      mode = tokenMode;
    else if (mode != tokenMode)
      return false;

    if (!WOBJ_TOKEN_SEP(p[0]) && p[0] != '#')
      return false;
    if ((AkUInt)posIdx > (AkUInt)posCount)
      return false;
    if (texIdx && (AkUInt)texIdx > (AkUInt)texCount)
      return false;
    if (norIdx && (AkUInt)norIdx > (AkUInt)norCount)
      return false;

    vals[i * 3 + 0] = posIdx;
    vals[i * 3 + 1] = texIdx;
    vals[i * 3 + 2] = norIdx;
  }

  p = wobj_skip_inline_space(p);
  if (p[0] != '\0' && p[0] != '\n' && p[0] != '\r' && p[0] != '#')
    return false;

  p0 = (AkUInt)vals[0] - 1u;
  p1 = (AkUInt)vals[3] - 1u;
  p2 = (AkUInt)vals[6] - 1u;
  if (p0 == p1 || p0 == p2 || p1 == p2)
    return false;

  faces = wobj_data_append_slots(prim->dc_face, 3);
  for (i = 0; i < 3; i++) {
    AkInt *face;

    face    = &faces[i * 3];
    face[0] = vals[i * 3 + 0];
    face[1] = vals[i * 3 + 1];
    face[2] = vals[i * 3 + 2];
  }

  if (mode == 1 || mode == 3)
    prim->hasTexture = true;
  else
    prim->missingTexture = true;

  if (mode == 2 || mode == 3)
    prim->hasNormal = true;
  else
    prim->missingNormal = true;

  prim->maxVC = GLM_MAX(prim->maxVC, 3u);
  *(int32_t *)wobj_data_append_slot(prim->dc_vcount) = 3;
  *pp = p;
  return true;
}

AK_HIDE
AkResult
wobj_obj(AkDoc     ** __restrict dest,
         const char * __restrict filepath) {
  AkHeap   *heap;
  AkDoc    *doc;
  void     *objstr;
  char     *p, *begin, *end, *m;
  AkScene  *scene;
  AkNode   *rootNode;
  WOPrim   *prim;
  WOState   wstVal = {0}, *wst;
  size_t    objstrSize;
  AkResult  ret;
  uint32_t  vc;
  char      c;

  if ((ret = ak_readfile(filepath, NULL, &objstr, &objstrSize)) != AK_OK)
    return ret;

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));

  doc->inf                = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name          = filepath;
  doc->inf->dir           = ak_path_dir(heap, doc, filepath);
  doc->inf->flipImage     = true;
  doc->inf->ftype         = AK_FILE_TYPE_WAVEFRONT;
  doc->inf->base.coordSys = AK_YUP;
  doc->coordSys           = AK_YUP; /* Default */

  if (!((p = objstr) && (c = *p) != '\0')) {
    ak_free(doc);
    ak_releasefile(objstr, objstrSize);
    return AK_ERR;
  }
  
  ak_heap_setdata(heap, doc);
  ak_id_newheap(heap);

  /* default scene */
  scene                  = ak_heap_calloc(heap, doc, sizeof(*scene));
  scene->node            = ak_heap_calloc(heap, scene, sizeof(*scene->node));
  ak_setypeid(scene->node, AKT_NODE);
  scene->node->visible   = true;
  rootNode               = ak_heap_calloc(heap, doc, sizeof(*rootNode));
  ak_setypeid(rootNode, AKT_NODE);
  rootNode->visible      = true;
  AK_LIB_PREPEND(doc->lib.nodes, rootNode, docNext);
  ak_addSubNode(scene->node, rootNode, false);
  AK_LIB_PREPEND(doc->lib.scenes, scene, next);
  doc->scene             = scene;

  /* parse state */
  memset(&wstVal, 0, sizeof(wstVal));
  wst              = &wstVal;
  wstVal.doc       = doc;
  wstVal.heap      = heap;
  wstVal.tmp       = ak_heap_alloc(heap, doc, sizeof(void*));
  wstVal.node      = rootNode;
  wstVal.lib_geom  = &doc->lib.geometries;
  wstVal.posCompSize = AK_COMPONENT_SIZE_VEC3;
  wstVal.texCompSize = AK_COMPONENT_SIZE_VEC2;

  /* vertex data (shared across file) */
  wst->dc_pos      = ak_data_new(wst->tmp,
                                 WOBJ_DATA_NODE_ITEMS,
                                 sizeof(vec3),
                                 NULL);
  wst->dc_tex      = ak_data_new(wst->tmp,
                                 WOBJ_DATA_NODE_ITEMS,
                                 sizeof(vec2),
                                 NULL);
  wst->dc_nor      = ak_data_new(wst->tmp,
                                 WOBJ_DATA_NODE_ITEMS,
                                 sizeof(vec3),
                                 NULL);

  /* default group */
  wobj_switchObject(wst);

  prim = wst->obj->prim;
  
  /* parse .obj */
  do {
    /* skip spaces */
    SKIP_SPACES

    if (p[1] == ' ' || p[1] == '\t') {
      switch (c) {
        case '#': {
          /* ignore comments */
          while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
          /* while ((c = *++p) != '\0' &&  AK_ARRAY_NLINE_CHECK); */
          break;
        }
        case 'v': {
          float    values[8];
          char    *lineEnd;
          uint32_t nValues;
          bool     hasW, hasColor, hasAlpha;
          bool     validValues;
          uint32_t colorStart;

          if (*++p == '\0')
            goto err;

          lineEnd = p;
          if (wobj_parse_float3_exact_line(p, values, &lineEnd)) {
            nValues     = 3;
            validValues = true;
            p           = lineEnd;
          } else {
            nValues = wobj_parse_float_line(p,
                                            values,
                                            8,
                                            3,
                                            &validValues,
                                            &lineEnd);
            p = lineEnd;
          }
          if (!validValues)
            break;

          hasW       = nValues == 4 || nValues >= 8;
          hasColor   = nValues == 6 || nValues == 7 || nValues >= 8;
          hasAlpha   = nValues == 7 || nValues >= 8;
          colorStart = nValues >= 8 ? 4u : 3u;

          wobj_append_position(wst, values, hasW);
          wobj_append_color(wst, values, colorStart, hasColor, hasAlpha);
          break;
        }
        case 'f': {
          WODataMark faceMark;
          AkUInt     seen[WOBJ_FACE_STACK_LIMIT];
          uint32_t   seenCount;
          size_t     posCount, texCount, norCount;
          bool       validFace;
          bool       duplicateFace;
          bool       oldHasTexture, oldHasNormal;
          bool       oldMissingTexture, oldMissingNormal;
          bool       parsedFast;

          if ((c = *(p += 2)) == '\0')
            goto err;

          prim = wobj_prepare_prim_kind(wst, prim, AK_PRIMITIVE_TRIANGLES);
          posCount = wst->dc_pos->itemcount;
          texCount = wst->dc_tex->itemcount;
          norCount = wst->dc_nor->itemcount;

          parsedFast = false;
          switch (prim->faceFastPath) {
            case WOBJ_FACE_FAST_DISABLED:
              break;
            case WOBJ_FACE_FAST_TRI_VN:
              parsedFast = wobj_try_parse_tri_vn_fast(prim,
                                                       &p,
                                                       posCount,
                                                       norCount);
              if (!parsedFast && !wobj_face_first_token_vn_fast(p)) {
                parsedFast = wobj_try_parse_tri_fast(prim,
                                                     &p,
                                                     posCount,
                                                     texCount,
                                                     norCount);
                if (parsedFast)
                  prim->faceFastPath = WOBJ_FACE_FAST_TRI;
              }
              break;
            case WOBJ_FACE_FAST_TRI:
              parsedFast = wobj_try_parse_tri_fast(prim,
                                                   &p,
                                                   posCount,
                                                   texCount,
                                                   norCount);
              break;
            default:
              if (wobj_face_first_token_vn_fast(p)) {
                parsedFast = wobj_try_parse_tri_vn_fast(prim,
                                                        &p,
                                                        posCount,
                                                        norCount);
                if (parsedFast)
                  prim->faceFastPath = WOBJ_FACE_FAST_TRI_VN;
              } else {
                parsedFast = wobj_try_parse_tri_fast(prim,
                                                     &p,
                                                     posCount,
                                                     texCount,
                                                     norCount);
                if (parsedFast)
                  prim->faceFastPath = WOBJ_FACE_FAST_TRI;
              }
            }
          if (parsedFast)
            break;
          if (prim->faceFastPath != WOBJ_FACE_FAST_DISABLED) {
            if (prim->faceFastMisses < UINT8_MAX)
              prim->faceFastMisses++;
            if (prim->faceFastMisses >= WOBJ_FACE_FAST_MISS_LIMIT)
              prim->faceFastPath = WOBJ_FACE_FAST_DISABLED;
          }

          faceMark = wobj_data_mark(prim->dc_face);
          seenCount = 0;
          validFace = true;
          duplicateFace = false;
          oldHasTexture = prim->hasTexture;
          oldHasNormal = prim->hasNormal;
          oldMissingTexture = prim->missingTexture;
          oldMissingNormal = prim->missingNormal;
          vc = 0;

          while (p
                 && p[0] != '\0'
                 && p[0] != '\n'
                 && p[0] != '\r') {
            AkInt *face;
            AkInt idx;
            bool   hasTexIndex, hasNorIndex;

            /* vertex index */
            SKIP_SPACES

            if (AK_ARRAY_NLINE_CHECK)
              break;

            face = wobj_data_append_slot(prim->dc_face);
            p = wobj_parse_face_index(p, &idx);
            face[0] = idx;
            face[1] = 0;
            face[2] = 0;
            hasTexIndex = false;
            hasNorIndex = false;
            if (!wobj_index_valid(idx, posCount)) {
              validFace = false;
            } else if (seenCount < AK_ARRAY_LEN(seen)) {
              AkUInt realIndex;

              realIndex = wobj_index_real_unchecked(idx, posCount);
              if (wobj_seen_index(seen, seenCount, realIndex)) {
                duplicateFace = true;
              } else {
                seen[seenCount++] = realIndex;
              }
            }

            /* texture index */
            if (p && p[0] == '/') {
              p++;
              if (p[0] != '/'
                  && p[0] != '\0'
                  && !WOBJ_TOKEN_SEP(p[0])) {
                p = wobj_parse_face_index(p, &idx);
                face[1] = idx;
                hasTexIndex = idx != 0;
                if (hasTexIndex && !wobj_index_valid(idx, texCount))
                  validFace = false;
                
                if (!prim->hasTexture)
                  prim->hasTexture = true;
              }
            }
            
            /* normal index */
            if (p && p[0] == '/') {
              p++;
              if (p[0] != '\0'
                  && !WOBJ_TOKEN_SEP(p[0])) {
                p = wobj_parse_face_index(p, &idx);
                face[2] = idx;
                hasNorIndex = idx != 0;
                if (hasNorIndex && !wobj_index_valid(idx, norCount))
                  validFace = false;

                if (!prim->hasNormal)
                  prim->hasNormal = true;
              }
            }

            if (p && p[0] == '/')
              validFace = false;

            if (!hasTexIndex)
              prim->missingTexture = true;
            if (!hasNorIndex)
              prim->missingNormal = true;

            vc += 1;

            while (p
                   && p[0] != '\0'
                   && !WOBJ_TOKEN_SEP(p[0]))
              p++;
          }

          if (vc < 3)
            validFace = false;
          else if (validFace && duplicateFace)
            validFace = wobj_compact_duplicate_face(prim->dc_face,
                                                    &faceMark,
                                                    posCount,
                                                    &vc);

          if (validFace) {
            prim->maxVC = GLM_MAX(prim->maxVC, vc);
            *(int32_t *)wobj_data_append_slot(prim->dc_vcount) = (int32_t)vc;
            if (vc == 3) {
              prim->faceFastPath   = WOBJ_FACE_FAST_UNKNOWN;
              prim->faceFastMisses = 0;
            }
          } else {
            wobj_data_rollback(prim->dc_face, &faceMark);
            prim->hasTexture = oldHasTexture;
            prim->hasNormal = oldHasNormal;
            prim->missingTexture = oldMissingTexture;
            prim->missingNormal = oldMissingNormal;
          }
          break;
        }
        case 'l': {
          AkInt prev;
          size_t posCount;
          bool  hasPrev;

          if ((c = *(p += 2)) == '\0')
            goto err;

          prim    = wobj_prepare_prim_kind(wst, prim, AK_PRIMITIVE_LINES);
          posCount = wst->dc_pos->itemcount;
          hasPrev = false;
          vc      = 0;

          while (p
                 && p[0] != '\0'
                 && p[0] != '\n'
                 && p[0] != '\r') {
            AkInt idx;

            while (p[0] != '\0'
                   && (p[0] == ' ' || p[0] == '\t'
                       || p[0] == '\f' || p[0] == '\v'))
              p++;
            if (p[0] == '\0' || p[0] == '\n' || p[0] == '\r' || p[0] == '#')
              break;

            p = wobj_parse_position_index_token(p, &idx);
            if (!wobj_index_valid(idx, posCount)) {
              break;
            }

            if (hasPrev) {
              AkInt *a, *b;

              a    = wobj_data_append_slot(prim->dc_face);
              a[0] = prev;
              a[1] = 0;
              a[2] = 0;

              b    = wobj_data_append_slot(prim->dc_face);
              b[0] = idx;
              b[1] = 0;
              b[2] = 0;
            }

            prev    = idx;
            hasPrev = true;
            vc++;
          }

          if (vc > 1) {
            prim->maxVC = 2;
            *(int32_t *)wobj_data_append_slot(prim->dc_vcount) = (int32_t)vc;
          }
          break;
        }
        case 'p': {
          if ((c = *(p += 2)) == '\0')
            goto err;

          prim = wobj_prepare_prim_kind(wst, prim, AK_PRIMITIVE_POINTS);
          vc   = 0;

          while (p
                 && p[0] != '\0'
                 && p[0] != '\n'
                 && p[0] != '\r') {
            AkInt idx;
            AkInt *point;

            while (p[0] != '\0'
                   && (p[0] == ' ' || p[0] == '\t'
                       || p[0] == '\f' || p[0] == '\v'))
              p++;
            if (p[0] == '\0' || p[0] == '\n' || p[0] == '\r' || p[0] == '#')
              break;

            p = wobj_parse_position_index_token(p, &idx);
            if (wobj_index_valid(idx, wst->dc_pos->itemcount)) {
              point    = wobj_data_append_slot(prim->dc_face);
              point[0] = idx;
              point[1] = 0;
              point[2] = 0;
              vc++;
            }
          }

          if (vc > 0) {
            prim->maxVC = 1;
            *(int32_t *)wobj_data_append_slot(prim->dc_vcount) = (int32_t)vc;
          }
          break;
        }
        case 'o': {
          wobj_switchObject(wst);
          prim = wst->obj->prim;
          break;
        }
        case 's': {
          bool smooth;

          if ((c = *(p += 2)) == '\0')
            goto err;

          smooth = wobj_parse_smooth(p);
          if (wst->smooth != smooth) {
            wst->smooth = smooth;
            if (prim) {
              if (prim->dc_face->itemcount > 0)
                prim = wobj_switchPrim(wst, prim->mtlname);
              else
                prim->smooth = smooth;
            }
          }
          break;
        }
        case 'g':
          break;
        default:
          break;
      }
    } else if (p[2] == ' ' || p[2] == '\t') {
      if (p[0] == 'v' && p[1] == 'n') {
        float values[3];
        char *lineEnd;
        uint32_t nValues;
        bool validValues;
        float *nor;

        if (*(p += 2) == '\0')
          goto err;

        lineEnd = p;
        if (wobj_parse_float3_exact_line(p, values, &lineEnd)) {
          nValues     = 3;
          validValues = true;
          p           = lineEnd;
        } else {
          nValues = wobj_parse_float_line(p,
                                          values,
                                          3,
                                          3,
                                          &validValues,
                                          &lineEnd);
          p = lineEnd;
        }
        if (!validValues || nValues < 3)
          goto skip_line;

        nor = wobj_data_append_slot(wst->dc_nor);
        memcpy(nor, values, sizeof(values));
      } else if (p[0] == 'v' && p[1] == 't') {
        float    values[3];
        char    *lineEnd;
        uint32_t nValues;
        bool     validValues;

        if (*(p += 2) == '\0')
          goto err;

        lineEnd = p;
        nValues = wobj_parse_float_line(p,
                                        values,
                                        3,
                                        1,
                                        &validValues,
                                        &lineEnd);
        p = lineEnd;
        if (!validValues)
          goto skip_line;

        wobj_append_texcoord(wst, values, nValues);
      }
    } else if (ak_str_pack4_fast(p, 4) == WOBJ_KW_MTLL
               && p[4] == 'i'
               && p[5] == 'b'
               && (p[6] == ' ' || p[6] == '\t')) {
      p += 7;
      SKIP_SPACES

      begin = p;
      while ((c = *p) != '\0' && !AK_ARRAY_NLINE_CHECK)
        p++;
      end = p;

      if (end > begin
          && (m = ak_heap_strndup(heap, wst->doc, begin, end - begin)))
        wst->mtlib = wobj_mtl(wst, m);
    } else if (ak_str_pack4_fast(p, 4) == WOBJ_KW_USEM
               && p[4] == 't'
               && p[5] == 'l'
               && (p[6] == ' ' || p[6] == '\t')) {
      p += 7;
      SKIP_SPACES

      begin = p;
      while ((c = *p) != '\0' && !AK_ARRAY_NLINE_CHECK)
        p++;
      end = p;

      m = NULL;
      if (end > begin)
        m = ak_heap_strndup(heap, wst->doc, begin, end - begin);
      
      wst->mtlname = m;
      prim = wobj_switchPrim(wst, m);
    } else if (wobj_is_freeform_keyword(p)) {
      wst->hasFreeform = true;
    }
    
skip_line:
    NEXT_LINE
  } while (p && p[0] != '\0'/* && (c = *++p) != '\0'*/);

  wobj_prepareMissingDefaults(wst);

  wst->ac_pos = wobj_acc(wst, wst->dc_pos, wst->posCompSize, AKT_FLOAT);
  if (wst->dc_nor->itemcount > 0)
    wst->ac_nor = wobj_acc(wst, wst->dc_nor, AK_COMPONENT_SIZE_VEC3, AKT_FLOAT);
  if (wst->dc_tex->itemcount > 0)
    wst->ac_tex = wobj_acc(wst, wst->dc_tex, wst->texCompSize, AKT_FLOAT);
  if (wst->dc_col && wst->dc_col->itemcount > 0)
    wst->ac_col = wobj_acc(wst, wst->dc_col, AK_COMPONENT_SIZE_VEC4, AKT_FLOAT);

  wobj_finishObjects(wst);

  io_postscript(doc);

  *dest = doc;
  
  /* cleanup */
  ak_free(wst->tmp);
  ak_releasefile(objstr, objstrSize);

  return AK_OK;

err:
  ak_free(doc);
  
  if (objstr)
    ak_releasefile(objstr, objstrSize);

  return AK_ERR;
}

#undef WOBJ_TOKEN_SEP
