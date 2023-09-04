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
  AkHeap                   *heap;
  AkMorphInspectView       *view;
  AkMorphInspectTargetView *targetView, *last;
  AkMorphTarget            *target;
  AkMorphable              *morphable;
  AkGeometry               *geom;
  AkInput                  *inp, *inpPosition;
  AkAccessor               *posAcc, *acc;
  AkObject                 *targetObj, *geomPrimObj;
  void                     *targetPtr;
  AkMesh                   *mesh;
  AkMeshPrimitive          *prim;
  AkInput                 **baseMeshInputs;
  uint32_t                  i, count, nBaseMeshInputs;
  size_t                    targetStride;

  if (!baseMesh || !(target = morph->target)) { return AK_ERR; }

  targetStride = 0;
  count        = 0;
  last         = NULL;
  heap         = ak_heap_getheap(morph);
  view         = ak_heap_calloc(heap, morph, sizeof(*view));

#define ak__collectBaseInput(inp)                                             \
  do {                                                                        \
    flist_sp_insert(&targetView->inputs, inp);                                \
    baseMeshInputs[targetView->inputsCount++] = inp;                          \
    if (includeBaseShape)                                                     \
      targetStride += acc->fillByteSize;                                      \
  } while (0)

#define ak__collectTargetInput(inp)                                           \
  do {                                                                        \
    AkInput *_inp;                                                            \
    for (i = 0; i < nBaseMeshInputs; i++) {                                   \
      _inp = baseMeshInputs[i];                                               \
      if (!ignoreUncommonInputs                                               \
           || (_inp->semantic == inp->semantic && _inp->set == inp->set)) {   \
        flist_sp_insert(&targetView->inputs, inp);                            \
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
     || !(inpPosition = inp = prim->pos)
     || !(posAcc      = inpPosition->accessor)
     || !(count       = posAcc->count)) { 
    return AK_ERR;
  }

#define ak__collectInput ak__collectBaseInput
  nBaseMeshInputs = prim->inputCount;
  baseMeshInputs  = alloca(nBaseMeshInputs * sizeof(*baseMeshInputs));
  do { COLLECT_TARGET } while((prim = prim->next));
#undef ak__collectInput
#define ak__collectInput ak__collectTargetInput

  /* collect morph targets */
  do {
   if (!(targetObj = target->target) || !(targetPtr = ak_objGet(targetObj)))
     continue;

    switch (targetObj->type) {
      case AK_MORPHABLE_MORPHABLE: {
        morphable = targetPtr;
        if (!(inp = morphable->input) 
           || !(inpPosition = ak_getPositionInput(inp))
           || !(posAcc      = inpPosition->accessor)
           || !(count       = posAcc->count)) {
          continue;
        }
        do { COLLECT_TARGET } while((morphable = morphable->next)); 
        break;
      }
      case AK_MORPHABLE_GEOMETRY: {
        geom = targetPtr;
        if (!(geomPrimObj   = geom->gdata)
           || (geomPrimObj->type != AK_GEOMETRY_MESH)
           || !(mesh        = ak_objGet(geomPrimObj))
           || !(prim        = mesh->primitive)
           || !(inpPosition = inp = prim->pos)
           || !(posAcc      = inpPosition->accessor)
           || !(count       = posAcc->count)) {
          continue;
        }
        do { COLLECT_TARGET } while((prim = prim->next));
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
ak_morphInterleave(AkGeometry * __restrict baseMesh,
                   AkMorph    * __restrict morph, 
                   AkMorphInterleaveLayout layout, 
                   void       * __restrict destBuff) {
  AkMorphInspectView       *morphView;
  AkMorphInspectTargetView *targetView, *base;
  AkInput                  *inp;
  AkAccessor               *acc;
  AkBuffer                 *buf;
  FListItem                *finp;
  char                     *src, *dst;
  uint16_t                 *offs2, *offs1;
  size_t                    srcStride, targetStride, compSize;
  uint32_t                  j, k, count, nTargets, inpOff;

  /* ispect is not called, we need to inspect it with default behavior */
  if (!morph->inspectResult) { 
    if (ak_morphInspect(baseMesh, morph, NULL, 0, false, true) != AK_OK) { return AK_ERR; }
  }

  if (!(morphView     = morph->inspectResult)
      || !(base       = morphView->base)
      || !(targetView = morphView->targets)) {
    return AK_ERR;
  }

  dst          = (char *)destBuff;
  nTargets     = morphView->nTargets;
  targetStride = morphView->interleaveByteStride;
  count        = morphView->accessorAccessCount;
  offs1        = alloca(base->inputsCount * sizeof(*offs1));
  offs2        = alloca(base->inputsCount * sizeof(*offs2));
  finp         = base->inputs;

  /*  currently two layouts are supported: 
      ------------------------------------------------------------------------
      P1 P2 P3    N1 N2 N3    T01 T02 T03 ...
      P1 N1 T01   P2 N2 T02   P3  N3  T03 ...

      offs1 -- P1  -- offs2 --  P2
   */
  switch (layout) {
    case AK_MORPH_ILAYOUT_P1P2N1N2: {
      for (j = 0, inpOff = 0;
           finp && (inp = finp->data) && (acc = inp->accessor); 
           finp = finp->next, j++) {
        compSize = acc->fillByteSize;
        offs1[j] = inpOff + compSize * nTargets;
        offs2[j] = targetStride - offs1[j];
        inpOff  += offs1[j];
      }
      break;
    }
    case AK_MORPH_ILAYOUT_P1N1P2N2: {
      for (j = 0, inpOff = 0;
           finp && (inp = finp->data) && (acc = inp->accessor); 
           finp = finp->next, j++) {
        compSize = acc->fillByteSize;
        offs1[j] = inpOff + compSize;
        offs2[j] = targetStride - offs1[j];
        inpOff  += offs1[j];
      }
      break;
    }
    case AK_MORPH_ILAYOUT_P1N1P2N2_ORIGINAL: {
      printf("AK_MORPH_ILAYOUT_P1N1P2N2_ORIGINAL is not implemented yet.");
      return AK_ERR;
    }
    default: return AK_ERR;
  }

  /* TODO: optimize these operations */

  for (;
       targetView && (finp = targetView->inputs);
       targetView = targetView->next)
  {
    for (j = 0;
         finp
         && (inp = finp->data)
         && (acc = inp->accessor)
         && (buf = acc->buffer)
         && (src = (char *)buf->data + acc->byteOffset);
         finp = finp->next, j++)
    {
      srcStride = acc->byteStride;
      compSize  = acc->fillByteSize;
      inpOff    = offs1[j] + offs2[j];

      for (k = 0; k < count; k++) {
        memcpy(dst + inpOff * k, src + srcStride * k, compSize);
      }
    }
  }

  return AK_OK;
}
