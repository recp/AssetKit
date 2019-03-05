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
#include "../../accessor.h"

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
  size_t      jmeshCount, i;

  heap       = gst->heap;
  doc        = gst->doc;
  lib        = ak_heap_calloc(heap, doc, sizeof(*lib));
  last_geom  = NULL;

  jmeshes    = json_object_get(gst->root, _s_gltf_meshes);
  jmeshCount = (int32_t)json_array_size(jmeshes);
  jaccessors = json_object_get(gst->root, _s_gltf_accessors);

  for (i = 0; i < jmeshCount; i++) {
    AkGeometry      *geom;
    AkMesh          *mesh;
    AkObject        *meshObj;
    AkMeshPrimitive *last_prim;
    json_t          *jmesh, *jprims, *jmat;
    int32_t          jprimCount, j;

    geom              = ak_heap_calloc(heap, lib, sizeof(*geom));
    geom->materialMap = ak_map_new(ak_cmp_str);

    /* destroy heap with this object */
    ak_setAttachedHeap(geom, geom->materialMap->heap);

    meshObj    = ak_objAlloc(heap,
                             geom,
                             sizeof(AkMesh),
                             AK_GEOMETRY_TYPE_MESH,
                             true);
    geom->gdata = meshObj;
    mesh        = ak_objGet(meshObj);
    mesh->geom  = geom;
    mesh->primitiveCount = 0;

    jmesh      = json_array_get(jmeshes, i);
    jprims     = json_object_get(jmesh, _s_gltf_primitives);
    jprimCount = (int32_t)json_array_size(jprims);
    last_prim  = NULL;

    for (j = 0; j < jprimCount; j++) {
      AkMeshPrimitive *prim;
      AkInput         *last_inp;
      json_t          *jprim, *jattribs, *jval, *jindices;
      const char      *jkey;

      jprim = json_array_get(jprims, j);
      prim  = gltf_allocPrim(heap,
                             meshObj,
                             jsn_i32_def(jprim, _s_gltf_mode, 4));

      prim->input      = last_inp = NULL;
      prim->inputCount = 0;
      prim->mesh       = mesh;

      jattribs = json_object_get(jprim, _s_gltf_attributes);
      json_object_foreach(jattribs, jkey, jval) {
        AkInput    *inp;
        const char *semantic;
        AkSource   *source;
        json_t     *jacc;

        inp      = ak_heap_calloc(heap, prim, sizeof(*inp));
        semantic = strchr(jkey, '_');

        if (!semantic) {
          inp->semanticRaw = ak_heap_strdup(heap, inp, jkey);
        }

        /* ARRAYs e.g. TEXTURE_0, TEXTURE_1 */
        else {
          inp->semanticRaw = ak_heap_strndup(heap,
                                             inp,
                                             jkey,
                                             semantic - jkey);
          if (strlen(semantic) > 1) /* default is 0 with calloc */
            inp->set = (uint32_t)strtol(semantic + 1, NULL, 10);
        }

        source = ak_heap_calloc(heap, prim, sizeof(*source));
        jacc   = json_array_get(jaccessors, json_integer_value(jval));

        ak_setypeid(source, AKT_SOURCE);

        source->tcommon = gltf_accessor(gst, source, jacc);
        inp->semantic   = gltf_enumInputSemantic(inp->semanticRaw);
        inp->source.ptr = source;

        if (inp->semantic == AK_INPUT_SEMANTIC_POSITION)
          prim->pos = inp;

        if (last_inp)
          last_inp->next = inp;
        else
          prim->input = inp;

        last_inp = inp;
        prim->inputCount++;
      }

      if ((jindices = json_object_get(jprim, _s_gltf_indices))) {
        AkAccessor *indicesAcc;
        AkBuffer   *indicesBuff;
        json_t     *jacc;
        size_t      count, k, itemSize;

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
                                   sizeof(*indices) + sizeof(AkUInt) * count);
          indices->count = count;
          it1 = indices->items;
          it2 = (char *)indicesBuff->data + indicesAcc->byteOffset;

          /* we cannot use memcpy here, because we will promote short, byte
             type to int32 (for now)
           */
          for (k = 0; k < count; k++) {
            memcpy(&it1[k], it2 + itemSize * k, itemSize);
          }

          prim->indices     = indices;
          prim->indexStride = 1;
        }
      }

      /* material semantic */
      if ((jmat = json_object_get(jprim, _s_gltf_material))) {
        AkMaterial *mat;
        int32_t     matIndex;

        matIndex = (int32_t)json_integer_value(jmat);
        mat      = gst->doc->lib.materials->chld;
        while (matIndex > 0) {
          mat = mat->next;
          matIndex--;
        }

        /* we can use material id as semantic */
        if (mat) {
          char  *materialId, *symbol;
          size_t len;

          materialId  = ak_mem_getId(mat);
          len         = strlen(materialId) + ak_digitsize(j) + 1;
          symbol      = ak_heap_alloc(heap, prim, len + 1);
          symbol[len] = '\0';
          sprintf(symbol, "%s-%d", materialId, j);

          ak_meshSetMaterial(prim, (prim->material = symbol));
        }
      }

      if (last_prim)
        last_prim->next = prim;
      else
        mesh->primitive = prim;
      last_prim = prim;

      mesh->primitiveCount++;
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
    }
    case 1: {
      AkLines *lines;
      lines = ak_heap_calloc(heap, memParent, sizeof(*lines));
      lines->base.type = AK_MESH_PRIMITIVE_TYPE_LINES;
      lines->mode      = AK_LINE_MODE_LINES;
      return &lines->base;
    }
    case 2: {
      AkLines *lines;
      lines = ak_heap_calloc(heap, memParent, sizeof(*lines));
      lines->base.type = AK_MESH_PRIMITIVE_TYPE_LINES;
      lines->mode       = AK_LINE_MODE_LINE_LOOP;
      return &lines->base;
    }
    case 3: {
      AkLines *lines;
      lines = ak_heap_calloc(heap, memParent, sizeof(*lines));
      lines->base.type = AK_MESH_PRIMITIVE_TYPE_LINES;
      lines->mode      = AK_LINE_MODE_LINE_STRIP;
      return &lines->base;
    }
    case 4: {
      AkTriangles *tri;
      tri = ak_heap_calloc(heap, memParent, sizeof(*tri));
      tri->base.type = AK_MESH_PRIMITIVE_TYPE_TRIANGLES;
      tri->mode      = AK_TRIANGLE_MODE_TRIANGLES;
      return &tri->base;
    }
    case 5: {
      AkTriangles *tri;
      tri = ak_heap_calloc(heap, memParent, sizeof(*tri));
      tri->base.type = AK_MESH_PRIMITIVE_TYPE_TRIANGLES;
      tri->mode  = AK_TRIANGLE_MODE_TRIANGLE_STRIP;
      return &tri->base;
    }
    case 6: {
      AkTriangles *tri;
      tri = ak_heap_calloc(heap, memParent, sizeof(*tri));
      tri->base.type = AK_MESH_PRIMITIVE_TYPE_TRIANGLES;
      tri->mode  = AK_TRIANGLE_MODE_TRIANGLE_FAN;
      return &tri->base;
    }
  }

  return NULL;
}
