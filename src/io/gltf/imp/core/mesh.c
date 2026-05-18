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
#include "ext.h"
#include "../extra.h"
#include "../../../../accessor.h"
#include "../../../common/util.h"

#include <ds/rb.h>
#include <stdint.h>

/*
  glTF meshes      -> AkGeometry > AkMesh
  glTF primitives  -> AkMeshPrimitive
 */

typedef struct AkGLTFPrimProps {
  json_t *mode;
  json_t *attributes;
  json_t *indices;
  json_t *material;
  json_t *targets;
  json_t *extensions;
  json_t *extras;
} AkGLTFPrimProps;

static inline
void
gltf_primProps(json_t          * __restrict jprim,
               AkGLTFPrimProps * __restrict props) {
  json_t *it;
  char    first;

  if (!jprim || jprim->type != JSON_OBJECT)
    return;

  for (it = jprim->value; it; it = it->next) {
    if (!it->key)
      continue;

    first = it->key[0];

    switch (it->keysize) {
      case 4:
        if (first == 'm' && GLTF_JSON_KEY_EQ8(it, mode))
          props->mode = it;
        break;
      case 6:
        if (first == 'e' && GLTF_JSON_KEY_EQ8(it, extras))
          props->extras = it;
        break;
      case 7:
        if (first == 'i' && GLTF_JSON_KEY_EQ8(it, indices)) {
          props->indices = it;
        } else if (first == 't' && GLTF_JSON_KEY_EQ8(it, targets)) {
          props->targets = it;
        }
        break;
      case 8:
        if (first == 'm' && GLTF_JSON_KEY_EQ8(it, material))
          props->material = it;
        break;
      case 10:
        if (first == 'a' && gltf_jsonKeyEqLen(it, _s_gltf_attributes, 10)) {
          props->attributes = it;
        } else if (first == 'e' && gltf_jsonKeyEqLen(it, _s_gltf_extensions, 10)) {
          props->extensions = it;
        }
        break;
      default:
        break;
    }
  }
}

static inline
uint32_t
gltf_childCount(const json_t * __restrict json) {
  const json_t *it;
  uint32_t      count;

  count = 0;
  if (!json)
    return 0;

  for (it = json->value; it; it = it->next)
    count++;

  return count;
}

static
bool
gltf_attrKnownIndexedZero(const char       * __restrict key,
                          size_t                         keysize,
                          const char       ** __restrict raw,
                          AkInputSemantic   * __restrict semantic) {
  switch (keysize) {
    case _s_gltf_COLOR_0_len:
      if (ak_str_eq_packed_fast(key,
                                keysize,
                                _s_gltf_COLOR_0_u64_exact,
                                _s_gltf_COLOR_0_len)) {
        *raw      = _s_gltf_COLOR;
        *semantic = AK_INPUT_COLOR;
        return true;
      }
      break;
    case _s_gltf_JOINTS_0_len:
      if (ak_str_eq_packed_fast(key,
                                keysize,
                                _s_gltf_JOINTS_0_u64_exact,
                                _s_gltf_JOINTS_0_len)) {
        *raw      = _s_gltf_JOINTS;
        *semantic = AK_INPUT_JOINT;
        return true;
      }
      break;
    case _s_gltf_WEIGHTS_0_len:
      if (ak_str_eq_fast(key,
                         keysize,
                         _s_gltf_WEIGHTS_0,
                         _s_gltf_WEIGHTS_0_len)) {
        *raw      = _s_gltf_WEIGHTS;
        *semantic = AK_INPUT_WEIGHT;
        return true;
      }
      break;
    case _s_gltf_TEXCOORD_0_len:
      if (ak_str_eq_fast(key,
                         keysize,
                         _s_gltf_TEXCOORD_0,
                         _s_gltf_TEXCOORD_0_len)) {
        *raw      = _s_gltf_TEXCOORD;
        *semantic = AK_INPUT_TEXCOORD;
        return true;
      }
      break;
    default:
      break;
  }

  return false;
}

static
bool
gltf_attrIndexedSemantic(const char * __restrict key,
                         size_t                  keysize,
                         const char ** __restrict semantic) {
  const char *sep, *end, *c;

  if (!key || keysize == 0)
    return false;

  sep = memchr(key, '_', keysize);
  end = key + keysize;
  if (!sep || sep == key || sep + 1 >= end)
    return false;

  for (c = sep + 1; c < end; c++) {
    if (*c < '0' || *c > '9')
      return false;
  }

  if (semantic)
    *semantic = sep;
  return true;
}

static
uint32_t
gltf_attrSemanticSet(const char * __restrict begin,
                     const char * __restrict end) {
  uint32_t set;

  set = 0;
  while (begin < end)
    set = set * 10u + (uint32_t)(*begin++ - '0');

  return set;
}

static
bool
gltf_attrKnownSemantic(const char       * __restrict key,
                       size_t                         keysize,
                       const char       ** __restrict raw,
                       AkInputSemantic   * __restrict semantic) {
  switch (keysize) {
    case _s_gltf_COLOR_len:
      if (ak_str_eq_packed_fast(key,
                                keysize,
                                _s_gltf_COLOR_u64_exact,
                                _s_gltf_COLOR_len)) {
        *raw      = _s_gltf_COLOR;
        *semantic = AK_INPUT_COLOR;
        return true;
      }
      break;
    case _s_gltf_NORMAL_len:
      if (ak_str_eq_packed_fast(key,
                                keysize,
                                _s_gltf_NORMAL_u64_exact,
                                _s_gltf_NORMAL_len)) {
        *raw      = _s_gltf_NORMAL;
        *semantic = AK_INPUT_NORMAL;
        return true;
      }
      if (ak_str_eq_packed_fast(key,
                                keysize,
                                _s_gltf_JOINTS_u64_exact,
                                _s_gltf_JOINTS_len)) {
        *raw      = _s_gltf_JOINTS;
        *semantic = AK_INPUT_JOINT;
        return true;
      }
      break;
    case _s_gltf_TANGENT_len:
      if (ak_str_eq_packed_fast(key,
                                keysize,
                                _s_gltf_TANGENT_u64_exact,
                                _s_gltf_TANGENT_len)) {
        *raw      = _s_gltf_TANGENT;
        *semantic = AK_INPUT_TANGENT;
        return true;
      }
      if (ak_str_eq_packed_fast(key,
                                keysize,
                                _s_gltf_WEIGHTS_u64_exact,
                                _s_gltf_WEIGHTS_len)) {
        *raw      = _s_gltf_WEIGHTS;
        *semantic = AK_INPUT_WEIGHT;
        return true;
      }
      break;
    case _s_gltf_POSITION_len:
      if (ak_str_eq_packed_fast(key,
                                keysize,
                                _s_gltf_POSITION_u64_exact,
                                _s_gltf_POSITION_len)) {
        *raw      = _s_gltf_POSITION;
        *semantic = AK_INPUT_POSITION;
        return true;
      }
      if (ak_str_eq_packed_fast(key,
                                keysize,
                                _s_gltf_TEXCOORD_u64_exact,
                                _s_gltf_TEXCOORD_len)) {
        *raw      = _s_gltf_TEXCOORD;
        *semantic = AK_INPUT_TEXCOORD;
        return true;
      }
      break;
    default:
      break;
  }

  return false;
}

static
void
gltf_inputSemantic(AkHeap * __restrict heap,
                   AkInput * __restrict inp,
                   json_t  * __restrict jattrib) {
  const char      *semanticSep;
  const char      *raw;
  AkInputSemantic  sem;
  size_t           rawLen;

  if (gltf_attrKnownIndexedZero(jattrib->key,
                                jattrib->keysize,
                                &raw,
                                &sem)) {
    inp->semanticRaw = raw;
    inp->semantic    = sem;
    inp->set         = 0;
    return;
  }

  if (gltf_attrIndexedSemantic(jattrib->key, jattrib->keysize, &semanticSep)) {
    rawLen = (size_t)(semanticSep - jattrib->key);
    if (gltf_attrKnownSemantic(jattrib->key, rawLen, &raw, &sem)) {
      inp->semanticRaw = raw;
      inp->semantic    = sem;
    } else {
      inp->semanticRaw = ak_heap_strndup(heap, inp, jattrib->key, rawLen);
      inp->semantic    = gltf_enumInputSemantic(inp->semanticRaw);
    }

    inp->set = gltf_attrSemanticSet(semanticSep + 1,
                                    jattrib->key + jattrib->keysize);
  } else {
    if (gltf_attrKnownSemantic(jattrib->key,
                               jattrib->keysize,
                               &raw,
                               &sem)) {
      inp->semanticRaw = raw;
      inp->semantic    = sem;
    } else {
      inp->semanticRaw = ak_heap_strndup(heap,
                                         inp,
                                         jattrib->key,
                                         jattrib->keysize);
      inp->semantic    = gltf_enumInputSemantic(inp->semanticRaw);
    }
  }
}

static
AkMorphPreset*
gltf_meshMorphPresets(AkGLTFState * __restrict gst,
                      void        * __restrict parent,
                      json_t      * __restrict jpresets,
                      uint32_t    * __restrict count) {
  json_array_t *jarr;
  json_t       *jpreset;
  json_t       *jname;
  json_t       *jweights;
  json_array_t *jwarr;
  AkMorphPreset *presets;
  AkMorphPreset *preset;
  AkFloatArray *weights;
  AkHeap       *heap;
  uint32_t      n, i;

  *count = 0;

  if (!(jarr = json_array(jpresets)) || jarr->count == 0)
    return NULL;

  heap    = gst->heap;
  n       = (uint32_t)jarr->count;
  presets = ak_heap_calloc(heap, parent, sizeof(*presets) * n);
  jpreset = jarr->base.value;
  i       = 0;

  while (jpreset && i < n) {
    preset = &presets[n - 1 - i];

    if ((jname = GLTF_JSON_GET8(jpreset, name)))
      preset->name = json_strdup(jname, heap, parent);

    if ((jweights = GLTF_JSON_GET8(jpreset, weights))
        && (jwarr = json_array(jweights))
        && jwarr->count > 0) {
      weights = ak_heap_alloc(heap,
                              parent,
                              sizeof(*weights) + sizeof(float) * jwarr->count);
      json_array_float(weights->items,
                       jweights,
                       0.0f,
                       jwarr->count,
                       true);
      weights->count   = jwarr->count;
      preset->weights  = weights;
    }

    i++;
    jpreset = jpreset->next;
  }

  *count = n;
  return presets;
}

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
  size_t              meshIndex;

  if (!(jmeshes = json_array(jmesh)))
    return;

  gst        = userdata;
  heap       = gst->heap;
  doc        = gst->doc;
  lib        = ak_heap_calloc(heap, doc, sizeof(*lib));
  gst->geometriesCount   = jmeshes->count;
  gst->geometriesByIndex = ak_heap_calloc(heap,
                                          gst->tmpParent,
                                          sizeof(*gst->geometriesByIndex)
                                          * gst->geometriesCount);
  meshIndex = gst->geometriesCount;

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
    AkMorphPreset   *morphPresets      = NULL;
    uint32_t         morphPresetCount  = 0;
    uint32_t         morphPresetIdx;
    uint32_t         morphPresetWrite;

    mesh                 = ak_allocMesh(heap, lib, &geom);
    meshObj              = ak_objFrom(mesh);
    mesh->primitiveCount = 0;

    gltf_extra(gst,
               meshObj,
               GLTF_JSON_GET8(jmesh, extras),
               gltf_jsonGetLen(jmesh, _s_gltf_extensions, 10));
    mesh->extra = ak_extra(meshObj);

    jmeshVal = jmesh->value;
    while (jmeshVal) {
      if (gltf_jsonKeyEqLen(jmeshVal, _s_gltf_primitives, 10)
          && json_is_array(jmeshVal)) {
        json_t *jprim;

        jprim = jmeshVal->value;
        while (jprim) {
          AkMeshPrimitive *prim;
          AkGLTFPrimProps  primProps;
          json_t          *jprimVal;

          primProps = (AkGLTFPrimProps){0};
          gltf_primProps(jprim, &primProps);

          mode = json_int32(primProps.mode, 4);
          prim = gltf_allocPrim(heap, meshObj, mode);

          prim->input      = NULL;
          prim->inputCount = 0;
          prim->mesh       = mesh;

          gltf_extra(gst,
                     prim,
                     primProps.extras,
                     primProps.extensions);
          prim->extra = ak_extra(prim);

          if (primProps.extensions && !gltf_ext_dracoPrimitive(gst, prim, jprim)) {
            gst->stop = true;
            return;
          }

          if ((jprimVal = primProps.attributes)) {
            json_t *jattrib;
            AkInput *inputs;
            uint32_t attrIndex;

            /* attributes */
            inputs = ak_heap_calloc(heap,
                                    prim,
                                    sizeof(*inputs)
                                    * gltf_childCount(jprimVal));
            attrIndex = 0;
            jattrib = jprimVal->value;
            while (jattrib) {
              AkInput *inp;

              inp = &inputs[attrIndex++];
              gltf_inputSemantic(heap, inp, jattrib);

              inp->accessor = gltf_accessor_at(gst, json_int32(jattrib, -1));
              if (!inp->accessor) {
                jattrib = jattrib->next;
                continue;
              }

              if (inp->semantic == AK_INPUT_POSITION)
                prim->pos = inp;

              inp->next   = prim->input;
              prim->input = inp;
              prim->inputCount++;

              jattrib = jattrib->next;
            } /* jprimAttrib */
          }

          if ((jprimVal = primProps.indices)) {
            AkAccessor *acc;

            if (!(acc = gltf_accessor_at(gst, json_int32(jprimVal, -1)))
                || !acc->buffer)
              goto prim_next;

            prim->indexStride        = 1;
            prim->indexAccessor      = acc;
          }

          if ((jprimVal = primProps.material)) {
            AkMaterial *mat;
            int32_t     matIndex;

            matIndex = json_int32(jprimVal, -1);
            mat = gltf_material_at(gst, matIndex);

            if (mat) { prim->material = mat;                  }
            else     { prim->material = gst->defaultMaterial; }
          }

          if ((jprimVal = primProps.targets)) {
            json_array_t  *jtargets;
            json_t        *jtarget, *jattrib;
            AkMorphTarget *target;
            AkMorphable   *morphable;
            uint32_t       targetIdx;

            if ((jtargets = json_array(jprimVal))) {
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
                AkInput *inputs;
                uint32_t attrIndex;

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
                inputs = ak_heap_calloc(heap,
                                        prim,
                                        sizeof(*inputs)
                                        * gltf_childCount(jtarget));
                attrIndex = 0;
                jattrib = jtarget->value;
                while (jattrib) {
                  AkInput *inp;

                  inp = &inputs[attrIndex++];
                  gltf_inputSemantic(heap, inp, jattrib);

                  inp->accessor = gltf_accessor_at(gst, json_int32(jattrib, -1));
                  if (!inp->accessor) {
                    jattrib = jattrib->next;
                    continue;
                  }

                  inp->next        = morphable->input;
                  morphable->input = inp;
                  morphable->inputCount++;

                  jattrib = jattrib->next;
                } /* jattrib */

                jtarget = jtarget->next;
                targetIdx++;
              } /* jtarget */
            }
          }

          if (primProps.extensions) {
            if (!gltf_ext_primitiveVariants(gst, prim, jprim)
                || !gltf_ext_primitiveGaussianSplat(gst, prim, jprim)) {
              gst->stop = true;
              return;
            }
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
      } else if (GLTF_JSON_KEY_EQ8(jmeshVal, weights)) {
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
      } else if (GLTF_JSON_KEY_EQ8(jmeshVal, name)) {
        mesh->name = json_strdup(jmeshVal, heap, meshObj);
      } else if (GLTF_JSON_KEY_EQ8(jmeshVal, extras)) {
        json_t       *jnames;
        json_t       *jpresets;
        json_array_t *jarr;

        if ((jnames = gltf_jsonGetLen(jmeshVal, _s_gltf_targetNames, 11))
            && (jarr = json_array(jnames))
            && jarr->count > 0) {
          json_t   *jname;
          uint32_t  i;

          morphTargetNamesN = (uint32_t)jarr->count;
          morphTargetNames  = ak_heap_calloc(heap, meshObj,
                                             sizeof(const char *) * morphTargetNamesN);

          jname = jarr->base.value;
          i     = 0;
          while (jname && i < morphTargetNamesN) {
            morphTargetNames[morphTargetNamesN - 1 - i] =
              json_strdup(jname, heap, meshObj);
            i++;
            jname = jname->next;
          }
        }

        if ((jpresets = gltf_jsonGetLen(jmeshVal, _s_gltf_morphPresets, 12))) {
          morphPresets = gltf_meshMorphPresets(gst,
                                               meshObj,
                                               jpresets,
                                               &morphPresetCount);
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
      if (morphPresets && morphPresetCount > 0) {
        morphPresetWrite = 0;
        for (morphPresetIdx = 0;
             morphPresetIdx < morphPresetCount;
             morphPresetIdx++) {
          if (!morphPresets[morphPresetIdx].weights
              || morphPresets[morphPresetIdx].weights->count != meshMorph->targetCount)
            continue;
          if (morphPresetWrite != morphPresetIdx)
            morphPresets[morphPresetWrite] = morphPresets[morphPresetIdx];
          morphPresetWrite++;
        }

        if (morphPresetWrite > 0) {
          meshMorph->presets    = morphPresets;
          meshMorph->presetCount = morphPresetWrite;
        }
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
    if (meshIndex > 0)
      gst->geometriesByIndex[--meshIndex] = geom;

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
  size_t      indexCount;
  uint32_t    n;

  indexCount = ak_meshPrimitiveIndexCount(prim);
  if (indexCount > 0) {
    n = (uint32_t)indexCount;
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
