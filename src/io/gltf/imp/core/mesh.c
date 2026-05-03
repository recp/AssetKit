/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mesh.h"
#include "enum.h"
#include "accessor.h"
#include "buffer.h"
#include "../../../../accessor.h"
#include "../../../common/util.h"

#include <ds/rb.h>

/*
  glTF meshes      -> AkGeometry > AkMesh
  glTF primitives  -> AkMeshPrimitive
 */

AK_HIDE
void
gltf_meshes(json_t * __restrict jmesh,
            void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  AkLibrary          *lib;
  const json_array_t *jmeshes;
  const json_t       *jmeshVal;

  if (!(jmeshes = json_array(jmesh)))
    return;

  gst        = userdata;
  heap       = gst->heap;
  doc        = gst->doc;
  lib        = ak_heap_calloc(heap, doc, sizeof(*lib));

  jmesh = jmeshes->base.value;
  while (jmesh) {
    AkGeometry      *geom;
    AkMesh          *mesh;
    AkObject        *meshObj;
    uint32_t         mode;

    /* mesh-level morph state — built lazily on first primitive that has
       targets, then re-used by subsequent primitives so that all primitives
       of the mesh share one AkMorph (per glTF spec, weights are mesh-level). */
    AkMorph         *meshMorph        = NULL;
    AkMorphTarget  **meshTargetArr    = NULL;
    uint32_t         meshTargetCnt    = 0;

    /* mesh.extras.targetNames (blend shape names) — parsed independently of
       primitive order; attached to meshMorph after all keys are processed. */
    const char     **morphTargetNames = NULL;
    uint32_t         morphTargetNamesN = 0;

    mesh                 = ak_allocMesh(heap, lib, &geom);
    meshObj              = ak_objFrom(mesh);
    mesh->primitiveCount = 0;

    jmeshVal = jmesh->value;
    while (jmeshVal) {
      if (json_key_eq(jmeshVal, _s_gltf_primitives)
          && json_is_array(jmeshVal)) {
        json_t *jprim;

        jprim = jmeshVal->value;
        while (jprim) {
          AkMeshPrimitive *prim;
          json_t          *jprimVal;

          mode = json_int32(json_get(jprim, _s_gltf_mode), 4);
          prim = gltf_allocPrim(heap, meshObj, mode);

          prim->input      = NULL;
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

                inp      = ak_heap_calloc(heap, prim, sizeof(*inp));
                semantic = memchr(jattrib->key, '_', jattrib->keysize);

                if (!semantic) {
                  inp->semanticRaw = ak_heap_strndup(heap,
                                                     inp,
                                                     jattrib->key,
                                                     jattrib->keysize);
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

                inp->semantic = gltf_enumInputSemantic(inp->semanticRaw);
                inp->accessor = flist_sp_at(&doc->lib.accessors,
                                            json_int32(jattrib, -1));

                ak_retain(inp->accessor);

                if (inp->semantic == AK_INPUT_POSITION)
                  prim->pos = inp;

                inp->next   = prim->input;
                prim->input = inp;
                prim->inputCount++;

                jattrib = jattrib->next;
              } /* jprimAttrib */
            } else if (json_key_eq(jprimVal, _s_gltf_indices)) {
              AkAccessor   *acc;
              AkBuffer     *indicesBuff;
              AkUIntArray  *indices;
              AkUInt       *it1;
              char         *it2;
              size_t        count, k, itemSize;

              if (!(acc = flist_sp_at(&doc->lib.accessors,
                                      json_int32(jprimVal, -1)))
                  || !(indicesBuff = acc->buffer))
                goto prim_next;

              itemSize = acc->bytesPerComponent;
              count    = acc->count;
              indices  = ak_heap_calloc(heap,
                                        prim,
                                        sizeof(*indices)
                                        + sizeof(AkUInt) * count);
              indices->count = count;
              it1            = indices->items;
              it2            = ((char *)indicesBuff->data) + acc->byteOffset;

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
              GETCHILD(gst->doc->lib.materials->chld, mat, matIndex);
       
              if (mat) { prim->material = mat;                  }
              else     { prim->material = gst->defaultMaterial; }
            } else if (json_key_eq(jprimVal, _s_gltf_targets)) {
              json_array_t  *jtargets;
              json_t        *jtarget, *jattrib;
              AkMorphTarget *target;
              AkMorphable   *morphable;
              uint32_t       targetIdx;

              if (!(jtargets = json_array(jprimVal)))
                goto prmv_nxt;

              /* Lazy-init the mesh-level morph and pre-allocate one
                 AkMorphTarget per blend shape. All primitives of the mesh
                 share these targets — each adds one AkMorphable to each
                 target's chain. (glTF spec: every primitive in the mesh
                 has the same number of morph targets in the same order.) */
              if (!meshMorph) {
                AkMorphTarget *prev;
                uint32_t       i;

                meshMorph         = ak_heap_calloc(heap, doc, sizeof(*meshMorph));
                meshMorph->method = AK_MORPH_METHOD_ADDITIVE;
                meshTargetCnt     = (uint32_t)jtargets->count;
                meshTargetArr     = ak_heap_calloc(heap, meshMorph,
                                                   sizeof(*meshTargetArr) * meshTargetCnt);

                prev = NULL;
                for (i = 0; i < meshTargetCnt; i++) {
                  target           = ak_heap_calloc(heap, meshMorph, sizeof(*target));
                  meshTargetArr[i] = target;

                  if (prev) prev->next      = target;
                  else      meshMorph->target = target;
                  prev = target;
                  meshMorph->targetCount++;
                }
              }

              /* For this primitive, prepend one AkMorphable to each blend
                 shape's chain (one per target). The first primitive's
                 morphable is wrapped in an AkObject (carries the type tag
                 used by intr.c switch dispatch); subsequent primitives'
                 morphables are plain calloc'd and linked via .next.

                 The JSON parser walks the targets array in reverse source
                 order (an artifact of the reader's reverse-prepend
                 strategy). The pre-allocated meshTargetArr[] is in chain
                 (forward) order, so we mirror the index when filling: the
                 first jtarget we visit (source LAST) populates the last
                 chain slot, and the last jtarget visited (source FIRST)
                 populates chain[0] — i.e. meshMorph->target ends up =
                 source targets[0], matching glTF semantics where weight[i]
                 drives mesh.primitives[].targets[i]. Without this swap,
                 weight[0] animates the wrong blend shape and the morph
                 visually plays in reverse target order.

                 Primitive nodes below are linked with head-prepend. Prepending
                 morphables here keeps each target's morphable chain in the
                 same order as mesh->primitive, so inspect can walk both chains
                 lock-step for multi-primitive morphs. */
              jtarget   = jtargets->base.value;
              targetIdx = 0;

              while (jtarget && targetIdx < meshTargetCnt) {
                target = meshTargetArr[meshTargetCnt - 1 - targetIdx];

                if (!target->target) {
                  AkObject *targetObj;
                  targetObj      = ak_objAlloc(heap, target, sizeof(*morphable),
                                               AK_MORPHABLE_MORPHABLE, true);
                  morphable      = ak_objGet(targetObj);
                  target->target = targetObj;
                } else {
                  AkMorphable *head, *oldHead;

                  head    = ak_objGet(target->target);
                  oldHead = ak_heap_calloc(heap, target, sizeof(*oldHead));
                  memcpy(oldHead, head, sizeof(*oldHead));

                  memset(head, 0, sizeof(*head));
                  head->next = oldHead;
                  morphable  = head;
                }
                target->primitiveCount++;

                /* fill morphable->input from the target's attribute deltas.
                   IMPORTANT: do NOT touch prim->pos here — that's the base
                   primitive's POSITION binding; target POSITION is a delta. */
                jattrib = jtarget->value;
                while (jattrib) {
                  AkInput    *inp;
                  const char *semantic;

                  inp      = ak_heap_calloc(heap, prim, sizeof(*inp));
                  semantic = memchr(jattrib->key, '_', jattrib->keysize);

                  if (!semantic) {
                    inp->semanticRaw = ak_heap_strndup(heap,
                                                       inp,
                                                       jattrib->key,
                                                       jattrib->keysize);
                  } else {
                    /* indexed (e.g. TEXCOORD_0, COLOR_1) */
                    inp->semanticRaw = ak_heap_strndup(heap,
                                                       inp,
                                                       jattrib->key,
                                                       semantic - jattrib->key);
                    if (strlen(semantic) > 1)
                      inp->set = (uint32_t)strtol(semantic + 1, NULL, 10);
                  }

                  inp->semantic = gltf_enumInputSemantic(inp->semanticRaw);
                  inp->accessor = flist_sp_at(&doc->lib.accessors,
                                              json_int32(jattrib, -1));

                  ak_retain(inp->accessor);

                  inp->next        = morphable->input;
                  morphable->input = inp;
                  morphable->inputCount++;

                  jattrib = jattrib->next;
                } /* jattrib */

                jtarget = jtarget->next;
                targetIdx++;
              } /* jtarget */
            }

          prmv_nxt:
            jprimVal = jprimVal->next;
          }

          prim->next      = mesh->primitive;
          mesh->primitive = prim;
          mesh->primitiveCount++;

          prim->nPolygons = gltf_polyCount(prim, mode);

          if (!prim->material) {
            prim->material = gst->defaultMaterial;
          }
        prim_next:
          jprim = jprim->next;
        }
      } else if (json_key_eq(jmeshVal, _s_gltf_weights)) {
        AkFloatArray *weights;
        json_array_t *jarr;

        if ((jarr = json_array(jmeshVal))) {
          weights = ak_heap_alloc(heap,
                                  meshObj,
                                  sizeof(*weights)
                                   + sizeof(float) * jarr->count);
          /* Pass jmeshVal (the array NODE), not jmeshVal->value (its first
             child). json_array_float internally casts arg via json_array(). */
          json_array_float(weights->items,
                           jmeshVal,
                           0.0f,
                           jarr->count,
                           true);

          weights->count = jarr->count;
          mesh->weights  = weights;
        }
      } else if (json_key_eq(jmeshVal, _s_gltf_name)) {
        mesh->name = json_strdup(jmeshVal, heap, meshObj);
      } else if (json_key_eq(jmeshVal, _s_gltf_extras)) {
        /* glTF spec: blend shape names live in mesh.extras.targetNames as a
           string array of length morph.targetCount. Parse here regardless of
           whether the morph has been built yet — attached after primitives. */
        json_t       *jnames;
        json_array_t *jarr;

        if ((jnames = json_get(jmeshVal, "targetNames"))
            && (jarr = json_array(jnames))
            && jarr->count > 0) {
          json_t   *jname;
          uint32_t  i;

          morphTargetNamesN = (uint32_t)jarr->count;
          morphTargetNames  = ak_heap_calloc(heap, meshObj,
                                             sizeof(const char *) * morphTargetNamesN);

          /* JSON parser walks the names array in reverse source order;
             mirror the index so targetNames[i] aligns with mesh's
             primitives[].targets[i] (and morph weight slot i). */
          jname = jarr->base.value;
          i     = 0;
          while (jname && i < morphTargetNamesN) {
            morphTargetNames[morphTargetNamesN - 1 - i] =
              json_strdup(jname, heap, meshObj);
            i++;
            jname = jname->next;
          }
        }
      }

      jmeshVal = jmeshVal->next;
    }

    /* Register the mesh-level morph once all keys are processed, so that
       extras.targetNames (which can come after primitives in JSON order) is
       attached as well. Keyed by geometry so node.c finds it via meshTargets. */
    if (meshMorph) {
      if (morphTargetNames && morphTargetNamesN > 0) {
        meshMorph->targetNames = morphTargetNames;
      }
      if (doc->lib.morphs)
        meshMorph->base.next = &doc->lib.morphs->base;
      doc->lib.morphs = meshMorph;
      rb_insert(gst->meshTargets, geom, meshMorph);
    }

    /* Reversed */
    geom->base.next = lib->chld;
    lib->chld       = (void *)geom;

    lib->count++;

    jmesh = jmesh->next;
  }

  doc->lib.geometries = lib;
}

AK_HIDE
AkMeshPrimitive*
gltf_allocPrim(AkHeap * __restrict heap,
               void   * __restrict memParent,
               int                 mode) {
  switch (mode) {
    case 0: {
      AkMeshPrimitive *prim;
      prim       = ak_heap_calloc(heap, memParent, sizeof(*prim));
      prim->type = AK_PRIMITIVE_POINTS;
      return prim;
    }
    case 1: {
      AkLines *lines;
      lines            = ak_heap_calloc(heap, memParent, sizeof(*lines));
      lines->base.type = AK_PRIMITIVE_LINES;
      lines->mode      = AK_LINES;
      return &lines->base;
    }
    case 2: {
      AkLines *lines;
      lines            = ak_heap_calloc(heap, memParent, sizeof(*lines));
      lines->base.type = AK_PRIMITIVE_LINES;
      lines->mode      = AK_LINE_LOOP;
      return &lines->base;
    }
    case 3: {
      AkLines *lines;
      lines            = ak_heap_calloc(heap, memParent, sizeof(*lines));
      lines->base.type = AK_PRIMITIVE_LINES;
      lines->mode      = AK_LINE_STRIP;
      return &lines->base;
    }
    case 4: {
      AkTriangles *tri;
      tri            = ak_heap_calloc(heap, memParent, sizeof(*tri));
      tri->base.type = AK_PRIMITIVE_TRIANGLES;
      tri->mode      = AK_TRIANGLES;
      return &tri->base;
    }
    case 5: {
      AkTriangles *tri;
      tri            = ak_heap_calloc(heap, memParent, sizeof(*tri));
      tri->base.type = AK_PRIMITIVE_TRIANGLES;
      tri->mode      = AK_TRIANGLE_STRIP;
      return &tri->base;
    }
    case 6: {
      AkTriangles *tri;
      tri            = ak_heap_calloc(heap, memParent, sizeof(*tri));
      tri->base.type = AK_PRIMITIVE_TRIANGLES;
      tri->mode      = AK_TRIANGLE_FAN;
      return &tri->base;
    }
  }

  return NULL;
}

AK_HIDE
uint32_t
gltf_polyCount(AkMeshPrimitive *prim, uint32_t mode) {
  AkInput    *pos;
  AkAccessor *acc;
  uint32_t    n;

  if (prim->indices) {
    n = (uint32_t)prim->indices->count;
  } else if ((pos = prim->pos) && (acc = pos->accessor)) {
    n = (uint32_t)acc->count;
  } else {
    goto err;
  }

  switch (mode) {
    /* 0: points, 2: line loops */
    case 0:
    case 2: return n;
    /* 1: lines */
    case 1: return n * 0.5;
    /* 3: line strip */
    case 3: return n - 1;
    /* 4: triangles */
    case 4: return n / 3;
    /* 5: triangle strip, 6: triangle fan */
    case 5:
    case 6: return n - 2;
  }

err:
  return 0;
}
