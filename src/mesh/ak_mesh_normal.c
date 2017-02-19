/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_mesh_util.h"
#include "../ak_common.h"
#include "../ak_id.h"
#include "../ak_memory_common.h"
#include "../ak_data.h"

#include "ak_mesh_index.h"

#include <cglm.h>

typedef struct AkGenNormalStruct {
  AkFloat       *pos_it;
  AkDataContext *dctx;
  char          *srcurl;
  int            pos_stride;
} AkGenNormalStruct;

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
    if (input->base.semantic == AK_INPUT_SEMANTIC_NORMAL) {
      src = ak_getObjectByUrl(&input->base.source);
      if (!src
          || !(acc = src->tcommon)
          || !(data = ak_getObjectByUrl(&acc->source)))
        return ret;
      ret = false;
      break;
    }

    input = (AkInput *)input->base.next;
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

AK_EXPORT
void
ak_meshPrimGenNormals(AkMeshPrimitive    * __restrict prim,
                      AkGenNormalStruct  * __restrict objp) {
  AkUIntArray *inpIndices;
  AkFloat     *pos;
  AkUInt      *ind_it2;
  AkHeap      *heap;
  AkInput     *input;
  AkUInt      *ind_it;
  AkUInt       st, newst;
  AkInt        vo, pos_st;
  size_t       c;

  if (prim->type != AK_MESH_PRIMITIVE_TYPE_TRIANGLES
      && prim->type != AK_MESH_PRIMITIVE_TYPE_POLYGONS)
    return;

  vo = ak_mesh_vertex_off(prim);
  if (vo == -1)
    return;

  heap   = ak_heap_getheap(prim);
  pos    = objp->pos_it;
  pos_st = objp->pos_stride;

  if (!prim->indices || prim->indices->count == 0)
    return;

  ind_it = prim->indices->items + vo;
  st     = prim->indexStride;
  newst  = st + 1;
  c      = (prim->indices->count / st) * newst;

  /* TODO: for now join this into existing indices, 
           but in the future use separate to fix indices  */
  inpIndices = ak_heap_calloc(heap,
                              prim,
                              sizeof(*inpIndices) + c * sizeof(AkUInt));
  inpIndices->count = c;
  ind_it2 = inpIndices->items;

  switch (prim->type) {
    case AK_MESH_PRIMITIVE_TYPE_POLYGONS: {
      AkPolygon *poly;
      AkUInt    *vc_it;
      vec3       v1, v2, n;
      size_t     i;
      uint32_t   vc, j;
      AkUInt     idx;

      poly  = (AkPolygon *)prim;
      vc_it = poly->vcount->items;

      for (i = 0; i < poly->vcount->count; i++) {
        float *a, *b, *c;

        vc = vc_it[i];
        if (vc > 2) {
          a = pos + ind_it[i]          * pos_st;
          b = pos + ind_it[i + st]     * pos_st;
          c = pos + ind_it[i + st * 2] * pos_st;

          glm_vec_sub(a, b, v1);
          glm_vec_sub(b, c, v2);

          glm_vec_cross(v1, v2, n);
          glm_vec_normalize(n);

          glm_vec_cross(v1, v2, n);
          glm_vec_normalize(n);

          idx = ak_data_append_unq(objp->dctx, n);

          for (j = 0; j < 3; j++) {
            memcpy(ind_it2 + (i + j) * newst,
                   ind_it  + (i + j) * st,
                   sizeof(AkUInt) * st);
            ind_it2[(i + j) * newst + st] = idx;
          }
        } else {
          /* TODO: normals for lines or points ? */
        }
      }
      break;
    }
    case AK_MESH_PRIMITIVE_TYPE_TRIANGLES: {
      vec3     v1, v2, n;
      uint32_t i, j;
      size_t   tc;
      AkUInt   idx;

      idx = 0;
      tc  = prim->indices->count;

      for (i = vo; i < tc; i += st * 3 /* 3: triangle */) {
        float *a, *b, *c;

        a = pos + ind_it[i]          * pos_st;
        b = pos + ind_it[i + st]     * pos_st;
        c = pos + ind_it[i + st * 2] * pos_st;

        glm_vec_sub(a, b, v1);
        glm_vec_sub(b, c, v2);

        glm_vec_cross(v1, v2, n);
        glm_vec_normalize(n);

        idx = ak_data_append_unq(objp->dctx, n);

        for (j = 0; j < 3; j++) {
          memcpy(ind_it2 + (i + j) * newst,
                 ind_it  + (i + j) * st,
                 sizeof(AkUInt) * st);
          ind_it2[(i + j) * newst + st] = idx;
        }
      }
      break;
    }
    default:
      ak_free(inpIndices);
      return;
  }

  /* add input */
  input = ak_heap_calloc(heap, prim, sizeof(*input));
  input->offset           = prim->indexStride,
  input->base.semantic    = AK_INPUT_SEMANTIC_NORMAL;
  input->base.semanticRaw = "NORMAL";
  input->base.next        = &prim->input->base;

  ak_url_init(input,
              objp->srcurl,
              &input->base.source);

  prim->input = input;
  prim->inputCount++;
  prim->indexStride++;

  ak_free(prim->indices);
  prim->indices = inpIndices;
}

AK_EXPORT
void
ak_meshGenNormals(AkMesh * __restrict mesh) {
  AkHeap             *heap;
  AkMeshPrimitive    *prim;
  AkDataContext      *dctx;
  AkSourceFloatArray *arr,   *posarr;
  AkSource           *src,   *possrc;
  AkAccessor         *acc,   *posacc;
  AkObject           *data,  *posdata;
  char               *srcid, *srcurl;
  AkFloat            *pos_it;
  AkGenNormalStruct   objp;

  possrc = ak_mesh_pos_src(mesh);
  if (!possrc
      || !(posacc  = possrc->tcommon)
      || !(posdata = ak_getObjectByUrl(&posacc->source)))
    return;

  posarr = ak_objGet(posdata);

  heap   = ak_heap_getheap(ak_objFrom(mesh));
  srcid  = (char *)ak_id_gen(heap, NULL, NULL);
  srcurl = ak_id_urlstring(heap->allocator, srcid);
  pos_it = posarr->items;

  dctx = ak_data_new(ak_objFrom(mesh),
                     64,
                     sizeof(vec3),
                     ak_cmp_vec3);

  prim = mesh->primitive;

  objp.dctx       = dctx;
  objp.pos_it     = pos_it;
  objp.pos_stride = posacc->stride;
  objp.srcurl     = srcurl;

  while (prim) {
    ak_meshPrimGenNormals(prim, &objp);
    prim = prim->next;
  }

  src = ak_mesh_src_for_ext(heap,
                            mesh,
                            srcid,
                            AK_INPUT_SEMANTIC_NORMAL,
                            dctx->itemcount);

  acc  = src->tcommon;
  data = ak_getObjectByUrl(&acc->source);
  arr  = ak_objGet(data);

  (void)ak_data_join(dctx, arr->items);

  ak_free(dctx);
  heap->allocator->free(srcurl);

  ak_mesh_fix_indices(heap, mesh);
}
