/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "../id.h"
#include "../mem_common.h"
#include "../data.h"

#include "mesh_index.h"

#include <cglm/cglm.h>

_assetkit_hide
void
ak_meshPrimGenNormals(AkMeshPrimitive    * __restrict prim);

AK_EXPORT
bool
ak_meshPrimNeedsNormals(AkMeshPrimitive * __restrict prim) {
  AkAccessor   *acc;
  AkInput      *input;
  AkBuffer     *buff;
  bool          ret;

  ret   = true;
  input = prim->input;
  while (input) {
    if (input->semantic == AK_INPUT_SEMANTIC_NORMAL) {
      if (!(acc = input->accessor) || !(buff = acc->buffer))
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
  AkDoc         *doc;
  AkUIntArray   *inpIndices;
  AkFloat       *pos;
  AkUInt        *it, *it2;
  AkHeap        *heap;
  AkInput       *input, *nextInput;
  AkBuffer      *posBuff, *buff;
  AkAccessor    *posAcc, *acc;
  AkUInt         st, newst;
  AkInt          vo, pos_st;
  size_t         count;

  if ((prim->type != AK_MESH_PRIMITIVE_TYPE_TRIANGLES
       && prim->type != AK_MESH_PRIMITIVE_TYPE_POLYGONS)
      || !prim->pos
      || !(posAcc    = prim->pos->accessor)
      || !(posBuff   = ak_getObjectByUrl(&posAcc->source))
      || (vo = prim->pos->offset) == -1)
    return;

  dctx   = ak_data_new(prim, 64, sizeof(vec3), ak_cmp_vec3);
  heap   = ak_heap_getheap(prim);
  doc    = ak_heap_data(heap);
  pos    = posBuff->data;
  pos_st = posAcc->stride;

  if (!prim->indices || prim->indices->count == 0)
    return;

  it    = prim->indices->items + vo;
  st    = prim->indexStride;
  count = prim->indices->count / st;
  newst = st + 1;

  /* TODO: for now join this into existing indices,
           but in the future use separate to fix indices  */
  inpIndices = ak_heap_calloc(heap,
                              prim,
                              sizeof(*inpIndices)
                              + count * newst * sizeof(*it));
  inpIndices->count = count * newst;
  it2 = inpIndices->items;

  switch (prim->type) {
    case AK_MESH_PRIMITIVE_TYPE_POLYGONS: {
      AkPolygon *poly;
      AkUInt    *vc_it;
      float     *a, *b, *c;
      vec3       v1, v2, n;
      size_t     i, j, k, vc, ist;
      AkUInt     idx;

      poly = (AkPolygon *)prim;

      /* polygon ha triangulated */
      if (!poly->vcount)
        goto tri;

      vc_it = poly->vcount->items;

      for (i = k = 0; k < poly->vcount->count; k++) {
        vc = vc_it[k];

        /* TODO: normals for lines or points ? */
        if (vc < 3)
          continue;

        ist = i * st + vo;

        a = pos + it[ist]           * pos_st;
        b = pos + it[ist + st]      * pos_st;
        c = pos + it[ist + st + st] * pos_st;

        glm_vec3_sub(a, b, v1);
        glm_vec3_sub(b, c, v2);

        glm_vec3_cross(v1, v2, n);
        glm_vec3_normalize(n);

        idx = ak_data_append(dctx, n);

        for (j = i; j < i + vc; j++) {
          /* other inputs */
          memcpy(it2 + j * newst, it  + j * st, sizeof(*it) * st);

          /* normal */
          it2[j * newst + st] = idx;
        }

        i += vc;
      }
      break;
    }
    case AK_MESH_PRIMITIVE_TYPE_TRIANGLES: tri: {
      float *a, *b, *c;
      vec3   v1, v2, n;
      AkUInt i, j, idx, ist;

      for (i = 0; i < count; i += 3 /* 3: triangle */) {
        ist = i * st + vo;

        a = pos + it[ist]           * pos_st;
        b = pos + it[ist + st]      * pos_st;
        c = pos + it[ist + st + st] * pos_st;

        glm_vec3_sub(a, b, v1);
        glm_vec3_sub(b, c, v2);

        glm_vec3_cross(v1, v2, n);
        glm_vec3_normalize(n);

        idx = ak_data_append(dctx, n);

        for (j = i; j < i + 3; j++) {
          /* other inputs */
          memcpy(it2 + j * newst, it  + j * st, sizeof(*it) * st);

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

  acc = ak_heap_calloc(heap, doc, sizeof(*acc));
  ak_setypeid(acc, AKT_ACCESSOR);

  acc->stride        = 3;
  acc->count         = count;
  acc->componentType = AKT_FLOAT;
  acc->type          = ak_typeDesc(acc->componentType);
  acc->byteStride    = acc->stride * acc->type->size;
  acc->byteLength    = acc->count * acc->byteStride;
  
  buff               = ak_heap_calloc(heap, doc, sizeof(*buff));
  buff->data         = ak_heap_alloc(heap, buff, acc->byteLength);
  buff->length       = acc->byteLength;

  flist_sp_insert(&doc->lib.accessors, acc);
  flist_sp_insert(&doc->lib.buffers, buff);
  
  /* add input */
  input              = ak_heap_calloc(heap, prim, sizeof(*input));
  input->offset      = st;
  input->semantic    = AK_INPUT_SEMANTIC_NORMAL;
  input->semanticRaw = "NORMAL";

  nextInput = prim->input;
  while (nextInput->next)
    nextInput = nextInput->next;

  nextInput->next = input;

  input->accessor = acc;

  prim->inputCount++;
  prim->indexStride++;

  ak_free(prim->indices);
  prim->indices = inpIndices;

  (void)ak_data_join(dctx, buff->data);
  ak_free(dctx);
}

AK_EXPORT
void
ak_meshGenNormals(AkMesh * __restrict mesh) {
  AkMeshEditHelper *edith;
  AkMeshPrimitive  *prim;

  ak_meshBeginEdit(mesh);

  prim  = mesh->primitive;
  edith = mesh->edith;

  while (prim) {
    ak_meshPrimGenNormals(prim);

    if (!edith->skipFixIndices)
      ak_primFixIndices(mesh, prim);

    prim = prim->next;
  }

  ak_meshEndEdit(mesh);
}
