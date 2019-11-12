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
gltf_meshes(json_t * __restrict jmesh,
            void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  AkLibItem          *lib, *lib_morph;
  AkController       *last_morph;
  AkGeometry         *last_geom;
  const json_array_t *jmeshes;
  const json_t       *jmeshVal;

  if (!(jmeshes = json_array(jmesh)))
    return;

  gst        = userdata;
  heap       = gst->heap;
  doc        = gst->doc;
  lib        = ak_heap_calloc(heap, doc, sizeof(*lib));
  lib_morph  = doc->lib.controllers;
  last_geom  = NULL;
  last_morph = NULL;

  jmesh = jmeshes->base.value;
  while (jmesh) {
    AkGeometry      *geom;
    AkMesh          *mesh;
    AkObject        *meshObj;
    AkMeshPrimitive *last_prim;

    geom              = ak_heap_calloc(heap, lib, sizeof(*geom));
    geom->materialMap = ak_map_new(ak_cmp_str);

    /* destroy heap with this object */
    ak_setAttachedHeap(geom, geom->materialMap->heap);

    meshObj = ak_objAlloc(heap,
                          geom,
                          sizeof(AkMesh),
                          AK_GEOMETRY_TYPE_MESH,
                          true);
    geom->gdata          = meshObj;
    mesh                 = ak_objGet(meshObj);
    mesh->geom           = geom;
    mesh->primitiveCount = 0;

    last_prim = NULL;
    jmeshVal  = jmesh->value;

    while (jmeshVal) {
      if (json_key_eq(jmeshVal, _s_gltf_primitives)
          && json_is_array(jmeshVal)) {
        json_t *jprim;

        jprim = jmeshVal->value;
        while (jprim) {
          AkMeshPrimitive *prim;
          AkInput         *last_inp;
          json_t          *jprimVal;

          prim  = gltf_allocPrim(heap,
                                 meshObj,
                                 json_int32(json_get(jprim, _s_gltf_mode), 4));

          prim->input      = last_inp = NULL;
          prim->inputCount = 0;
          prim->mesh       = mesh;

          jprimVal = jprim->value;

          while (jprimVal) {
            if (json_key_eq(jprimVal, _s_gltf_attributes)) {
              json_t *jattrib;

              /* attributes */
              jattrib = jprimVal->value;
              while (jattrib) {
                AkInput    *inp;
                const char *semantic;
                AkSource   *source;
                AkAccessor *acc;

                inp      = ak_heap_calloc(heap, prim, sizeof(*inp));
                semantic = memchr(jattrib->key, '_', jattrib->keySize);

                if (!semantic) {
                  inp->semanticRaw = ak_heap_strndup(heap,
                                                     inp,
                                                     jattrib->key,
                                                     jattrib->keySize);
                }

                /* ARRAYs e.g. TEXTURE_0, TEXTURE_1 */
                else {
                  inp->semanticRaw = ak_heap_strndup(heap,
                                                     inp,
                                                     jattrib->key,
                                                     semantic - jattrib->key);
                  if (strlen(semantic) > 1) /* default is 0 with calloc */
                    inp->set = (uint32_t)strtol(semantic + 1, NULL, 10);
                }

                acc    = flist_sp_at(&doc->lib.accessors,
                                     json_int32(jattrib, -1));
                source = ak_heap_calloc(heap, prim, sizeof(*source));

                ak_setypeid(source, AKT_SOURCE);

                source->tcommon = acc;
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

                jattrib = jattrib->next;
              } /* jprimAttrib */
            } else if (json_key_eq(jprimVal, _s_gltf_indices)) {
              AkAccessor   *acc;
              AkBufferView *buffView;
              AkBuffer     *indicesBuff;
              AkUIntArray  *indices;
              AkUInt       *it1;
              char         *it2;
              size_t        count, k, itemSize;

              if (!(acc = flist_sp_at(&doc->lib.accessors,
                                      json_int32(jprimVal, -1)))
                  || !(buffView = acc->bufferView)
                  || !(indicesBuff = buffView->buffer))
                goto prim_next;

              itemSize = acc->type->size;
              count    = acc->count;
              indices  = ak_heap_calloc(heap,
                                        prim,
                                        sizeof(*indices)
                                        + sizeof(AkUInt) * count);
              indices->count = count;
              it1            = indices->items;
              it2            = (char *)indicesBuff->data + acc->byteOffset;

              /* we cannot use memcpy here, because we will promote short, byte
                 type to int32 (for now)
               */
              for (k = 0; k < count; k++) {
                memcpy(&it1[k], it2 + itemSize * k, itemSize);
              }

              prim->indices     = indices;
              prim->indexStride = 1;
            } else if (json_key_eq(jprimVal, _s_gltf_material)) {
              AkMaterial *mat;
              int32_t     matIndex;

              matIndex = json_int32(jprimVal, -1);
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
                len         = strlen(materialId)
                              + ak_digitsize(mesh->primitiveCount) + 1;
                symbol      = ak_heap_alloc(heap, prim, len + 1);
                symbol[len] = '\0';
                sprintf(symbol, "%s-%d", materialId, mesh->primitiveCount);

                ak_meshSetMaterial(prim, (prim->material = symbol));
              }
            } else if (json_key_eq(jprimVal, _s_gltf_targets)) {
//              if ((jtargets = json_object_get(jprim, _s_gltf_targets))) {
//                AkInput *last_targetinp;
//                size_t   jtargetsCount, j;
//
//
//                jtargetsCount  = json_array_size(jtargets);
//
//                for (j = 0; j < jtargetsCount; j++) {
//                  json_t *jtarget;
//
//                  jtarget        = json_array_get(jtargets, j);
//                  last_targetinp = NULL;
//
//                  json_object_foreach(jtarget, jkey, jval) {
//                    AkInput    *inp;
//                    const char *semantic;
//                    AkSource   *source;
//                    json_t     *jacc;
//
//                    inp      = ak_heap_calloc(heap, prim, sizeof(*inp));
//                    semantic = strchr(jkey, '_');
//
//                    if (!semantic) {
//                      inp->semanticRaw = ak_heap_strdup(heap, inp, jkey);
//                    }
//
//                    /* ARRAYs e.g. TEXTURE_0, TEXTURE_1 */
//                    else {
//                      inp->semanticRaw = ak_heap_strndup(heap,
//                                                         inp,
//                                                         jkey,
//                                                         semantic - jkey);
//                      if (strlen(semantic) > 1) /* default is 0 with calloc */
//                        inp->set = (uint32_t)strtol(semantic + 1, NULL, 10);
//                    }
//
//                    source = ak_heap_calloc(heap, prim, sizeof(*source));
//                    jacc   = json_array_get(jaccessors, json_integer_value(jval));
//
//                    ak_setypeid(source, AKT_SOURCE);
//
//                    source->tcommon = gltf_accessor(gst, source, jacc);
//                    inp->semantic   = gltf_enumInputSemantic(inp->semanticRaw);
//                    inp->source.ptr = source;
//
//                    if (last_targetinp)
//                      last_targetinp->next = inp;
//                    else
//                      last_targetinp = inp;
//                  }
//
//                  /* there must least one input to create morph object */
//                  if (last_targetinp) {
//                    AkController *ctlr;
//                    AkObject     *obj;
//                    AkMorph      *morph;
//                    AkTargets    *targets;
//                    char          morphid[32];
//
//                    ctlr = ak_heap_calloc(heap, lib_morph, sizeof(*ctlr));
//                    ak_setypeid(ctlr, AKT_CONTROLLER);
//
//                    ctlr->data = obj = ak_objAlloc(heap,
//                                                   ctlr,
//                                                   sizeof(*morph),
//                                                   AK_CONTROLLER_MORPH,
//                                                   true);
//                    morph   = ak_objGet(obj);
//                    targets = ak_heap_calloc(gst->heap, ctlr, sizeof(*targets));
//
//                    /* sets id as morph-meshindex-targetindex */
//                    sprintf(morphid, "%s-%zu-%zu", _s_gltf_morph, i, j);
//                    ak_heap_setId(heap,
//                                  ak__alignof(ctlr),
//                                  ak_heap_strdup(gst->heap, ctlr, morphid));
//
//                    rb_insert(gst->meshTargets, mesh, ctlr);
//
//                    morph->targets = targets;
//
//                    if (last_morph)
//                      last_morph->next = ctlr;
//                    else
//                      lib_morph->chld = ctlr;
//
//                    last_morph = ctlr;
//                    lib_morph->count++;
//                  }
//                }
//              }
            }

            jprimVal = jprimVal->next;
          }

          if (last_prim)
            last_prim->next = prim;
          else
            mesh->primitive = prim;
          last_prim = prim;

          mesh->primitiveCount++;

        prim_next:
          jprim = jprim->next;
        }
      } else if (json_key_eq(jmeshVal, _s_gltf_weights)) {
        /* TODO: */
      } else if (json_key_eq(jmeshVal, _s_gltf_name)) {
        mesh->name = json_strdup(jmeshVal, heap, meshObj);
      }

      jmeshVal = jmeshVal->next;
    }

    if (last_geom)
      last_geom->next = geom;
    else
      lib->chld = geom;

    last_geom = geom;
    lib->count++;

    jmesh = jmesh->next;
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
