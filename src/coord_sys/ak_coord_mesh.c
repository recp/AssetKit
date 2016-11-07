/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"

AK_EXPORT
void
ak_changeCoordSysMesh(AkMesh * __restrict mesh,
                      AkCoordSys * newCoordSys) {
  AkMeshPrimitive *primitive;
  AkDoc   *doc;
  AkHeap  *heap;
  AkInput *input;

  heap = ak_heap_getheap(mesh->vertices);
  doc  = heap->data;

  primitive = mesh->primitive;
  while (primitive) {
    input = primitive->input;
    while (input) {
      if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX) {
        AkVertices    *verts;
        AkInputBasic  *vib;
        AkSource      *vs;
        AkFloatArrayN *va;

        verts = ak_getObjectByUrl(&input->base.source);
        vib   = verts->input;

        while (vib) {
          vs = ak_getObjectByUrl(&vib->source);

          /* TODO: INT, DOUBLE.. */
          if (vs->data->type == AK_SOURCE_ARRAY_TYPE_FLOAT) {
            va = ak_objGet(vs->data);

            ak_coordCvtVectors(doc->coordSys,
                               va->items,
                               va->count,
                               newCoordSys);
          }

          vib = vib->next;
        } /* while */
      }
      
      input = (AkInput *)input->base.next;
    }
    primitive = primitive->next;
  }
}
