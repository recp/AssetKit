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

#include "ctlr.h"

static
AkResult
ak_fixBoneWeights(AkHeap        *heap,
                  size_t         nMeshVertex,
                  AkSkin        *skin,
                  AkDuplicator  *duplicator,
                  AkBoneWeights *intrWeights,
                  AkBoneWeights *weights,
                  AkAccessor    *weightsAcc,
                  uint32_t       jointOffset,
                  uint32_t       weightsOffset);

AK_HIDE
void
dae_fixup_ctlr(DAEState * __restrict dst) {
  AkDoc        *doc;
  AkController *ctlr;

  doc  = dst->doc;
  ctlr = (void *)dst->doc->lib.controllers->chld;
  while (ctlr) {
    switch (ctlr->type) {
      case AK_CONTROLLER_SKIN: {
        AkSkin     *skin;
        AkSkinDAE  *skindae;
        AkGeometry *geom;

        skin    = ctlr->data;
        skindae = ak_userData(skin);
        if (!(geom = ak_baseGeometry(&skindae->baseGeom)))
          goto nxt_ctlr;

        switch (geom->gdata->type) {
          case AK_GEOMETRY_MESH: {
            AkMesh          *mesh;
            AkDaeMeshInfo   *meshInfo;
            AkMeshPrimitive *prim;
            AkBoneWeights   *intrWeights; /* interleaved */
            AkInput         *jointswInp,  *weightsInp;
            AkAccessor      *weightsAcc;
            size_t           nMeshVertex;
            uint32_t         primIndex;

            mesh          = ak_objGet(geom->gdata);
            prim          = mesh->primitive;
            intrWeights   = (void *)skin->weights;
            primIndex     = 0;
            meshInfo      = rb_find(dst->meshInfo, mesh);
            skin->weights = ak_heap_alloc(dst->heap,
                                          ctlr->data,
                                          sizeof(void *)
                                          * mesh->primitiveCount);

            jointswInp  = skindae->weights.joints;
            weightsInp  = skindae->weights.weights;
            weightsAcc  = weightsInp->accessor;
            nMeshVertex = meshInfo->nVertex;

            flist_sp_insert(&mesh->skins, skin);

            while (prim) {
              AkAccessor    *posAcc;
              AkBoneWeights *weights; /* per-primitive weights */
              AkDuplicator  *dupl;
              size_t         count;

              posAcc  = prim->pos->accessor;
              count   = GLM_MAX(posAcc->count, 1);
              dupl    = rb_find(doc->reserved, prim);
              weights = ak_heap_calloc(dst->heap, ctlr->data, sizeof(*weights));

              weights->counts  = ak_heap_alloc(dst->heap,
                                               ctlr->data,
                                               count * sizeof(uint32_t));
              weights->indexes = ak_heap_alloc(dst->heap,
                                               ctlr->data,
                                               count * sizeof(size_t));

              weights->nVertex = count;

              ak_fixBoneWeights(dst->heap,
                                nMeshVertex,
                                skin,
                                dupl,
                                intrWeights,
                                weights,
                                weightsAcc,
                                jointswInp->offset,
                                weightsInp->offset);

              skin->weights[primIndex] = weights;
              primIndex++;
              prim = prim->next;
            }

            skin->nPrims = primIndex;

            ak_free(intrWeights);

            break;
          }
          default:
            break;
        }
        break;
      }
      case AK_CONTROLLER_MORPH: {
        AkMorph       *morph;
        AkMorphDAE    *morphdae;
        AkGeometry    *baseGeom;
        AkInput       *input, *targetInput, *weightInput;
        AkAccessor    *targetAcc, *weightAcc;
        AkBuffer      *targetBuff, *weightBuff;
        AkMorphTarget *prevTarget;
        uint32_t       basePrimCount;
        size_t         count, i;
        AkContext      sidCtx = { .doc = doc };

        morph    = ctlr->data;
        morphdae = ak_userData(morph);
        if (!(baseGeom = ak_baseGeometry(&morphdae->baseGeom)))
          goto nxt_ctlr;

        /* DAE morph has exactly one MORPH_TARGET input (a NAME/IDREF/SIDREF
           array of per-target geometries) and one MORPH_WEIGHT input (a
           float array of default weights). Split the chain so we can walk
           targets while still having the weights handy for defaultWeights. */
        targetInput = weightInput = NULL;
        for (input = morphdae->input; input; input = input->next) {
          if      (input->semantic == AK_INPUT_MORPH_TARGET) targetInput = input;
          else if (input->semantic == AK_INPUT_MORPH_WEIGHT) weightInput = input;
        }
        if (!targetInput || !(targetAcc = targetInput->accessor)
            || !(targetBuff = targetAcc->buffer)
            || !targetBuff->data
            || targetAcc->count == 0)
          goto nxt_ctlr;

        /* Per AkMorphTarget, primitiveCount mirrors the base mesh — DAE
           assumes target geometries share the base topology (same prim
           layout, same vertex count). Compute once. */
        basePrimCount = (baseGeom->gdata
                         && baseGeom->gdata->type == AK_GEOMETRY_MESH)
                          ? ((AkMesh *)ak_objGet(baseGeom->gdata))->primitiveCount
                          : 1;

        /* Build AkMorphTarget chain in source order (tail-insert). DAE
           animation channels reference targets via position — preserving
           parse order keeps weight indices aligned with the source <Name>
           array. */
        count      = targetAcc->count;
        prevTarget = NULL;

        for (i = 0; i < count; i++) {
          const char    *id;
          void          *resolved;
          AkObject      *wrap;
          AkMorphTarget *target;
          const char   **idArr;

          /* Source array entries are typed by the accessor's componentType.
             COLLADA 1.4/1.5 spec lists IDREF and Name as the typical morph
             target source types; SIDREF is technically allowed via the
             generic <param> mechanism. */
          idArr = targetBuff->data;
          if (!(id = idArr[i])) continue;

          switch (targetAcc->componentType) {
            case AKT_IDREF:
            case AKT_NAME:
              resolved = ak_getObjectById(doc, id);
              break;
            case AKT_SIDREF:
              resolved = ak_sid_resolve_from(&sidCtx, doc, id, NULL);
              break;
            default:
              resolved = NULL;
              break;
          }
          if (!resolved || ak_typeid(resolved) != AKT_GEOMETRY)
            continue;

          /* AkObject wrap carries the type tag used by intr.c switch
             dispatch. Payload is one pointer-sized slot storing the
             AkGeometry* — read back via ak_objGetTarget on C/Swift sides.
             The geometry itself stays in doc->lib.geometries with its own
             lifetime; this wrap just references it. */
          wrap = ak_objAlloc(dst->heap, morph,
                             sizeof(AkGeometry *),
                             AK_MORPHABLE_GEOMETRY,
                             true);
          ak_objGetTarget(wrap) = (AkGeometry *)resolved;

          target                 = ak_heap_calloc(dst->heap, morph,
                                                  sizeof(*target));
          target->target         = wrap;
          target->primitiveCount = basePrimCount;

          if (prevTarget) prevTarget->next = target;
          else            morph->target    = target;
          prevTarget = target;
          morph->targetCount++;
        }

        if (morph->targetCount == 0)
          goto nxt_ctlr;

        /* MORPH_WEIGHT → defaultWeights. Spec requires the weight array's
           length to match the target count; if it mismatches we still take
           min(count, targetCount) so partial assets don't silently drop
           on the floor. */
        if (weightInput
            && (weightAcc  = weightInput->accessor)
            && (weightBuff = weightAcc->buffer)
            && weightBuff->data
            && weightAcc->count > 0) {
          AkFloatArray *defaults;
          float        *src;
          size_t        nWeights;

          nWeights = weightAcc->count < morph->targetCount
                       ? weightAcc->count
                       : morph->targetCount;

          defaults = ak_heap_alloc(dst->heap, morph,
                                   sizeof(*defaults)
                                    + sizeof(float) * nWeights);
          defaults->count = nWeights;

          /* Weights are stored densely in the source's accessor; respect
             stride for safety (DAE float source from <float_array> is
             typically tightly packed but technically allowed to be
             interleaved within its <source>). */
          src = weightBuff->data;
          if (weightAcc->byteStride
              && weightAcc->byteStride != sizeof(float)) {
            char *base = (char *)src + weightAcc->byteOffset;
            for (i = 0; i < nWeights; i++) {
              defaults->items[i] =
                *(float *)(base + i * weightAcc->byteStride);
            }
          } else {
            src = (float *)((char *)src + weightAcc->byteOffset);
            for (i = 0; i < nWeights; i++)
              defaults->items[i] = src[i];
          }

          morph->defaultWeights = defaults;
        }

        /* Register geom→morph for instance hookup later (DAE node walker
           reads ctlrMorphMap to attach AkInstanceMorph when it sees an
           <instance_controller> referring to a morph controller). */
        rb_insert(dst->ctlrMorphMap, baseGeom, morph);

        break;
      }
      default:
        break;
    }

  nxt_ctlr:
    ctlr = (AkController *)ctlr->base.next;
  }
}

AK_HIDE
void
dae_fixup_instctlr(DAEState * __restrict dst) {
  AkSkinDAE            *skindae;
  FListItem            *item;
  AkInstanceController *instCtlr;
  AkController         *ctlr;
  AkNode               *node;
  AkInstanceGeometry   *instGeom;
  AkContext             ctx = { .doc = dst->doc };

  item = dst->instCtlrs;
  while (item) {
    AkMorphDAE *morphdae;

    instCtlr = item->data;
    ctlr     = ak_instanceObject(&instCtlr->base);
    node     = instCtlr->base.node;
    instGeom = ak_heap_calloc(dst->heap, node, sizeof(*instGeom));

    switch (ctlr->type) {
      case AK_CONTROLLER_SKIN: {
        AkInstanceSkin *instSkin;
        AkSkin         *skin;
        AkNode        **joints;
        AkInput        *jointsInp,  *matrixInp;
        AkAccessor     *jointsAcc,  *matrixAcc;
        AkBuffer       *jointsBuff, *matrixBuff;
        FListItem      *skel;
        const char     *sid, **it;
        AkFloat        *mit;
        AkFloat4x4     *invm;
        size_t          count, i;

        skin      = ctlr->data;
        skindae   = ak_userData(skin);
        instSkin  = ak_heap_calloc(dst->heap, node, sizeof(*instSkin));

        skin      = ctlr->data;
        jointsInp = skindae->joints.joints;
        matrixInp = skindae->joints.invBindMatrix;
        invm      = NULL;
        joints    = NULL;

        if ((jointsAcc = jointsInp->accessor)) {
          matrixAcc  = matrixInp->accessor;
          jointsBuff = jointsAcc->buffer;
          matrixBuff = matrixAcc->buffer;

          it         = jointsBuff->data;
          mit        = matrixBuff->data;
          count      = jointsAcc->count;
          joints     = ak_heap_alloc(dst->heap, instCtlr, sizeof(void **) * count);
          invm       = ak_heap_alloc(dst->heap, ctlr->data, sizeof(mat4) * count);

          for (i = 0; i < count; i++) {
            if (!(sid = it[i]))
              continue;

            switch (jointsAcc->componentType) {
              case AKT_IDREF:
                joints[i] = ak_getObjectById(dst->doc, sid);
                break;
              case AKT_SIDREF:
              case AKT_NAME:
                if ((skel = instCtlr->reserved)) {
                  do {
                    if ((joints[i] = ak_sid_resolve_from(&ctx, skel->data, sid, NULL)))
                      break;
                  } while ((skel = skel->next));
                }
                break;
              default:
                break;
            }

            /* move invBindMatrix to new location */
            memcpy(invm[i], mit + 16 * i, sizeof(AkFloat) * 16);
            glm_mat4_transpose(invm[i]);
          }

          skin->nJoints            = count;
          skin->invBindPoses       = invm;

          instSkin->skin           = skin;
          instSkin->overrideJoints = joints;

          /* COLLADA permits chained controllers — skin's source can be
             another controller (typically a morph from Maya):
                 <skin source="#someMorph"/>
                 <morph source="#baseGeom"/>
             Single-level lookahead is enough: AkInstanceGeometry has
             one morpher + one skinner slot, and no GPU renderer
             (SceneKit / Three.js / Filament / typical Metal/Vulkan
             pipelines) consumes a deeper controller stack — the spec's
             "arbitrary recursion" was never adopted in practice.
             ak_baseGeometry() collapses the rest of the chain when
             resolving `base.object`. */
          {
            void *src = ak_getObjectByUrl(&skindae->baseGeom);
            if (src && ak_typeid(src) == AKT_CONTROLLER) {
              AkController *intermediate = src;
              if (intermediate->type == AK_CONTROLLER_MORPH) {
                AkMorphDAE      *morphdae2;
                AkInstanceMorph *instMorph;
                morphdae2 = ak_userData(intermediate->data);
                instMorph = ak_heap_calloc(dst->heap, node,
                                           sizeof(*instMorph));
                instMorph->morph           = intermediate->data;
                instMorph->overrideWeights = NULL;
                instGeom->morpher          = instMorph;
                (void)morphdae2;
              }
              /* skin→skin nesting is exotic and the data model can't
                 represent two skinners — silently use the outer one. */
            }
            instGeom->base.object = ak_baseGeometry(&skindae->baseGeom);
          }

          /* create instance geometry for skin */
          instGeom->skinner        = instSkin;
          instGeom->bindMaterial   = instCtlr->bindMaterial;
          ak_heap_setpm(instCtlr->bindMaterial, instGeom);

          instGeom->base.next = (AkInstanceBase *)node->geometry;
          if (node->geometry)
            node->geometry->base.prev = (AkInstanceBase *)instGeom;
          node->geometry = instGeom;
        }
        break;
      }
      case AK_CONTROLLER_MORPH: {
        AkInstanceMorph *instMorph;
        AkMorph         *morph;

        morph     = ctlr->data;
        morphdae  = ak_userData(morph);
        instMorph = ak_heap_calloc(dst->heap, node, sizeof(*instMorph));

        instMorph->morph           = morph;
        instMorph->overrideWeights = NULL; /* DAE has no per-instance weights;
                                              animation drives morph.targets   */

        instGeom->morpher          = instMorph;
        instGeom->bindMaterial     = instCtlr->bindMaterial;

        /* Symmetric to SKIN case: morph→skin→geom is rare but spec-allowed.
           Single-level lookahead; deeper chains collapse via ak_baseGeometry. */
        {
          void *src = ak_getObjectByUrl(&morphdae->baseGeom);
          if (src && ak_typeid(src) == AKT_CONTROLLER) {
            AkController *intermediate = src;
            if (intermediate->type == AK_CONTROLLER_SKIN) {
              AkInstanceSkin *instSkin = ak_heap_calloc(dst->heap, node,
                                                        sizeof(*instSkin));
              instSkin->skin           = intermediate->data;
              instSkin->overrideJoints = NULL; /* picked up from default joints */
              instGeom->skinner        = instSkin;
            }
          }
        }
        instGeom->base.object = ak_baseGeometry(&morphdae->baseGeom);
        ak_heap_setpm(instCtlr->bindMaterial, instGeom);

        instGeom->base.next = (AkInstanceBase *)node->geometry;
        if (node->geometry)
          node->geometry->base.prev = (AkInstanceBase *)instGeom;
        node->geometry = instGeom;
        break;
      }
      default: break;
    }
    item = item->next;
  }
}

static
AkResult
ak_fixBoneWeights(AkHeap        *heap,
                  size_t         nMeshVertex,
                  AkSkin        *skin,
                  AkDuplicator  *duplicator,
                  AkBoneWeights *intrWeights,
                  AkBoneWeights *weights,
                  AkAccessor    *weightsAcc,
                  uint32_t       jointOffset,
                  uint32_t       weightsOffset) {
  AkSkinDAE    *skindae;
  AkBoneWeight *w, *iw;
  AkBuffer     *weightsBuff;
  AkUIntArray  *dupc, *dupcsum, *v;
  float        *pWeights;
  uint32_t     *pv, *pOldCount, *pOldCountSum;
  size_t       *wi, vc, d, s, pno, poo, nwsum, newidx, next, tmp;
  uint32_t     *nj, i, j, k, vcount, count, viStride;

  dupc    = duplicator->range->dupc;
  dupcsum = duplicator->range->dupcsum;
  vc      = nMeshVertex;
  nj      = weights->counts;
  wi      = weights->indexes;
  skindae = ak_userData(skin);
  nwsum   = count = 0;

  if (!(weightsBuff = weightsAcc->buffer)
      || !(pWeights = weightsBuff->data)
      || !(v        = skindae->weights.v)
      || !(pv       = v->items))
    return AK_ERR;

#ifdef DEBUG
  assert(nMeshVertex == intrWeights->nVertex);
#endif

  pOldCount    = intrWeights->counts;
  pOldCountSum = intrWeights->counts + intrWeights->nVertex;
  viStride     = skindae->inputCount; /* input count in <v> element */

  /* copy to new location and duplicate if needed */
  for (i = 0; i < vc; i++) {
    if ((poo = dupc->items[3 * i + 2]) == 0)
      continue;

    pno    = dupc->items[3 * i];
    d      = dupc->items[3 * i + 1];
    s      = dupcsum->items[pno];
    vcount = pOldCount[poo - 1];

    for (j = 0; j <= d; j++) {
      newidx     = pno + j + s;
      wi[newidx] = vcount;
      nj[newidx] = vcount;
    }

    nwsum += vcount * (d + 1);
    count++;
  }

  /* prepare weight index */
  for (next = j = 0; j < weights->nVertex; j++) {
    tmp   = wi[j];
    wi[j] = next;
    next  = tmp + next;
  }

  /* now we know the size of arrays: weights, pJointsCount, npWeightsIndex */
  w     = ak_heap_alloc(heap, weights, sizeof(*w) * nwsum);
  nwsum = 0;

  for (i = 0; i < vc; i++) {
    uint32_t *old;

    if ((poo = dupc->items[3 * i + 2]) == 0)
      continue;

    pno    = dupc->items[3 * i];
    d      = dupc->items[3 * i + 1];
    s      = dupcsum->items[pno];
    vcount = pOldCount[poo - 1];
    old    = &pv[pOldCountSum[poo - 1] * viStride];

    for (j = 0; j <= d; j++) {
      newidx = wi[pno + j + s];

      for (k = 0; k < vcount; k++) {
        iw         = &w[newidx + k];
        iw->joint  = old[k * viStride + jointOffset];
        iw->weight = pWeights[old[k * viStride + weightsOffset]];
      }
    }

    nwsum += vcount * (d + 1);
  }

  weights->weights  = w;
  weights->nWeights = nwsum;

  return AK_OK;
}
