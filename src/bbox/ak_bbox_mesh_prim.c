/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_bbox.h"
#include "../mesh/ak_mesh_util.h"
#include <cglm.h>

void
ak_bbox_mesh_prim(struct AkMeshPrimitive * __restrict prim) {
  AkHeap             *heap;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkSourceFloatArray *posarray;
  AkFloat            *items;
  AkInputBasic       *inputb;
  AkAccessor         *acc;
  vec3                min, max;

  mesh     = prim->mesh;
  geom     = mesh->geom;
  inputb   = mesh->vertices->input;
  posarray = NULL;
  acc      = NULL;

  while (inputb) {
    if (inputb->semantic == AK_INPUT_SEMANTIC_POSITION) {
      AkSource *src;
      AkObject *data;

      src = ak_getObjectByUrl(&inputb->source);
      if (!src)
        break;

      acc = src->tcommon;
      if (!acc)
        break;

      data = ak_getObjectByUrl(&acc->source);
      if (data)
        posarray = ak_objGet(data);
      break;
    }
    inputb = inputb->next;
  }

  /* there is no positions */
  if (!posarray || !acc)
    return;

  items = posarray->items;

  glm_vec_broadcast(0.0f, min);
  glm_vec_broadcast(0.0f, max);

  /* we must walk through indices if exists because source may contain
     unrelated data and this will cause get wrong box
   */
  if (prim->indices) {
    AkUInt  *ind;
    size_t   i, icount;
    uint32_t st, vo;

    icount = prim->indices->count;
    vo     = ak_mesh_vertex_off(prim);
    st     = prim->indexStride;
    ind    = prim->indices->items;

    for (i = 0; i < icount; i += st) {
      float *vec;
      vec = items + ind[i + vo] * acc->stride;
      ak_bbox_pick(min, max, vec);
    }
  } else {
    size_t i;
    for (i = 0; i < acc->count; i++) {
      float *vec;
      vec = items + acc->offset + acc->stride * i;
      ak_bbox_pick(min, max, vec);
    }
  }

  heap = ak_heap_getheap(mesh->vertices->input);

  if (!prim->bbox)
    prim->bbox = ak_heap_calloc(heap,
                                prim,
                                sizeof(*prim->bbox));

  if (!mesh->bbox)
    mesh->bbox = ak_heap_calloc(heap,
                                prim,
                                sizeof(*prim->bbox));

  if (!geom->bbox)
    geom->bbox = ak_heap_calloc(heap,
                                prim,
                                sizeof(*prim->bbox));

  glm_vec_copy(min, prim->bbox->min);
  glm_vec_copy(max, prim->bbox->max);

  ak_bbox_pick_pbox(mesh->bbox, prim->bbox);
  ak_bbox_pick_pbox(geom->bbox, mesh->bbox);
}
