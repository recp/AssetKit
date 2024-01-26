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

#include "../common.h"
#include "../accessor.h"

AK_INLINE
AkInput*
ak_getPositionInput(AkInput *inp) {
  do {
    if (inp->semantic == AK_INPUT_POSITION)
      return inp;
  } while ((inp = inp->next));

  return NULL;
}

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
  AkMorphInspectTargetView   *targetView, *last;
  AkMorphTarget              *target;
  AkMorphable                *morphable;
  AkGeometry                 *geom;
  AkInput                    *inp, *inpPosition;
  AkAccessor                 *posAcc, *acc;
  AkObject                   *targetObj, *geomPrimObj;
  void                       *targetPtr;
  AkMesh                     *mesh;
  AkMeshPrimitive            *prim;
  AkMorphInspectTargetInput **baseMeshInputs;
  uint32_t                    i, j, count, nBaseMeshInputs;
  size_t                      targetStride;

  if (!baseMesh || !(target = morph->target)) { return AK_ERR; }

  targetStride = 0;
  last         = NULL;
  heap         = ak_heap_getheap(morph);
  view         = ak_heap_calloc(heap, morph, sizeof(*view));

#define ak__collectBaseInput(inp)                                             \
  do {                                                                        \
    AkMorphInspectTargetInput *iti;                                           \
    iti             = ak_heap_calloc(heap, targetView, sizeof(*iti));         \
    iti->input      = inp;                                                    \
    iti->inBaseMesh = true;                                                   \
    AK_APPEND_FLINK(targetView->input, targetView->lastInput, iti);           \
    baseMeshInputs[targetView->inputsCount++] = iti;                          \
    if (includeBaseShape)                                                     \
      targetStride += acc->fillByteSize;                                      \
  } while (0)

#define ak__collectTargetInput(inp)                                           \
  do {                                                                        \
    AkMorphInspectTargetInput *iti, *iti_t;                                   \
    for (j = 0; j < nBaseMeshInputs; j++) {                                   \
      iti = baseMeshInputs[j];                                                \
      if (!ignoreUncommonInputs                                               \
           || (iti->input->semantic == inp->semantic                          \
               && iti->input->set == inp->set)) {                             \
        iti_t           = ak_heap_calloc(heap, targetView, sizeof(*iti_t));   \
        iti_t->input    = inp;                                                \
        iti_t->inTarget = true;                                               \
        AK_APPEND_FLINK(targetView->input, targetView->lastInput, iti_t);     \
        targetView->inputsCount++;                                            \
        targetStride += acc->fillByteSize;                                    \
        break;                                                                \
      }                                                                       \
    }                                                                         \
  } while (0)

#define COLLECT_INPUTS                                                        \
  do {                                                                        \
    /* validate POSITION; all positions must be similar layout,               \
       otherwise ignore target to morph */                                    \
    if (!(acc = inp->accessor) || acc->count != count) { continue; }          \
                                                                              \
    /* if desiredInputsCount > 0 then collect desired inputs */               \
    if (desiredInputsCount > 0) {                                             \
      for (i = 0; i < desiredInputsCount; i++) {                              \
        if (desiredInputs[i] == inp->semantic) {                              \
          ak__collectInput(inp);                                              \
          break;                                                              \
        }                                                                     \
      }                                                                       \
      continue;                                                               \
    }                                                                         \
                                                                              \
    /* otherwsie collect all inputs */                                        \
    ak__collectInput(inp);                                                    \
  } while ((inp = inp->next))

#define COLLECT_TARGET                                                        \
  targetView         = ak_heap_calloc(heap, view, sizeof(*targetView));       \
  targetView->weight = target->weight;                                        \
  view->nTargets++;                                                           \
  COLLECT_INPUTS;                                                             \
  AK_APPEND_FLINK(view->targets, last, targetView);

  /* collect base mesh */
  if (!(geomPrimObj   = baseMesh->gdata)
     || (geomPrimObj->type != AK_GEOMETRY_MESH)
     || !(mesh        = ak_objGet(geomPrimObj))
     || !(prim        = mesh->primitive)
     || !(inpPosition = prim->pos)
     || !(posAcc      = inpPosition->accessor)
     || !(count       = posAcc->count)) { 
    return AK_ERR;
  }

#define ak__collectInput ak__collectBaseInput
  nBaseMeshInputs = prim->inputCount;
  baseMeshInputs  = alloca(nBaseMeshInputs * sizeof(*baseMeshInputs));
  do {
    if (!(inp = prim->input)) {
      nBaseMeshInputs--;
      continue;
    }
    COLLECT_TARGET
  } while((prim = prim->next));
#undef ak__collectInput
#define ak__collectInput ak__collectTargetInput

  if (nBaseMeshInputs < 1) { return AK_ERR; }

  /* collect morph targets */
  do {
    if (!(targetObj = target->target) || !(targetPtr = ak_objGet(targetObj)))
      continue;

    switch (targetObj->type) {
      case AK_MORPHABLE_MORPHABLE: {
        morphable = targetPtr;
        do {
          /* TODO: maybe position can be optional in future desired... */
          if (!(inp = morphable->input)
              || !(inpPosition = ak_getPositionInput(inp))
              || !(posAcc      = inpPosition->accessor)
              || !(count       = posAcc->count)) {
            continue;
          }
          COLLECT_TARGET
        } while((morphable = morphable->next));
        break;
      }
      case AK_MORPHABLE_GEOMETRY: {
        geom = targetPtr;
        if (!(geomPrimObj         = geom->gdata)
           || (geomPrimObj->type != AK_GEOMETRY_MESH)
           || !(mesh              = ak_objGet(geomPrimObj))
           || !(prim              = mesh->primitive)
           || !(inpPosition       = prim->pos)
           || !(posAcc            = inpPosition->accessor)
           || !(count             = posAcc->count)) {
          continue;
        }
        do {
          if (!(inp = prim->input)) { continue; }
          COLLECT_TARGET
        } while((prim = prim->next));
        break;
      }
      default: goto err;
    }
  } while ((target = target->next));

  view->base = view->targets;

  /* ignore base mesh in interleaved data if needed */
  if (!includeBaseShape) {
    if (view->targets) view->targets = view->targets->next;
    view->nTargets--;
  }

  view->interleaveByteStride = targetStride;
  view->interleaveBufferSize = targetStride * morph->targetCount * count;
  view->accessorAccessCount  = count;
  view->includeBaseShape     = includeBaseShape;
  morph->inspectResult       = view;

#undef COLLECT_INPUTS
#undef COLLECT_TARGET
#undef ak__collectBaseInput
#undef ak__collectTargetInput

  return AK_OK;
err:
  ak_free(view);
  return AK_ERR;
}

AK_EXPORT
AkResult
ak_morphInspectPrepareLayout(AkMorphInspectView * __restrict inspectView, 
                             AkMorphInterleaveLayout         layout) {                            
  AkMorphInspectTargetView  *targetView, *base;
  AkMorphInspectTargetInput *tinp, *tinpt;
  AkInput                   *inp;
  AkAccessor                *acc;
  uint32_t                   inpOff;
  bool                       includeBaseShape, ignoreUncommonInputs;

  if (!(base = inspectView->base) || !(tinp = base->input)) { return AK_ERR; }
  if (inspectView->layout == layout)                        { return AK_OK;  }

  includeBaseShape     = inspectView->includeBaseShape;
  ignoreUncommonInputs = inspectView->ignoreUncommonInputs;
  inpOff               = 0;

  switch (layout) {
    case AK_MORPH_P1P2N1N2: {
      /* inputs are in base mesh */
      do {
        for (targetView = inspectView->targets;
             targetView && (tinpt = targetView->input);
             targetView = targetView->next) {
          do {
            if ((!tinp->inTarget && !includeBaseShape)
                || !(tinpt->input->semantic == tinp->input->semantic 
                    && tinpt->input->set == tinp->input->set)
                || (ignoreUncommonInputs && !tinpt->inBaseMesh)) { 
              continue;
            }

            inp               = tinpt->input;
            acc               = inp->accessor;
            tinpt->intrOffset = inpOff;
            inpOff           += (uint32_t)acc->fillByteSize;
            goto nxt;
          } while ((tinpt = tinpt->next));
        nxt: continue;
        }
      } while ((tinp = tinp->next));

      /* put all inputs that are not in base mesh at last
               without grouping them (TODO?) */
      if (!ignoreUncommonInputs) {
        for (targetView = inspectView->targets;
             targetView;
             targetView = targetView->next) {
          if ((tinp = targetView->input)) {
            do {
              /* get only that are not in base */
              if (tinp->inBaseMesh) { continue; }
              inp              = tinp->input;
              acc              = inp->accessor;
              tinp->intrOffset = inpOff;
              inpOff          += (uint32_t)acc->fillByteSize;
            } while ((tinp = tinp->next));
          }
        }
      }
      return AK_OK;
    }
    // case AK_MORPH_P1N1P2N2: {
    //   for (targetView = inspectView->targets;
    //        targetView;
    //        targetView = targetView->next) {
    //     if ((tinpt = targetView->inputs)) {
    //       tinp = base->input;
    //       do {
    //         do {
    //           if (!tinp->inTarget 
    //               || !(tinpt->input->semantic == tinp->input->semantic 
    //                    && tinpt->input->set == tinpt->input->set)) { 
    //             continue; 
    //           }
    //           inp               = tinpt->input;
    //           acc               = inp->accessor;
    //           tinpt->intrOffset = inpOff;
    //           inpOff          += (uint32_t)acc->fillByteSize;
    //         } while ((tinp = tinp->next));

    //         tinpt = targetView->inputs;
    //       } while ((tinp = tinp->next));
    //     } else { continue; }
    //   }
    //   return AK_OK;
    // }
    case AK_MORPH_NATURAL: {
      for (targetView = inspectView->targets;
           targetView;
           targetView = targetView->next) {
        if ((tinp = targetView->input)) {
          do {
            /* if (!tinp->inBaseMesh) { continue; } */
            inp              = tinp->input;
            acc              = inp->accessor;
            tinp->intrOffset = inpOff;
            inpOff          += (uint32_t)acc->fillByteSize;
          } while ((tinp = tinp->next));
        }
      }
      return AK_OK;
    }
    default: break;
  }

  return AK_ERR;
}

AK_EXPORT
AkResult
ak_morphInterleave(AkGeometry * __restrict baseMesh,
                   AkMorph    * __restrict morph, 
                   AkMorphInterleaveLayout layout, 
                   void       * __restrict destBuff) {
  AkMorphInspectView        *morphView;
  AkMorphInspectTargetView  *targetView;
  AkMorphInspectTargetInput *tinp;
  AkInput                   *inp;
  AkAccessor                *acc;
  AkBuffer                  *buf;
  char                      *src, *dst;
  uint32_t                   srcStride, targetStride, compSize;
  uint32_t                   i, j, k, count, intrOffset;

  /* ispect is not called, we need to inspect it with default behavior */
  if (!(morphView = morph->inspectResult)
      || ak_morphInspect(baseMesh, morph, NULL, 0, false, true) != AK_OK
      || !(morphView = morph->inspectResult)
      || (layout != morphView->layout
          && ak_morphInspectPrepareLayout(morphView, layout) != AK_OK)
      || !(targetView = morphView->targets)) { 
    return AK_ERR; 
  }

  dst          = (char *)destBuff;
  targetStride = (uint32_t)morphView->interleaveByteStride;
  count        = morphView->accessorAccessCount;

  /* TODO: optimize these operations */

  for (i = 0;
       targetView && (tinp = targetView->input);
       targetView = targetView->next, i++)
  {
    for (j = 0;
         tinp
         && (inp = tinp->input)
         && (acc = inp->accessor)
         && (buf = acc->buffer)
         && (src = (char *)buf->data + acc->byteOffset);
         tinp = tinp->next, j++)
    {
      srcStride  = (uint32_t)acc->byteStride;
      compSize   = (uint32_t)acc->fillByteSize;
      intrOffset = tinp->intrOffset;

      for (k = 0; k < count; k++) {
        memcpy(dst + intrOffset + targetStride * k, src + srcStride * k, compSize);
      }
    }
  }

  return AK_OK;
}
