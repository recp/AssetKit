/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "../ak_map.h"

/* TODO: use mutex */

AK_EXPORT
void
ak_changeCoordSysMesh(AkMesh * __restrict mesh,
                      AkCoordSys * newCoordSys) {
  AkMeshPrimitive *primi;
  AkDoc           *doc;
  AkHeap          *heap;
  AkInput         *input;
  AkInputBasic    *inputb;
  AkMap           *map;
  AkMapItem       *mapi;

  heap = ak_heap_getheap(mesh->vertices);
  doc  = heap->data;
  map  = ak_map_new(NULL);

  /* find sources to update */
  inputb = mesh->vertices->input;
  while (inputb) {
    /* TODO: other semantics which are depend on coord sys */
    if (inputb->semantic == AK_INPUT_SEMANTIC_POSITION
        || inputb->semantic == AK_INPUT_SEMANTIC_NORMAL) {
      AkSource *srci;

      /* only current document */
      if (inputb->source.doc != doc) {
        inputb = inputb->next;
        continue;
      }

      srci = ak_getObjectByUrl(&inputb->source);
      if (!srci || !srci->techniqueCommon) {
        inputb = inputb->next;
        continue;
      }

      ak_map_addptr(map, srci);
    }
    
    inputb = inputb->next;
  }

  primi = mesh->primitive;
  while (primi) {
    input = primi->input;
    while (input) {
      if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX) {
        input = (AkInput *)input->base.next;
        continue;
      }

      /* TODO: other semantics which are depend on coord sys */
      if (input->base.semantic == AK_INPUT_SEMANTIC_POSITION
          || input->base.semantic == AK_INPUT_SEMANTIC_NORMAL) {
        AkSource *srci;

        /* only current document */
        if (input->base.source.doc != doc) {
          input = (AkInput *)input->base.next;
          continue;
        }

        srci = ak_getObjectByUrl(&input->base.source);
        if (!srci || !srci->techniqueCommon) {
          input = (AkInput *)input->base.next;
          continue;
        }
        
        ak_map_addptr(map, srci);
      }
      input = (AkInput *)input->base.next;
    }
    primi = primi->next;
  }

  mapi = map->root;
  while (mapi) {
    AkSource   *srci;
    AkAccessor *acci;
    AkObject   *datai;

    srci  = ak_getId(mapi);
    acci  = srci->techniqueCommon;
    datai = ak_getObjectByUrl(&acci->source);
    if (!datai) {
      mapi = mapi->next;
      continue;
    }

    /* TODO: INT, DOUBLE.. */
    if (datai->type == AK_SOURCE_ARRAY_TYPE_FLOAT) {
      AkSourceFloatArray *arri;

      arri = ak_objGet(datai);
      ak_coordCvtVectors(doc->coordSys,
                         arri->items,
                         arri->count,
                         newCoordSys);
    }
    mapi = mapi->next;
  }

  ak_map_destroy(map);
}
