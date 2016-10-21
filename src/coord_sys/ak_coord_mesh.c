/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"

AK_INLINE
_assetkit_hide
void
ak_coordApplyNewCoordSysToInput(AkDoc * doc,
                                AkInput * input,
                                AkCoordSys * newCoordSys) {
  /* stride count */
  while (input) {
    if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX) {
      AkVertices    *verts;
      AkInputBasic  *vib;
      AkSource      *vs;
      AkFloatArrayN *va;

      verts = ak_getObjectByUrl(doc, input->base.source);
      vib  = verts->input;
      while (vib) {
        vs = ak_getObjectByUrl(doc, vib->source);

        /* TODO: INT, DOUBLE.. */
        if (vs->data->type == AK_SOURCE_ARRAY_TYPE_FLOAT) {
          va = ak_objGet(vs->data);

          ak_coordCvtSequenceN(doc->coordSys,
                               va->items,
                               va->count,
                               newCoordSys);
        }

        vib = vib->next;
      } /* while */
    }

    input = (AkInput *)input->base.next;
  }
}

AK_EXPORT
void
ak_changeCoordSysMesh(AkMesh * __restrict mesh,
                      AkCoordSys * newCoordSys) {
  AkDoc    *doc;
  AkHeap   *heap;
  AkObject *primitive;

  heap = ak_heap_getheap(mesh->vertices);
  doc  = heap->data;

  primitive = mesh->gprimitive;

  switch ((AkMeshPrimitiveType)primitive->type) {
    case AK_MESH_PRIMITIVE_TYPE_POLYGONS: {
      AkPolygon *polygon;
      polygon = ak_objGet(primitive);
      while (polygon) {
        ak_coordApplyNewCoordSysToInput(doc,
                                        polygon->input,
                                        newCoordSys);
        polygon = polygon->next;
      }
      
      break;
    }

    case AK_MESH_PRIMITIVE_TYPE_TRIANGLES: {
      AkTriangles *triangles;
      triangles = ak_objGet(primitive);
      while (triangles) {
        ak_coordApplyNewCoordSysToInput(doc,
                                        triangles->input,
                                        newCoordSys);
        triangles = triangles->next;
      }

      break;
    }

    case AK_MESH_PRIMITIVE_TYPE_LINES: {
      AkLines *lines;
      lines = ak_objGet(primitive);
      while (lines) {
        ak_coordApplyNewCoordSysToInput(doc,
                                        lines->input,
                                        newCoordSys);
        lines = lines->next;
      }

      break;
    }
  }
}
