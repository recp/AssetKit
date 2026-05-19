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
ak_wobjFreeDupl(RBTree *tree, RBNode *node);

static
void
wobj_prepareMissingDefaults(WOState * __restrict wst);

AK_INLINE
void*
wobj_data_append_slot(AkDataContext * __restrict dctx);

#define WOBJ_KW_MTLL AK_STR_PACK4_CHARS('m', 't', 'l', 'l')
#define WOBJ_KW_USEM AK_STR_PACK4_CHARS('u', 's', 'e', 'm')

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

AK_INLINE
void*
wobj_data_append_slot(AkDataContext * __restrict dctx) {
  AkDataChunk *chunk;
  size_t       size;

  size = dctx->itemsize;
  if (dctx->usedsize + size > dctx->size) {
    chunk = ak_heap_alloc(dctx->heap,
                          dctx,
                          sizeof(*chunk) + dctx->nodesize);
    chunk->usedsize = 0;
    chunk->next     = NULL;

    if (dctx->last)
      dctx->last->next = chunk;

    dctx->last  = chunk;
    dctx->size += dctx->nodesize;

    if (!dctx->data)
      dctx->data = chunk;
  } else {
    chunk = dctx->last;
  }

  dctx->usedsize += size;
  dctx->itemcount++;
  chunk->usedsize += size;

  return chunk->data + chunk->usedsize - size;
}

static
uint32_t
wobj_parse_float_line(char     * __restrict p,
                      float    * __restrict values,
                      uint32_t               cap) {
  float    ignored;
  uint32_t count;

  count = 0;
  while (p && p[0] != '\0' && p[0] != '\n' && p[0] != '\r') {
    while (p[0] != '\0'
           && (p[0] == ' ' || p[0] == '\t' || p[0] == '\f' || p[0] == '\v')) {
      p++;
    }

    if (p[0] == '\0' || p[0] == '\n' || p[0] == '\r' || p[0] == '#')
      break;

    if (count < cap) {
      p = ak_strtof_one_fast(p, &values[count]);
    } else {
      p = ak_strtof_one_fast(p, &ignored);
    }
    count++;

    while (p[0] != '\0' && !WOBJ_TOKEN_SEP(p[0]) && p[0] != '#')
      p++;
  }

  return count;
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
  if (prim->kind == 0) {
    prim->kind = kind;
    return prim;
  }

  if (prim->kind == kind)
    return prim;

  prim       = wobj_switchPrim(wst, prim->mtlname);
  prim->kind = kind;

  return prim;
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

#define WOBJ_PARSE_FLOAT2(PTR, DEST)                                          \
  do {                                                                        \
    (PTR) = ak_strtof_one_fast((PTR), &(DEST)[0]);                            \
    (PTR) = ak_strtof_one_fast((PTR), &(DEST)[1]);                            \
  } while (0)

#define WOBJ_PARSE_FLOAT3(PTR, DEST)                                          \
  do {                                                                        \
    (PTR) = ak_strtof_one_fast((PTR), &(DEST)[0]);                            \
    (PTR) = ak_strtof_one_fast((PTR), &(DEST)[1]);                            \
    (PTR) = ak_strtof_one_fast((PTR), &(DEST)[2]);                            \
  } while (0)

AK_HIDE
AkResult
wobj_obj(AkDoc     ** __restrict dest,
         const char * __restrict filepath) {
  AkHeap             *heap;
  AkDoc              *doc;
  void               *objstr;
  char               *p, *begin, *end, *m;
  AkLibrary          *lib_vscene;
  AkVisualScene      *scene;
  WOPrim             *prim;
  WOState             wstVal = {0}, *wst;
  size_t              objstrSize;
  AkResult            ret;
  uint32_t            vc;
  char                c;

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
  
  /* for fixing skin and morph vertices */
  doc->reserved = rb_newtree_ptr();
  ((RBTree *)doc->reserved)->onFreeNode = ak_wobjFreeDupl;
  
  ak_heap_setdata(heap, doc);
  ak_id_newheap(heap);

  /* libraries */
  doc->lib.geometries = ak_heap_calloc(heap, doc, sizeof(AkLibrary));
  lib_vscene = ak_heap_calloc(heap, doc, sizeof(*lib_vscene));
  
  /* default scene */
  scene                  = ak_heap_calloc(heap, doc, sizeof(*scene));
  scene->node            = ak_heap_calloc(heap, doc, sizeof(*scene->node));
  lib_vscene->chld       = &scene->base;
  lib_vscene->count      = 1;
  doc->lib.visualScenes  = lib_vscene;
  doc->scene.visualScene = ak_instanceMake(heap, doc, scene);

  /* parse state */
  memset(&wstVal, 0, sizeof(wstVal));
  wst              = &wstVal;
  wstVal.doc       = doc;
  wstVal.heap      = heap;
  wstVal.tmp       = ak_heap_alloc(heap, doc, sizeof(void*));
  wstVal.node      = scene->node;
  wstVal.lib_geom  = doc->lib.geometries;
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
          uint32_t nValues;
          bool     hasW, hasColor, hasAlpha;
          uint32_t colorStart;

          if (*++p == '\0')
            goto err;

          nValues = wobj_parse_float_line(p, values, 8);
          if (nValues < 3)
            goto err;

          hasW       = nValues == 4 || nValues >= 8;
          hasColor   = nValues == 6 || nValues == 7 || nValues >= 8;
          hasAlpha   = nValues == 7 || nValues >= 8;
          colorStart = nValues >= 8 ? 4u : 3u;

          wobj_append_position(wst, values, hasW);
          wobj_append_color(wst, values, colorStart, hasColor, hasAlpha);
          break;
        }
        case 'f': {
          if ((c = *(p += 2)) == '\0')
            goto err;

          prim = wobj_prepare_prim_kind(wst, prim, AK_PRIMITIVE_TRIANGLES);
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

            /* texture index */
            if (p && p[0] == '/') {
              p++;
              if (p[0] != '/'
                  && p[0] != '\0'
                  && !WOBJ_TOKEN_SEP(p[0])) {
                p = wobj_parse_face_index(p, &idx);
                face[1] = idx;
                hasTexIndex = idx != 0;
                
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

                if (!prim->hasNormal)
                  prim->hasNormal = true;
              }
            }

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

          prim->maxVC = GLM_MAX(prim->maxVC, vc);
          *(int32_t *)wobj_data_append_slot(prim->dc_vcount) = (int32_t)vc;
          break;
        }
        case 'l': {
          AkInt prev;
          bool  hasPrev;

          if ((c = *(p += 2)) == '\0')
            goto err;

          prim    = wobj_prepare_prim_kind(wst, prim, AK_PRIMITIVE_LINES);
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

            p        = wobj_parse_position_index_token(p, &idx);
            point    = wobj_data_append_slot(prim->dc_face);
            point[0] = idx;
            point[1] = 0;
            point[2] = 0;
            vc++;
          }

          if (vc > 0) {
            prim->maxVC = 1;
            *(int32_t *)wobj_data_append_slot(prim->dc_vcount) = (int32_t)vc;
          }
          break;
        }
        case 'o':
        case 'g': {
          wobj_switchObject(wst);
          prim = wst->obj->prim;
          break;
        }
        default:
          break;
      }
    } else if (p[2] == ' ' || p[2] == '\t') {
      if (p[0] == 'v' && p[1] == 'n') {
        float *nor;

        if (*(p += 2) == '\0')
          goto err;

        nor = wobj_data_append_slot(wst->dc_nor);
        WOBJ_PARSE_FLOAT3(p, nor);
      } else if (p[0] == 'v' && p[1] == 't') {
        float    values[3];
        uint32_t nValues;

        if (*(p += 2) == '\0')
          goto err;

        nValues = wobj_parse_float_line(p, values, 3);
        if (nValues < 1)
          goto err;

        wobj_append_texcoord(wst, values, nValues);
      }
    } else if (ak_str_pack4_fast(p, 4) == WOBJ_KW_MTLL
               && p[4] == 'i'
               && p[5] == 'b'
               && (p[6] == ' ' || p[6] == '\t')) {
      p += 7;
      SKIP_SPACES

      begin = p;
      while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
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
      while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
      end = p;

      m = NULL;
      if (end > begin)
        m = ak_heap_strndup(heap, wst->doc, begin, end - begin);
      
      prim = wobj_switchPrim(wst, m);
    }
    
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

#undef WOBJ_PARSE_FLOAT2
#undef WOBJ_PARSE_FLOAT3
#undef WOBJ_TOKEN_SEP

static
void
ak_wobjFreeDupl(RBTree *tree, RBNode *node) {
  if (node == tree->nullNode)
    return;
  ak_free(node->val);
}
