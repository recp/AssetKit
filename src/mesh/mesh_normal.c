/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "mesh_util.h"
#include "../common.h"
#include "../id.h"
#include "../memory_common.h"
#include "../data.h"

#include "mesh_index.h"

#include <cglm/cglm.h>

_assetkit_hide
void
ak_meshPrimGenNormals(AkMeshPrimitive    * __restrict prim);

AK_EXPORT
bool
ak_meshPrimNeedsNormals(AkMeshPrimitive * __restrict prim) {
  AkAccessor *acc;
  AkSource   *src;
  AkObject   *data;
  AkInput    *input;
  bool        ret;

  ret   = true;
  input = prim->input;
  while (input) {
    if (input->semantic == AK_INPUT_SEMANTIC_NORMAL) {
      src = ak_getObjectByUrl(&input->source);
      if (!src
          || !(acc = src->tcommon)
          || !(data = ak_getObjectByUrl(&acc->source)))
        return ret;
      ret = false;
      break;
    }

    input = input->next;
  }

  return ret;
}

AK_EXPORT
bool
ak_meshNeedsNormals(AkMesh * __restrict mesh) {
  AkMeshPrimitive *prim;
  bool ret;

  ret  = false;
  prim = mesh->primitive;
  while (prim) {
    ret |= ak_meshPrimNeedsNormals(prim);
    if (ret)
      break;
    prim = prim->next;
  }

  return ret;
}

_assetkit_hide
void
ak_meshPrimGenNormals(AkMeshPrimitive * __restrict prim) {
  AkDataContext *dctx;
  AkUIntArray   *inpIndices;
  AkFloat       *pos;
  AkUInt        *it, *it2;
  AkHeap        *heap;
  AkInput       *input, *nextInput;
  AkBuffer      *posBuff, *buff;
  AkSource      *posSource, *src;
  AkAccessor    *posAcc, *acc;
  char          *srcurl;
  AkUInt         st, newst;
  AkInt          vo, pos_st;
  size_t         count;

  if ((prim->type != AK_MESH_PRIMITIVE_TYPE_TRIANGLES
       && prim->type != AK_MESH_PRIMITIVE_TYPE_POLYGONS)
      || !prim->pos
      || !(posSource = ak_getObjectByUrl(&prim->pos->source))
      || !(posAcc    = posSource->tcommon)
      || !(posBuff   = ak_getObjectByUrl(&posAcc->source))
      || (vo = prim->pos->offset) == -1)
    return;

  dctx   = ak_data_new(prim, 64, sizeof(vec3), ak_cmp_vec3);
  heap   = ak_heap_getheap(prim);
  pos    = posBuff->data;
  pos_st = posAcc->stride;

  if (!prim->indices || prim->indices->count == 0)
    return;

  it    = prim->indices->items + vo;
  st    = prim->indexStride;
  count = prim->indices->count / st;
  newst = st + 1;

  src   = ak_mesh_src_for(heap, prim->mesh, prim, AK_INPUT_SEMANTIC_NORMAL);
  acc   = src->tcommon;
  buff  = ak_getObjectByUrl(&acc->source);

  /* TODO: for now join this into existing indices,
           but in the future use separate to fix indices  */
  inpIndices = ak_heap_calloc(heap,
                              prim,
                              sizeof(*inpIndices)
                              + count * newst * sizeof(AkUInt));
  inpIndices->count = count * newst;
  it2 = inpIndices->items;

  switch (prim->type) {
    case AK_MESH_PRIMITIVE_TYPE_POLYGONS: {
      AkPolygon *poly;
      AkUInt    *vc_it;
      vec3       v1, v2, n;
      size_t     i, k, vc, j;
      AkUInt     idx;

      poly = (AkPolygon *)prim;

      /* polygon ha triangulated */
      if (!poly->vcount)
        goto tri;

      vc_it = poly->vcount->items;

      for (i = k = 0; k < poly->vcount->count; k++) {
        float *a, *b, *c;

        vc = vc_it[k];

        /* TODO: normals for lines or points ? */
        if (vc < 3)
          continue;

        a = pos + it[i + vo]           * pos_st;
        b = pos + it[i + vo + st]      * pos_st;
        c = pos + it[i + vo + st + st] * pos_st;

        glm_vec_sub(a, b, v1);
        glm_vec_sub(b, c, v2);

        glm_vec_cross(v1, v2, n);
        glm_vec_normalize(n);

        idx = ak_data_append(dctx, n);

        for (j = i; j < i + vc; j++) {
          /* other inputs */
          memcpy(it2 + j * newst,
                 it  + j * st,
                 sizeof(AkUInt) * st);

          /* normal */
          it2[j * newst + st] = idx;
        }

        i += vc;
      }
      break;
    }
    case AK_MESH_PRIMITIVE_TYPE_TRIANGLES: tri: {
      vec3     v1, v2, n;
      uint32_t i, j;
      AkUInt   idx;

      for (i = 0; i < count; i += 3 /* 3: triangle */) {
        float *a, *b, *c;

        a = pos + it[i + vo]           * pos_st;
        b = pos + it[i + vo + st]      * pos_st;
        c = pos + it[i + vo + st + st] * pos_st;

        glm_vec_sub(a, b, v1);
        glm_vec_sub(b, c, v2);

        glm_vec_cross(v1, v2, n);
        glm_vec_normalize(n);

        idx = ak_data_append(dctx, n);

        for (j = i; j < i + 3; j++) {
          /* other inputs */
          memcpy(it2 + j * newst,
                 it  + j * st,
                 sizeof(AkUInt) * st);

          /* normal */
          it2[j * newst + st] = idx;
        }
      }
      break;
    }
    default:
      ak_free(inpIndices);
      return;
  }

  /* add input */
  srcurl = ak_url_string(heap->allocator, ak_getId(src));
  input = ak_heap_calloc(heap, prim, sizeof(*input));
  input->offset      = st;
  input->semantic    = AK_INPUT_SEMANTIC_NORMAL;
  input->semanticRaw = "NORMAL";

  nextInput = prim->input;
  while (nextInput->next)
    nextInput = nextInput->next;

  nextInput->next = input;

  ak_url_init(input, srcurl, &input->source);

  prim->inputCount++;
  prim->indexStride++;

  ak_free(prim->indices);
  prim->indices = inpIndices;
  heap->allocator->free(srcurl);

  (void)ak_data_join(dctx, buff->data);
  ak_free(dctx);
  
  ak_primFixIndices(heap, prim->mesh, prim);
}

AK_EXPORT
void
ak_meshGenNormals(AkMesh * __restrict mesh) {
  AkMeshPrimitive *prim;

  prim = mesh->primitive;

  ak_meshBeginEdit(mesh);

  while (prim) {
    ak_meshPrimGenNormals(prim);
    prim = prim->next;
  }

  ak_meshEndEdit(mesh);
}
