/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "../memory_common.h"

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
        AkSource *srci;

        /* only current document */
        if (input->source.doc != doc) {
          input = input->next;
          continue;
        }

        srci = ak_getObjectByUrl(&input->source);
        if (!srci || !srci->tcommon) {
          input = input->next;
          continue;
        }
        
        ak_map_addptr(map, srci);
      }
      input = input->next;
    }
    primi = primi->next;
  }

  mapi = map->root;
  while (mapi) {
    AkSource   *srci;
    AkAccessor *acci;
    AkBuffer   *buffi;

    srci = ak_getId(mapi);
    acci = srci->tcommon;
    buffi = ak_getObjectByUrl(&acci->source);
    if (!buffi) {
      mapi = mapi->next;
      continue;
    }

    /* TODO: INT, DOUBLE.. */
    if (acci->itemTypeId == AKT_FLOAT) {
      ak_coordCvtVectors(doc->coordSys,
                         buffi->data,
                         buffi->length / acci->type->size,
                         newCoordSys);
    }
    mapi = mapi->next;
  }

  ak_map_destroy(map);
}
