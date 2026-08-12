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
#include <string.h>

/*============================================================================
 * Inspect helpers (file-local)
 *============================================================================*/

AK_EXPORT
const AkMorphPreset*
ak_morphPresetByName(AkMorph    * __restrict morph,
                     const char * __restrict name) {
  AkMorphPreset *preset;
  uint32_t       i;

  if (!morph || !name || !morph->presets || morph->presetCount == 0)
    return NULL;

  preset = morph->presets;
  for (i = 0; i < morph->presetCount; i++) {
    if (preset[i].name && strcmp(preset[i].name, name) == 0)
      return &preset[i];
  }

  return NULL;
}

AK_EXPORT
bool
ak_morphApplyPreset(AkMorph    * __restrict morph,
                    const char * __restrict presetName,
                    float      * __restrict outWeights,
                    uint32_t                capacity) {
  const AkMorphPreset *preset;
  AkFloatArray        *weights;
  uint32_t             i;

  if (!morph || !outWeights || capacity < morph->targetCount)
    return false;

  preset = ak_morphPresetByName(morph, presetName);
  if (!preset || !(weights = preset->weights))
    return false;

  if (weights->count != morph->targetCount)
    return false;

  for (i = 0; i < morph->targetCount; i++)
    outWeights[i] = weights->items[i];

  return true;
}

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
                       const AkMorphInspectMorphable *base) {
  AkMorphInspectInput *bi;
  AkInput             *binp;

  if (!inp || !base) return false;

  for (bi = base->input; bi; bi = bi->next) {
    binp = bi->input;
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

AK_INLINE
AkUInt
ak_morphInspect_indexValue(const AkIndexArray *array, size_t index) {
  if (!array || index >= array->count)
    return UINT32_MAX;

  switch (array->componentType) {
    case AKT_UBYTE:  return ((const uint8_t *)array->items)[index];
    case AKT_USHORT: return ((const uint16_t *)array->items)[index];
    case AKT_UINT:   return ((const uint32_t *)array->items)[index];
    default:         return UINT32_MAX;
  }
}

/**
 * Expand one mesh-wide morph input through the base primitive's retained
 * tuple-index duplicator.  DAE target geometries without primitives keep the
 * original vertex-domain count, while the base primitive may have been
 * triangulated and split for normals/UVs before morph inspection.
 */
static
AkInput *
ak_morphInspect_expandInput(AkHeap       *heap,
                            void         *parent,
                            AkInput      *input,
                            AkDuplicator *duplicator,
                            uint32_t      expectedCount) {
  AkDuplicatorRange *range;
  AkIndexArray      *dupc, *dupcsum;
  AkAccessor        *sourceAcc, *derivedAcc;
  AkBuffer          *sourceBuffer, *derivedBuffer;
  AkInput           *derivedInput;
  const uint8_t     *source;
  uint8_t           *dest;
  size_t             sourceStride, rowSize, originalCount, destLength;
  size_t             i, j, written;

  range        = duplicator ? duplicator->range : NULL;
  dupc         = range ? range->dupc : NULL;
  dupcsum      = range ? range->dupcsum : NULL;
  sourceAcc    = input ? input->accessor : NULL;
  sourceBuffer = sourceAcc ? sourceAcc->buffer : NULL;
  if (!heap || !parent || !dupc || !dupcsum || !sourceAcc
      || !sourceBuffer || !sourceBuffer->data || dupc->count % 3u != 0u
      || expectedCount == 0u)
    return NULL;

  originalCount = dupc->count / 3u;
  rowSize       = sourceAcc->fillByteSize;
  sourceStride  = sourceAcc->byteStride ? sourceAcc->byteStride : rowSize;
  if (originalCount == 0u
      || sourceAcc->count != originalCount
      || rowSize == 0u
      || sourceStride < rowSize
      || rowSize > sourceBuffer->length
      || sourceAcc->byteOffset > sourceBuffer->length
      || (originalCount - 1u)
           > (SIZE_MAX - sourceAcc->byteOffset) / sourceStride
      || sourceAcc->byteOffset
           + (originalCount - 1u) * sourceStride
           > sourceBuffer->length - rowSize
      || expectedCount > SIZE_MAX / rowSize)
    return NULL;

  destLength = (size_t)expectedCount * rowSize;
  derivedInput = ak_heap_calloc(heap, parent, sizeof(*derivedInput));
  derivedAcc   = ak_heap_calloc(heap, derivedInput, sizeof(*derivedAcc));
  derivedBuffer = ak_heap_calloc(heap, derivedAcc, sizeof(*derivedBuffer));
  dest = ak_heap_calloc(heap, derivedBuffer, destLength);
  if (!derivedInput || !derivedAcc || !derivedBuffer || !dest)
    return NULL;

  memcpy(derivedInput, input, sizeof(*derivedInput));
  derivedInput->next     = NULL;
  derivedInput->reserved = NULL;
  derivedInput->accessor = derivedAcc;

  memcpy(derivedAcc, sourceAcc, sizeof(*derivedAcc));
  ak_setypeid(derivedAcc, AKT_ACCESSOR);
  derivedAcc->next       = NULL;
  derivedAcc->buffer     = derivedBuffer;
  derivedAcc->min        = NULL;
  derivedAcc->max        = NULL;
  derivedAcc->byteOffset = 0u;
  derivedAcc->byteStride = rowSize;
  derivedAcc->byteLength = destLength;
  derivedAcc->count      = expectedCount;

  derivedBuffer->data   = dest;
  derivedBuffer->length = destLength;
  source = (const uint8_t *)sourceBuffer->data + sourceAcc->byteOffset;
  written = 0u;

  for (i = 0; i < originalCount; i++) {
    AkUInt pno, duplicates, sourceOrdinal, prefix;

    pno           = ak_morphInspect_indexValue(dupc, 3u * i);
    duplicates    = ak_morphInspect_indexValue(dupc, 3u * i + 1u);
    sourceOrdinal = ak_morphInspect_indexValue(dupc, 3u * i + 2u);
    if (sourceOrdinal == 0u)
      continue;
    if (pno == UINT32_MAX || duplicates == UINT32_MAX
        || sourceOrdinal == UINT32_MAX || sourceOrdinal > originalCount
        || pno >= dupcsum->count)
      return NULL;

    prefix = ak_morphInspect_indexValue(dupcsum, pno);
    if (prefix == UINT32_MAX)
      return NULL;

    for (j = 0; j <= duplicates; j++) {
      size_t destIndex;

      if ((size_t)pno > SIZE_MAX - j
          || (size_t)pno + j > SIZE_MAX - prefix)
        return NULL;
      destIndex = (size_t)pno + j + prefix;
      if (destIndex >= expectedCount)
        return NULL;

      memcpy(dest + destIndex * rowSize,
             source + ((size_t)sourceOrdinal - 1u) * sourceStride,
             rowSize);
      written++;
    }
  }

  return written == expectedCount ? derivedInput : NULL;
}

static
AkInput *
ak_morphInspect_expandInputs(AkHeap       *heap,
                             void         *parent,
                             AkInput      *inputs,
                             AkDuplicator *duplicator,
                             uint32_t      expectedCount) {
  AkInput *head, *last, *input;

  head = last = NULL;
  for (input = inputs; input; input = input->next) {
    AkInput *derived;

    derived = ak_morphInspect_expandInput(heap,
                                          parent,
                                          input,
                                          duplicator,
                                          expectedCount);
    if (!derived)
      continue;
    AK_APPEND_FLINK(head, last, derived);
  }

  return head;
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
  AkDoc                      *doc;
  AkMorphInspectView         *view;
  AkMorphInspectTargetView   *tv,        *lastTV;
  AkMorphInspectMorphable    *m,         *lastM;
  AkMorphInspectMorphable    *baseM,     *mIter;
  AkMorphInspectInput        *lastInput;
  AkMorphTarget              *target;
  AkObject                   *gdataObj,  *targetObj;
  AkMesh                     *mesh,      *targetMesh;
  AkMeshPrimitive            *prim,      *basePrim, *targetPrim;
  AkInput                    *inp,       *posInp;
  AkAccessor                 *acc;
  AkMorphable                *morphable;
  AkGeometry                 *targetGeom;
  AkFloatArray               *iw;
  void                       *targetPtr;
  uint32_t                    primIdx,   primCount;
  uint32_t                    baseInputsCount;
  uint32_t                    targetIdx;
  uint32_t                    primVertCount, targetVertCount;
  uint32_t                    primStride,    targetStride;
  uint32_t                    expectedCount, i;
  size_t                      baseBufOff, targetBufOff, tvSize;
  bool                        inBase;

  if (!baseMesh || !morph) return AK_ERR;

  heap = ak_heap_getheap(morph);
  doc  = ak_heap_data(heap);
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

  baseInputsCount = 0;
  lastM           = NULL;
  baseBufOff      = 0;

  primIdx = 0;
  for (prim = mesh->primitive; prim && primIdx < primCount;
       prim = prim->next, primIdx++) {
    m = ak_morphInspect_appendMorphable(heap, tv, &lastM, 0.0f);
    if (!prim->pos || !(acc = prim->pos->accessor)) continue;

    primVertCount = (uint32_t)acc->count;
    if (primVertCount == 0) continue;

    m->vertexCount = primVertCount;
    lastInput      = NULL;

    primStride = 0;
    for (inp = prim->input; inp; inp = inp->next) {
      if (!ak_morphInspect_isDesired(inp->semantic, desiredInputs,
                                     desiredInputsCount))
        continue;
      if (!(acc = inp->accessor) || acc->count != primVertCount)
        continue;

      ak_morphInspect_appendInput(heap, m, &lastInput, inp, true);
      primStride += (uint32_t)acc->fillByteSize;
      baseInputsCount++;
    }

    m->stridePerVertex = primStride;
    m->bufferOffset    = baseBufOff;
    m->bufferSize      = (size_t)primStride * primVertCount;
    baseBufOff        += m->bufferSize;

    m->lastInput = lastInput;

  }

  if (baseInputsCount == 0) return AK_ERR;

  /*========================================================================*/
  /* Phase 3: build TargetView per AkMorphTarget                            */
  /*========================================================================*/

  lastTV    = NULL;
  targetIdx = 0;

  for (target = morph->target; target;
       target = target->next, targetIdx++) {
    tv    = ak_morphInspect_appendTargetView(heap, view, &lastTV);
    lastM = NULL;

    targetBufOff = 0;
    baseM        = view->base ? view->base->morphable : NULL;

    /* Preserve the authored target slot even when its URI/object could not
       be resolved.  Runtime weights and animation indices address slots, not
       merely the subset of targets with usable POSITION data. */
    if (!(targetObj = target->target)
        || !(targetPtr = ak_objGet(targetObj)))
      continue;

    /* polymorphic dispatch on AkMorphableType (kept as enum slot in
       AkObject.type — see controller.h) */
    switch (targetObj->type) {
      case AK_MORPHABLE_MORPHABLE: {
        /* glTF-style: AkMorphable chain, one per primitive. Walk
           the target's morphable chain in lockstep with the base
           morphables so each target primitive gets sized against
           its corresponding base primitive's vertex count rather
           than a single global anchor. */
        morphable = targetPtr;

        for (primIdx = 0; primIdx < primCount; primIdx++) {
          expectedCount = baseM ? baseM->vertexCount : 0;

          m = ak_morphInspect_appendMorphable(
              heap, tv, &lastM,
              ak_morphInspect_initialWeight(morph, mesh, targetIdx));
          m->vertexCount = expectedCount;
          m->bufferOffset = targetBufOff;

          if (!morphable
              || !(posInp = ak_morphInspect_findPosition(morphable->input))
              || !(acc = posInp->accessor)
              || (expectedCount > 0 && (uint32_t)acc->count != expectedCount))
            goto stepBaseM_a;
          targetVertCount = (uint32_t)acc->count;

          m->vertexCount = targetVertCount;
          lastInput      = NULL;

          targetStride = 0;
          for (inp = morphable->input; inp; inp = inp->next) {
            if (!ak_morphInspect_isDesired(inp->semantic, desiredInputs,
                                           desiredInputsCount))
              continue;
            if (!(acc = inp->accessor) || acc->count != targetVertCount)
              continue;

            inBase = ak_morphInspect_inBase(inp, baseM);
            if (ignoreUncommonInputs && !inBase) continue;

            ak_morphInspect_appendInput(heap, m, &lastInput, inp, inBase);
            targetStride += (uint32_t)acc->fillByteSize;

          }

          m->stridePerVertex = targetStride;
          m->bufferOffset    = targetBufOff;
          m->bufferSize      = (size_t)targetStride * targetVertCount;
          targetBufOff      += m->bufferSize;
          m->lastInput       = lastInput;

stepBaseM_a:
          if (morphable) morphable = morphable->next;
          if (baseM) baseM = baseM->next;
        }
        break;
      }

      case AK_MORPHABLE_GEOMETRY: {
        /* DAE-style: pointer-storage payload — wrap holds an AkGeometry*
           in its inline slot. ak_objGetTarget hides the deref. */
        targetGeom = ak_objGetTarget(targetObj);
        targetMesh = NULL;
        if (targetGeom
            && (gdataObj = targetGeom->gdata)
            && gdataObj->type == AK_GEOMETRY_MESH)
          targetMesh = ak_objGet(gdataObj);

        basePrim   = mesh->primitive;
        targetPrim = targetMesh ? targetMesh->primitive : NULL;
        for (primIdx = 0; primIdx < primCount; primIdx++) {
          AkInput *targetInputs;

          expectedCount = baseM ? baseM->vertexCount : 0;

          m = ak_morphInspect_appendMorphable(
              heap, tv, &lastM,
              ak_morphInspect_initialWeight(morph, mesh, targetIdx));
          m->vertexCount = expectedCount;
          m->bufferOffset = targetBufOff;

          /* A COLLADA morph target geometry may legally contain only a
             mesh-wide <vertices> group and no render primitive.  Apply that
             vertex domain to each base primitive.  If primitives do exist,
             keep strict ordinal pairing so a missing POSITION leaves an
             empty placeholder instead of shifting later primitive slots. */
          targetInputs = targetPrim
                           ? targetPrim->input
                           : (targetMesh && !targetMesh->primitive
                              && targetMesh->vertices
                                ? targetMesh->vertices->input
                                : NULL);

          posInp = ak_morphInspect_findPosition(targetInputs);
          if (!targetPrim
              && targetMesh
              && !targetMesh->primitive
              && posInp
              && posInp->accessor
              && expectedCount > 0
              && posInp->accessor->count != expectedCount) {
            AkDuplicator *duplicator;

            duplicator = doc && basePrim
                           ? ak__docDuplicatorFind(doc, basePrim)
                           : NULL;
            targetInputs = ak_morphInspect_expandInputs(heap,
                                                        m,
                                                        targetInputs,
                                                        duplicator,
                                                        expectedCount);
            posInp = ak_morphInspect_findPosition(targetInputs);
          }

          if (!posInp || !(acc = posInp->accessor)
              || (expectedCount > 0 && (uint32_t)acc->count != expectedCount))
            goto stepBaseM_b;
          targetVertCount = (uint32_t)acc->count;

          m->vertexCount = targetVertCount;
          lastInput      = NULL;

          targetStride = 0;
          for (inp = targetInputs; inp; inp = inp->next) {
            if (!ak_morphInspect_isDesired(inp->semantic, desiredInputs,
                                           desiredInputsCount))
              continue;
            if (!(acc = inp->accessor) || acc->count != targetVertCount)
              continue;

            inBase = ak_morphInspect_inBase(inp, baseM);
            if (ignoreUncommonInputs && !inBase) continue;

            ak_morphInspect_appendInput(heap, m, &lastInput, inp, inBase);
            targetStride += (uint32_t)acc->fillByteSize;

          }

          m->stridePerVertex = targetStride;
          m->bufferOffset    = targetBufOff;
          m->bufferSize      = (size_t)targetStride * targetVertCount;
          targetBufOff      += m->bufferSize;
          m->lastInput       = lastInput;

stepBaseM_b:
          if (basePrim) basePrim = basePrim->next;
          if (targetPrim) targetPrim = targetPrim->next;
          if (baseM) baseM = baseM->next;
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
  /* Phase 5: target slice sizes                                            */
  /*========================================================================*/

  /* Per-morphable bufferSize is the source of truth — sum across
     a target's morphables to get its slice size, then sum across
     targets for the whole-buffer size. */
  view->interleaveTotalBufferSize = 0;
  for (tv = view->targets; tv; tv = tv->next) {
    tvSize = 0;
    for (mIter = tv->morphable; mIter; mIter = mIter->next) {
      tvSize += mIter->bufferSize;
    }
    tv->interleaveBufferSize = tvSize;
    view->interleaveTotalBufferSize += tvSize;
  }

  /* convenience cache: per-blend-shape initial weights for runtime
     (length = morph->targetCount; base shape is NOT included here) */
  if (morph->targetCount > 0) {
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
  AkMorphInspectMorphable   *baseMorph;
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

  /* Layout model:
   *
   *   destBuff = [target0 slice][target1 slice][target2 slice] ...
   *      target slice = [morphable0 sub-slice][morphable1 sub-slice] ...
   *           sub-slice = vertexCount × stridePerVertex bytes
   *
   * `intrOffset` for each input is the byte offset *within* its
   * morphable's stride row (0 .. morphable.stridePerVertex). The
   * morphable's `bufferOffset` (set during inspect) places the
   * sub-slice within its target's slice. The interleave pass
   * advances `dst` by `targetView->interleaveBufferSize` per
   * target, then for each morphable computes
   *   `dst_morph = dst_target + morphable.bufferOffset`
   * and writes inputs at
   *   `dst_morph + input.intrOffset + stride*k`.
   *
   * P1P2N1N2 = within each morphable, walk inputs grouped by
   *            base-input order (POSITION → NORMAL → TANGENT, ...)
   * NATURAL  = within each morphable, walk inputs in authored
   *            order.
   */
  switch (layout) {
    case AK_MORPH_P1P2N1N2: {
      for (targetView = inspectView->targets;
           targetView;
           targetView = targetView->next) {
        baseMorph     = inspectView->base ? inspectView->base->morphable : NULL;
        inspMorphable = targetView->morphable;
        while (inspMorphable) {
          inpOff = 0;

          /* Use the paired base morphable's input ordering as the
             template, when available; this gives target inputs
             offsets matching base order. */
          if (baseMorph) {
            for (tinp = baseMorph->input; tinp; tinp = tinp->next) {
              if (!tinp->inTarget && !includeBaseShape) continue;
              for (tinpt = inspMorphable->input; tinpt; tinpt = tinpt->next) {
                if (!(tinpt->input->semantic == tinp->input->semantic
                      && tinpt->input->set == tinp->input->set)
                    || (ignoreUncommonInputs && !tinpt->inBaseMesh))
                  continue;

                inp               = tinpt->input;
                acc               = inp->accessor;
                tinpt->intrOffset = inpOff;
                inpOff           += (uint32_t)acc->fillByteSize;
                break;  /* one target input assigned per base input */
              }
            }
          }

          /* Trailing target-only inputs (rare). */
          if (!ignoreUncommonInputs) {
            for (tinpt = inspMorphable->input; tinpt; tinpt = tinpt->next) {
              if (tinpt->inBaseMesh) continue;
              inp                = tinpt->input;
              acc                = inp->accessor;
              tinpt->intrOffset  = inpOff;
              inpOff            += (uint32_t)acc->fillByteSize;
            }
          }

          inspMorphable = inspMorphable->next;
          if (baseMorph) baseMorph = baseMorph->next;
        }
      }
      inspectView->layout = layout;
      return AK_OK;
    }

    case AK_MORPH_NATURAL: {
      /* Per-morphable scoped offsets in authored input order. */
      for (targetView = inspectView->targets;
           targetView;
           targetView = targetView->next) {
        for (inspMorphable = targetView->morphable;
             inspMorphable;
             inspMorphable = inspMorphable->next) {
          inpOff = 0;
          for (tinp = inspMorphable->input; tinp; tinp = tinp->next) {
            inp              = tinp->input;
            acc              = inp->accessor;
            tinp->intrOffset = inpOff;
            inpOff          += (uint32_t)acc->fillByteSize;
          }
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

static
AkResult
ak_morphInterleaveInternal(AkGeometry         * __restrict baseMesh,
                           AkMorph            * __restrict morph,
                           AkMorphInterleaveLayout         layout,
                           void               * __restrict destBuff,
                           AkMorphProgressFn                progress,
                           void               * __restrict userdata) {
  AkMorphInspectView        *morphView;
  AkMorphInspectTargetView  *targetView;
  AkMorphInspectMorphable   *inspMorphable;
  AkMorphInspectInput       *tinp;
  AkInput                   *inp;
  AkAccessor                *acc;
  AkBuffer                  *buf;
  char                      *src,        *dst;
  char                      *targetDst,  *morphDst;
  char                      *sp,         *dp;
  uint32_t                   srcStride,  compSize;
  uint32_t                   k,          intrOffset;
  uint32_t                   mStride,    mCount;
  uint32_t                   targetIdx;

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
  targetIdx = 0;

  for (; targetView; targetView = targetView->next) {
    if (progress && !progress(morph, targetIdx, morphView->nTargets, userdata))
      return AK_ERR;

    /* Per morphable: walk inputs, write at the morphable's
       sub-slice using its own per-primitive stride and vertex
       count. */
    targetDst = dst;
    for (inspMorphable = targetView->morphable;
         inspMorphable;
         inspMorphable = inspMorphable->next) {
      tinp = inspMorphable->input;
      if (!tinp)
        continue;

      morphDst = targetDst + inspMorphable->bufferOffset;
      mStride  = inspMorphable->stridePerVertex;
      mCount   = inspMorphable->vertexCount;

      for (; tinp
             && (inp = tinp->input)
             && (acc = inp->accessor)
             && (buf = acc->buffer)
             && (src = (char *)buf->data + acc->byteOffset);
           tinp = tinp->next) {
        srcStride  = (uint32_t)acc->byteStride;
        compSize   = (uint32_t)acc->fillByteSize;
        intrOffset = tinp->intrOffset;

        dp = morphDst + intrOffset;
        sp = src;
        switch (compSize) {
          case 4:
            for (k = 0; k < mCount; k++) {
              memcpy(dp, sp, 4);
              dp += mStride;
              sp += srcStride;
            }
            break;
          case 8:
            for (k = 0; k < mCount; k++) {
              memcpy(dp, sp, 8);
              dp += mStride;
              sp += srcStride;
            }
            break;
          case 12:
            for (k = 0; k < mCount; k++) {
              memcpy(dp, sp, 12);
              dp += mStride;
              sp += srcStride;
            }
            break;
          case 16:
            for (k = 0; k < mCount; k++) {
              memcpy(dp, sp, 16);
              dp += mStride;
              sp += srcStride;
            }
            break;
          default:
            for (k = 0; k < mCount; k++) {
              memcpy(dp, sp, compSize);
              dp += mStride;
              sp += srcStride;
            }
            break;
        }
      }
    }

    /* Advance to the next target's slice. */
    dst += targetView->interleaveBufferSize;
    targetIdx++;
  }

  if (progress && !progress(morph, targetIdx, morphView->nTargets, userdata))
    return AK_ERR;

  return AK_OK;
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
  return ak_morphInterleaveInternal(baseMesh,
                                    morph,
                                    layout,
                                    destBuff,
                                    NULL,
                                    NULL);
}

AK_EXPORT
AkResult
ak_morphInterleaveWithProgress(AkGeometry         * __restrict baseMesh,
                               AkMorph            * __restrict morph,
                               AkMorphInterleaveLayout         layout,
                               void               * __restrict destBuff,
                               AkMorphProgressFn                progress,
                               void               * __restrict userdata) {
  return ak_morphInterleaveInternal(baseMesh,
                                    morph,
                                    layout,
                                    destBuff,
                                    progress,
                                    userdata);
}
