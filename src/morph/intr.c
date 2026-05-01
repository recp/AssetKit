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

/*
 * Morph inspect / layout / interleave
 * ===================================
 *
 * Inspect builds a hierarchical view over a morph controller and its targets,
 * suitable for GPU-side blending. The view mirrors the diagram in
 * controller.h:
 *
 *   AkMorphInspectView
 *     ├─ base    : AkMorphInspectTargetView                (always built)
 *     │             └─ morphable[]  : per primitive
 *     │                  └─ input[] : per semantic (POSITION, NORMAL, ...)
 *     └─ targets : AkMorphInspectTargetView linked list    (one per blend shape;
 *                  base is prepended here when includeBaseShape == true)
 *
 * Inspect is static analysis: collects inputs, computes per-vertex stride and
 * per-target buffer size. It does NOT apply weights — those are runtime values
 * (see ak_morphHasOverride and the runtime evaluator/uniform path).
 *
 * Multi-primitive: each TargetView contains one Morphable per primitive of
 * the base mesh; multi-primitive support is structural here. Whether the
 * loaders populate multi-primitive chains is a separate concern.
 */

#include "../common.h"
#include "../accessor.h"

/*============================================================================
 * Inspect helpers (file-local)
 *============================================================================*/

/**
 * @brief initial weight for the i-th morph target.
 *
 * Precedence:
 *   morph.defaultWeights[idx]  → controller-level default (highest)
 *   mesh.weights[idx]           → mesh-level default
 *   0.0                         → fallback
 *
 * Instance-level overrideWeights is intentionally NOT consulted here:
 * inspect is static prep, override is a runtime concern.
 */
AK_INLINE
float
ak_morphInspect_initialWeight(const AkMorph *morph,
                              const AkMesh  *mesh,
                              uint32_t       targetIdx) {
  AkFloatArray *arr;

  if ((arr = morph->defaultWeights) && targetIdx < arr->count)
    return arr->items[targetIdx];

  if (mesh && (arr = mesh->weights) && targetIdx < arr->count)
    return arr->items[targetIdx];

  return 0.0f;
}

/**
 * @brief true iff the input semantic is in the desired filter (or no filter).
 */
AK_INLINE
bool
ak_morphInspect_isDesired(AkInputSemantic   sem,
                          AkInputSemantic  *desired,
                          uint8_t           desiredCount) {
  uint8_t i;

  if (desiredCount == 0) return true;

  for (i = 0; i < desiredCount; i++) {
    if (desired[i] == sem) return true;
  }
  return false;
}

/**
 * @brief true iff `inp` matches one of the base inputs by (semantic, set).
 */
AK_INLINE
bool
ak_morphInspect_inBase(const AkInput              *inp,
                       AkMorphInspectInput * const *base,
                       uint32_t                     baseCount) {
  uint32_t  i;
  AkInput  *binp;

  for (i = 0; i < baseCount; i++) {
    binp = base[i]->input;
    if (binp->semantic == inp->semantic && binp->set == inp->set)
      return true;
  }
  return false;
}

/**
 * @brief append one AkMorphInspectInput to a morphable's input chain.
 */
AK_INLINE
AkMorphInspectInput *
ak_morphInspect_appendInput(AkHeap                   *heap,
                            AkMorphInspectMorphable  *m,
                            AkMorphInspectInput     **last,
                            AkInput                  *input,
                            bool                      inBaseMesh) {
  AkMorphInspectInput *ii;

  ii             = ak_heap_calloc(heap, m, sizeof(*ii));
  ii->input      = input;
  ii->inBaseMesh = inBaseMesh;

  AK_APPEND_FLINK(m->input, (*last), ii);
  m->inputsCount++;

  return ii;
}

/**
 * @brief allocate a Morphable, append to the TargetView's morphable chain.
 */
AK_INLINE
AkMorphInspectMorphable *
ak_morphInspect_appendMorphable(AkHeap                     *heap,
                                AkMorphInspectTargetView   *tv,
                                AkMorphInspectMorphable   **last,
                                float                       weight) {
  AkMorphInspectMorphable *m;

  m         = ak_heap_calloc(heap, tv, sizeof(*m));
  m->weight = weight;

  AK_APPEND_FLINK(tv->morphable, (*last), m);
  tv->nTargets++;       /* nTargets here = primitive count of this target view */

  return m;
}

/**
 * @brief allocate a TargetView, append to the View's targets chain.
 */
AK_INLINE
AkMorphInspectTargetView *
ak_morphInspect_appendTargetView(AkHeap                      *heap,
                                 AkMorphInspectView          *view,
                                 AkMorphInspectTargetView   **last) {
  AkMorphInspectTargetView *tv;

  tv = ak_heap_calloc(heap, view, sizeof(*tv));
  AK_APPEND_FLINK(view->targets, (*last), tv);
  view->nTargets++;

  return tv;
}

/**
 * @brief find the POSITION input in an input chain (NULL if absent).
 */
AK_INLINE
AkInput *
ak_morphInspect_findPosition(AkInput *first) {
  AkInput *inp;

  for (inp = first; inp; inp = inp->next) {
    if (inp->semantic == AK_INPUT_POSITION) return inp;
  }
  return NULL;
}

/*============================================================================
 * ak_morphInspect
 *============================================================================*/

AK_EXPORT
AkResult
ak_morphInspect(AkGeometry * __restrict baseMesh,
                AkMorph    * __restrict morph,
                AkInputSemantic         desiredInputs[],
                uint8_t                 desiredInputsCount,
                bool                    includeBaseShape,
                bool                    ignoreUncommonInputs) {
  AkHeap                     *heap;
  AkMorphInspectView         *view;
  AkMorphInspectTargetView   *tv,        *lastTV;
  AkMorphInspectMorphable    *m,         *lastM;
  AkMorphInspectInput        *iiOut,     *lastInput;
  AkMorphInspectInput       **baseInputs;        /* matching scratch (first base prim) */
  AkMorphTarget              *target;
  AkObject                   *gdataObj,  *targetObj;
  AkMesh                     *mesh,      *targetMesh;
  AkMeshPrimitive            *prim,      *targetPrim;
  AkInput                    *inp,       *posInp;
  AkAccessor                 *acc;
  AkMorphable                *morphable;
  AkGeometry                 *targetGeom;
  void                       *targetPtr;
  uint32_t                    primIdx,   primCount;
  uint32_t                    vertexCount;
  uint32_t                    baseInputsCount;
  uint32_t                    targetIdx;
  size_t                      stridePerVertex;
  bool                        inBase;

  if (!baseMesh || !morph) return AK_ERR;

  heap = ak_heap_getheap(morph);
  view = ak_heap_calloc(heap, morph, sizeof(*view));

  view->layout                = AK_MORPH_UNKNOWN;
  view->includeBaseShape      = includeBaseShape;
  view->ignoreUncommonInputs  = ignoreUncommonInputs;

  /*========================================================================*/
  /* Phase 1: validate base mesh                                            */
  /*========================================================================*/

  if (!(gdataObj = baseMesh->gdata)
      || gdataObj->type != AK_GEOMETRY_MESH
      || !(mesh    = ak_objGet(gdataObj))
      || !(prim    = mesh->primitive)) {
    return AK_ERR;
  }

  primCount = mesh->primitiveCount;
  if (primCount == 0) return AK_ERR;

  /*========================================================================*/
  /* Phase 2: build base TargetView                                         */
  /*                                                                        */
  /* Always built. Even when includeBaseShape == false, base inputs are     */
  /* needed to match against target inputs (ignoreUncommonInputs filter).   */
  /*========================================================================*/

  tv          = ak_heap_calloc(heap, view, sizeof(*tv));
  view->base  = tv;

  baseInputs      = NULL;
  baseInputsCount = 0;
  vertexCount     = 0;
  stridePerVertex = 0;
  lastM           = NULL;

  primIdx = 0;
  for (prim = mesh->primitive; prim && primIdx < primCount;
       prim = prim->next, primIdx++) {
    if (!prim->pos || !(acc = prim->pos->accessor)) continue;

    /* anchor: use first primitive's POSITION count as the canonical
       vertex count and allocate the matching scratch */
    if (primIdx == 0) {
      vertexCount = acc->count;
      if (prim->inputCount > 0)
        baseInputs = alloca(sizeof(*baseInputs) * prim->inputCount);
    }

    m         = ak_morphInspect_appendMorphable(heap, tv, &lastM, 0.0f);
    lastInput = NULL;

    for (inp = prim->input; inp; inp = inp->next) {
      if (!ak_morphInspect_isDesired(inp->semantic, desiredInputs,
                                     desiredInputsCount))
        continue;
      if (!(acc = inp->accessor) || acc->count != vertexCount)
        continue;

      iiOut = ak_morphInspect_appendInput(heap, m, &lastInput, inp, true);

      /* matching scratch + stride contribution: only first primitive */
      if (primIdx == 0 && baseInputs) {
        baseInputs[baseInputsCount++] = iiOut;
        if (includeBaseShape)
          stridePerVertex += acc->fillByteSize;
      }
    }

    m->lastInput = lastInput;
  }

  if (vertexCount == 0 || baseInputsCount == 0) return AK_ERR;

  /*========================================================================*/
  /* Phase 3: build TargetView per AkMorphTarget                            */
  /*========================================================================*/

  lastTV    = NULL;
  targetIdx = 0;

  for (target = morph->target; target;
       target = target->next, targetIdx++) {
    if (!(targetObj = target->target)
        || !(targetPtr = ak_objGet(targetObj)))
      continue;

    tv    = ak_morphInspect_appendTargetView(heap, view, &lastTV);
    lastM = NULL;

    /* polymorphic dispatch on AkMorphableType (kept as enum slot in
       AkObject.type — see controller.h) */
    switch (targetObj->type) {
      case AK_MORPHABLE_MORPHABLE: {
        /* glTF-style: AkMorphable chain, one per primitive */
        morphable = targetPtr;
        primIdx   = 0;

        for (; morphable && primIdx < primCount;
             morphable = morphable->next, primIdx++) {
          if (!(posInp = ak_morphInspect_findPosition(morphable->input))
              || !(acc = posInp->accessor)
              || acc->count != vertexCount)
            continue;

          m = ak_morphInspect_appendMorphable(
              heap, tv, &lastM,
              ak_morphInspect_initialWeight(morph, mesh, targetIdx));
          lastInput = NULL;

          for (inp = morphable->input; inp; inp = inp->next) {
            if (!ak_morphInspect_isDesired(inp->semantic, desiredInputs,
                                           desiredInputsCount))
              continue;
            if (!(acc = inp->accessor) || acc->count != vertexCount)
              continue;

            inBase = ak_morphInspect_inBase(inp, baseInputs, baseInputsCount);
            if (ignoreUncommonInputs && !inBase) continue;

            ak_morphInspect_appendInput(heap, m, &lastInput, inp, inBase);

            /* stride: only first target's first primitive contributes
               (uniform layout assumed across targets) */
            if (primIdx == 0 && targetIdx == 0)
              stridePerVertex += acc->fillByteSize;
          }
          m->lastInput = lastInput;
        }
        break;
      }

      case AK_MORPHABLE_GEOMETRY: {
        /* DAE-style: pointer-storage payload — wrap holds an AkGeometry*
           in its inline slot. ak_objGetTarget hides the deref. */
        targetGeom = ak_objGetTarget(targetObj);
        if (!targetGeom
            || !(gdataObj   = targetGeom->gdata)
            || gdataObj->type != AK_GEOMETRY_MESH
            || !(targetMesh = ak_objGet(gdataObj)))
          continue;

        primIdx = 0;
        for (targetPrim = targetMesh->primitive;
             targetPrim && primIdx < primCount;
             targetPrim = targetPrim->next, primIdx++) {
          if (!targetPrim->pos
              || !(acc = targetPrim->pos->accessor)
              || acc->count != vertexCount)
            continue;

          m = ak_morphInspect_appendMorphable(
              heap, tv, &lastM,
              ak_morphInspect_initialWeight(morph, mesh, targetIdx));
          lastInput = NULL;

          for (inp = targetPrim->input; inp; inp = inp->next) {
            if (!ak_morphInspect_isDesired(inp->semantic, desiredInputs,
                                           desiredInputsCount))
              continue;
            if (!(acc = inp->accessor) || acc->count != vertexCount)
              continue;

            inBase = ak_morphInspect_inBase(inp, baseInputs, baseInputsCount);
            if (ignoreUncommonInputs && !inBase) continue;

            ak_morphInspect_appendInput(heap, m, &lastInput, inp, inBase);

            if (primIdx == 0 && targetIdx == 0)
              stridePerVertex += acc->fillByteSize;
          }
          m->lastInput = lastInput;
        }
        break;
      }

      default:
        /* unknown target kind — TargetView already linked, leave empty */
        break;
    }
  }

  /*========================================================================*/
  /* Phase 4: prepend base to view->targets when includeBaseShape           */
  /*========================================================================*/

  if (includeBaseShape) {
    view->base->next = view->targets;
    view->targets    = view->base;
    view->nTargets++;
  }

  /*========================================================================*/
  /* Phase 5: summary fields                                                */
  /*========================================================================*/

  view->interleaveTotalBufferSize = stridePerVertex * vertexCount
                                  * view->nTargets;

  for (tv = view->targets; tv; tv = tv->next) {
    tv->interleaveByteStride = stridePerVertex;
    tv->interleaveBufferSize = stridePerVertex * vertexCount;
    tv->accessorAccessCount  = vertexCount;
  }

  /* convenience cache: per-blend-shape initial weights for runtime
     (length = morph->targetCount; base shape is NOT included here) */
  if (morph->targetCount > 0) {
    AkFloatArray *iw;
    uint32_t      i;

    iw = ak_heap_calloc(heap, view,
                        sizeof(*iw) + sizeof(AkFloat) * morph->targetCount);
    iw->count = morph->targetCount;
    for (i = 0; i < morph->targetCount; i++) {
      iw->items[i] = ak_morphInspect_initialWeight(morph, mesh, i);
    }
    view->initialWeights = iw;
  }

  morph->inspectResult = view;
  return AK_OK;
}

/*============================================================================
 * ak_morphInspectPrepareLayout
 *============================================================================*/

AK_EXPORT
AkResult
ak_morphInspectPrepareLayout(AkMorphInspectView * __restrict inspectView,
                             AkMorphInterleaveLayout         layout) {
  AkMorphInspectTargetView  *targetView, *base;
  AkMorphInspectMorphable   *inspMorphable;
  AkMorphInspectInput       *tinp,       *tinpt;
  AkInput                   *inp;
  AkAccessor                *acc;
  uint32_t                   inpOff;
  bool                       includeBaseShape, ignoreUncommonInputs;

  if (!inspectView
      || !(base = inspectView->base)
      || !(inspMorphable = base->morphable)
      || !(tinp = inspMorphable->input))
    return AK_ERR;

  if (inspectView->layout == layout) return AK_OK;

  includeBaseShape     = inspectView->includeBaseShape;
  ignoreUncommonInputs = inspectView->ignoreUncommonInputs;
  inpOff               = 0;

  switch (layout) {
    case AK_MORPH_P1P2N1N2: {
      /* group by input type: P1 P2 P3 ... N1 N2 N3 ... */
      do {
        for (targetView = inspectView->targets;
             targetView
             && (inspMorphable = targetView->morphable)
             && (tinpt = inspMorphable->input);
             targetView = targetView->next) {
          do {
            if ((!tinp->inTarget && !includeBaseShape)
                || !(tinpt->input->semantic == tinp->input->semantic
                     && tinpt->input->set == tinp->input->set)
                || (ignoreUncommonInputs && !tinpt->inBaseMesh))
              continue;

            inp               = tinpt->input;
            acc               = inp->accessor;
            tinpt->intrOffset = inpOff;
            inpOff           += (uint32_t)acc->fillByteSize;
            goto nxt;
          } while ((tinpt = tinpt->next));
        nxt: continue;
        }
      } while ((tinp = tinp->next));

      /* trailing: inputs that don't exist in base, ungrouped */
      if (!ignoreUncommonInputs) {
        for (targetView = inspectView->targets;
             targetView;
             targetView = targetView->next) {
          if ((inspMorphable = targetView->morphable)
              && (tinp = inspMorphable->input)) {
            do {
              if (tinp->inBaseMesh) continue;
              inp              = tinp->input;
              acc              = inp->accessor;
              tinp->intrOffset = inpOff;
              inpOff          += (uint32_t)acc->fillByteSize;
            } while ((tinp = tinp->next));
          }
        }
      }
      inspectView->layout = layout;
      return AK_OK;
    }

    case AK_MORPH_NATURAL: {
      /* natural order: P1 N1 T1   P2 N2 T2   ... */
      for (targetView = inspectView->targets;
           targetView;
           targetView = targetView->next) {
        if ((inspMorphable = targetView->morphable)
            && (tinp = inspMorphable->input)) {
          do {
            inp              = tinp->input;
            acc              = inp->accessor;
            tinp->intrOffset = inpOff;
            inpOff          += (uint32_t)acc->fillByteSize;
          } while ((tinp = tinp->next));
        }
      }
      inspectView->layout = layout;
      return AK_OK;
    }

    default:
      break;
  }

  return AK_ERR;
}

/*============================================================================
 * ak_morphInterleave
 *============================================================================*/

AK_EXPORT
AkResult
ak_morphInterleave(AkGeometry * __restrict baseMesh,
                   AkMorph    * __restrict morph,
                   AkMorphInterleaveLayout layout,
                   void       * __restrict destBuff) {
  AkMorphInspectView        *morphView;
  AkMorphInspectTargetView  *targetView;
  AkMorphInspectMorphable   *inspMorphable;
  AkMorphInspectInput       *tinp;
  AkInput                   *inp;
  AkAccessor                *acc;
  AkBuffer                  *buf;
  char                      *src,        *dst;
  uint32_t                   srcStride,  targetStride, compSize;
  uint32_t                   k,          count, intrOffset;

  /* lazy-inspect with default options if caller didn't run it explicitly */
  if (!(morphView = morph->inspectResult)) {
    if (ak_morphInspect(baseMesh, morph, NULL, 0, false, true) != AK_OK
        || !(morphView = morph->inspectResult))
      return AK_ERR;
  }

  if (layout != morphView->layout
      && ak_morphInspectPrepareLayout(morphView, layout) != AK_OK)
    return AK_ERR;

  if (!(targetView = morphView->targets)) return AK_ERR;

  dst = (char *)destBuff;

  for (; targetView; targetView = targetView->next) {
    targetStride = (uint32_t)targetView->interleaveByteStride;
    count        = targetView->accessorAccessCount;

    for (inspMorphable = targetView->morphable;
         inspMorphable && (tinp = inspMorphable->input);
         inspMorphable = inspMorphable->next) {
      for (; tinp
             && (inp = tinp->input)
             && (acc = inp->accessor)
             && (buf = acc->buffer)
             && (src = (char *)buf->data + acc->byteOffset);
           tinp = tinp->next) {
        srcStride  = (uint32_t)acc->byteStride;
        compSize   = (uint32_t)acc->fillByteSize;
        intrOffset = tinp->intrOffset;

        for (k = 0; k < count; k++) {
          memcpy(dst + intrOffset + targetStride * k,
                 src + srcStride * k,
                 compSize);
        }
      }
    }
  }

  return AK_OK;
}
