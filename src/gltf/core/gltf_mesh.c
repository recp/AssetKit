/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_mesh.h"
#include "gltf_enums.h"
#include "gltf_accessor.h"
#include "gltf_buffer.h"
#include "../../ak_accessor.h"

/*
  glTF meshes      -> AkGeometry > AkMesh
  glTF primitives  -> AkMeshPrimitive
 */

void _assetkit_hide
gltf_meshes(AkGLTFState * __restrict gst) {
  AkHeap     *heap;
  AkDoc      *doc;
  json_t     *jmeshes, *jaccessors;
  AkLibItem  *lib;
  AkGeometry *last_geom;
  int32_t     jmeshCount, i;

  heap        = gst->heap;
  doc         = gst->doc;
  lib         = ak_heap_calloc(heap, doc, sizeof(*lib));
  last_geom   = NULL;

  jmeshes    = json_object_get(gst->root, _s_gltf_meshes);
  jmeshCount = (int32_t)json_array_size(jmeshes);
  jaccessors = json_object_get(gst->root, _s_gltf_accessors);

  for (i = jmeshCount - 1; i >= 0; i--) {
    AkGeometry *geom;
    AkObject   *last_mesh;

    json_t     *jmesh, *jprims;
    int32_t     jprimCount, j;

    geom       = ak_heap_calloc(heap, lib, sizeof(*geom));

    jmesh      = json_array_get(jmeshes, i);
    jprims     = json_object_get(jmesh, _s_gltf_primitives);
    jprimCount = (int32_t)json_array_size(jprims);
    last_mesh  = NULL;

    for (j = 0; j < jprimCount; j++) {
      AkObject        *meshObj;
      AkMesh          *mesh;
      AkMeshPrimitive *prim;
      AkInput         *last_inp, *last_vert_inp, *vertInp;
      json_t          *jprim, *jattribs, *jval, *jindices;
      const char      *jkey;

      jprim   = json_array_get(jprims, i);
      meshObj = ak_objAlloc(heap,
                            geom,
                            sizeof(AkMesh),
                            AK_GEOMETRY_TYPE_MESH,
                            true);
      mesh       = ak_objGet(meshObj);
      mesh->geom = geom;

      prim  = gltf_allocPrim(heap,
                             meshObj,
                             json_int32_def(jprim, _s_gltf_mode,  4));

      mesh->vertices = ak_heap_calloc(heap,
                                      prim,
                                      sizeof(*prim->vertices));

      vertInp = ak_heap_calloc(heap, prim, sizeof(*vertInp));
      vertInp->base.semantic    = AK_INPUT_SEMANTIC_VERTEX;
      vertInp->base.semanticRaw = ak_heap_strdup(heap,
                                                 vertInp,
                                                 _s_gltf_VERTEX);
      vertInp->base.source.ptr = mesh->vertices;
      prim->input = last_inp = vertInp;
      prim->inputCount = 1;

      last_vert_inp = NULL;
      jattribs      = json_object_get(jprim, _s_gltf_attributes);
      json_object_foreach(jattribs, jkey, jval) {
        AkInput    *inp;
        const char *semantic;
        AkSource   *source;
        json_t     *jacc;

        inp      = ak_heap_calloc(heap, prim, sizeof(*inp));
        semantic = strchr(jkey, '_');

        if (!semantic) {
          inp->base.semanticRaw = ak_heap_strdup(heap,
                                                 inp,
                                                 jkey);
        }

        /* ARRAYs e.g. TEXTURE_0, TEXTURE_1 */
        else {
          inp->base.semanticRaw = ak_heap_strndup(heap,
                                                  inp,
                                                  jkey,
                                                  semantic - jkey);
        }

        source = ak_heap_calloc(heap, prim, sizeof(*source));
        jacc   = json_array_get(jaccessors, json_integer_value(jval));

        source->tcommon      = gltf_accessor(gst, source, jacc);
        inp->base.semantic   = gltf_enumInputSemantic(inp->base.semanticRaw);
        inp->base.source.ptr = source;

        /* TODO: check morph targets to insert them to <vertices> */
        if (inp->base.semantic == AK_INPUT_SEMANTIC_POSITION) {
          if (last_vert_inp)
            last_vert_inp->base.next = &inp->base;
          else
            mesh->vertices->input = &inp->base;

          last_vert_inp = inp;
          mesh->vertices->inputCount++;
        } else {
          if (last_inp)
            last_inp->base.next = &inp->base;
          else
            prim->input = inp;

          last_inp = inp;
          prim->inputCount++;
        }
      }

      if ((jindices = json_object_get(jprim, _s_gltf_indices))) {
        AkAccessor *indicesAcc;
        AkBuffer   *indicesBuff;
        json_t     *jacc;
        size_t      count, i, itemSize;

        jacc        = json_array_get(jaccessors, json_integer_value(jindices));
        indicesAcc  = gltf_accessor(gst, prim, jacc);
        indicesBuff = indicesAcc->source.ptr;
        itemSize    = indicesAcc->type->size;
        count       = indicesAcc->count;

        if (indicesBuff) {
          AkUIntArray *indices;
          AkUInt      *it1;
          char        *it2;

          indices = ak_heap_calloc(heap,
                                   prim,
                                   sizeof(*indices)
                                     + sizeof(AkUInt) * count);
          indices->count = count;
          it1 = indices->items;
          it2 = indicesBuff->data;

          /* we cannot use memcpy here, because we will promote short, byte
             type to int32 (for now)
           */
          for (i = 0; i < count; i++) {
            memcpy(&it1[i], it2 + itemSize * i, itemSize);
          }

          prim->indices     = indices;
          prim->indexStride = 1;
        }
      }

      prim->vertices    = mesh->vertices;
      prim->mesh        = mesh;
      mesh->primitive   = prim;

      mesh->primitiveCount = 1;
      mesh->geom = geom;

      if (last_mesh)
        last_mesh->next = meshObj;
      else
        geom->gdata = meshObj;

      last_mesh = meshObj;
    }

    if (last_geom)
      last_geom->next = geom;
    else
      lib->chld = geom;

    last_geom = geom;
    lib->count++;
  }

  doc->lib.geometries = lib;
}

AkMeshPrimitive* _assetkit_hide
gltf_allocPrim(AkHeap * __restrict heap,
               void   * __restrict memParent,
               int                 mode) {
  switch (mode) {
    case 0: {
      AkMeshPrimitive *prim;
      prim = ak_heap_calloc(heap, memParent, sizeof(*prim));
      prim->type = AK_MESH_PRIMITIVE_TYPE_POINTS;
      return prim;
      break;
    }
    case 1: {
      AkLines *lines;
      lines = ak_heap_calloc(heap, memParent, sizeof(*lines));
      lines->base.type = AK_MESH_PRIMITIVE_TYPE_LINES;
      lines->mode      = AK_LINE_MODE_LINES;
      return &lines->base;
      break;
    }
    case 2: {
      AkLines *lines;
      lines = ak_heap_calloc(heap, memParent, sizeof(*lines));
      lines->base.type = AK_MESH_PRIMITIVE_TYPE_LINES;
      lines->mode       = AK_LINE_MODE_LINE_LOOP;
      return &lines->base;
      break;
    }
    case 3: {
      AkLines *lines;
      lines = ak_heap_calloc(heap, memParent, sizeof(*lines));
      lines->base.type = AK_MESH_PRIMITIVE_TYPE_LINES;
      lines->mode      = AK_LINE_MODE_LINE_STRIP;
      return &lines->base;
      break;
    }
    case 4: {
      AkTriangles *tri;
      tri = ak_heap_calloc(heap, memParent, sizeof(*tri));
      tri->base.type = AK_MESH_PRIMITIVE_TYPE_TRIANGLES;
      tri->mode      = AK_TRIANGLE_MODE_TRIANGLES;
      return &tri->base;
      break;
    }
    case 5: {
      AkTriangles *tri;
      tri = ak_heap_calloc(heap, memParent, sizeof(*tri));
      tri->base.type = AK_MESH_PRIMITIVE_TYPE_TRIANGLES;
      tri->mode  = AK_TRIANGLE_MODE_TRIANGLE_STRIP;
      return &tri->base;
      break;
    }
    case 6: {
      AkTriangles *tri;
      tri = ak_heap_calloc(heap, memParent, sizeof(*tri));
      tri->base.type = AK_MESH_PRIMITIVE_TYPE_TRIANGLES;
      tri->mode  = AK_TRIANGLE_MODE_TRIANGLE_FAN;
      return &tri->base;
      break;
    }
  }

  return NULL;
}

