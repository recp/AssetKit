/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"

/* TODO: use mutex */

AK_EXPORT
void
ak_changeCoordSysMesh(AkMesh * __restrict mesh,
                      AkCoordSys * newCoordSys) {
  AkMeshPrimitive *primi;
  AkDoc           *doc;
  AkHeap          *heap;
  AkInput         *input;
  AkMap           *map;
  AkMapItem       *mapi;

  heap = ak_heap_getheap(mesh->geom);
  doc  = heap->data;
  map  = ak_map_new(NULL);

  /* find sources to update */
  if (!(primi = mesh->primitive))
    return;

  while (primi) {
    input = primi->input;
    while (input) {
      /* TODO: other semantics which are depend on coord sys */
      if (input->semantic == AK_INPUT_SEMANTIC_POSITION
          || input->semantic == AK_INPUT_SEMANTIC_NORMAL) {
        if (!input->accessor) {
          input = input->next;
          continue;
        }
        
        ak_map_addptr(map, input->accessor);
      }
      input = input->next;
    }
    primi = primi->next;
  }

  mapi = map->root;
  while (mapi) {
    AkAccessor *acci;
    AkBuffer   *buffi;

    acci  = ak_getId(mapi);
    buffi = acci->buffer;
    if (!buffi) {
      mapi = mapi->next;
      continue;
    }

    /* TODO: INT, DOUBLE.. */
    if (acci->componentType == AKT_FLOAT) {
      ak_coordCvtVectors(doc->coordSys,
                         buffi->data,
                         buffi->length / acci->type->size,
                         newCoordSys);
    }
    mapi = mapi->next;
  }

  ak_map_destroy(map);
}
