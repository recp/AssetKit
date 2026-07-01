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

#include <string.h>

static
AkNode*
dae_find_node_in_tree(AkNode     * __restrict node,
                      const char * __restrict name,
                      bool                    sidFirst,
                      uint32_t                depth);

static
AkNode*
dae_resolve_skin_joint(DAEState   * __restrict dst,
                       FListItem  * __restrict skeletons,
                       const char * __restrict name,
                       AkTypeId                type);

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

AK_INLINE
uint32_t
ak_daeSafeWeightCount(AkBoneWeights * __restrict intrWeights,
                      AkUIntArray   * __restrict v,
                      uint32_t                   viStride,
                      size_t                     oldIdx) {
  uint32_t *pCount;
  uint32_t *pSum;
  size_t    off;
  size_t    avail;
  uint32_t  count;

  if (!intrWeights || !intrWeights->counts || !v || viStride == 0)
    return 0;
  if (oldIdx >= intrWeights->nVertex)
    return 0;

  pCount = intrWeights->counts;
  pSum   = intrWeights->counts + intrWeights->nVertex;
  off    = pSum[oldIdx];
  count  = pCount[oldIdx];

  if (off > v->count / viStride)
    return 0;

  avail = v->count / viStride - off;
  if (count > avail)
    count = (uint32_t)avail;

  return count;
}

AK_INLINE
float
ak_daeReadSkinWeight(AkAccessor * __restrict acc,
                     uint32_t                idx) {
  AkBuffer *buff;
  char     *base;
  size_t    stride;
  float     val;

  if (!acc || idx >= acc->count || !(buff = acc->buffer) || !buff->data)
    return 0.0f;

  stride = acc->byteStride ? acc->byteStride : sizeof(float);
  base   = (char *)buff->data + acc->byteOffset;

  memcpy(&val, base + (size_t)idx * stride, sizeof(val));

  return val;
}

static
AkNode*
dae_find_node_in_tree(AkNode     * __restrict node,
                      const char * __restrict name,
                      bool                    sidFirst,
                      uint32_t                depth) {
  AkNode *found;

  if (!name || depth > 512)
    return NULL;

  for (; node; node = node->next) {
    const char     *sid;
    const char     *id;
    AkInstanceNode *inst;

    sid = ak_sid_get(node);
    id  = ak_getId(node);

    if (sidFirst) {
      if (sid && strcmp(sid, name) == 0)
        return node;
    } else if (id && strcmp(id, name) == 0) {
      return node;
    }

    if ((found = dae_find_node_in_tree(node->chld, name, sidFirst, depth + 1)))
      return found;

    for (inst = node->node; inst; inst = inst->next) {
      AkNode *target;

      target = ak_instanceNodeTarget(inst);
      if ((found = dae_find_node_in_tree(target, name, sidFirst, depth + 1)))
        return found;
    }
  }

  return NULL;
}

static
AkNode*
dae_find_skin_joint_in_scope(AkDoc      * __restrict doc,
                             void       * __restrict scope,
                             const char * __restrict name,
                             AkTypeId                type) {
  AkNode *root;
  AkNode *found;

  if (!scope || !name)
    return NULL;

  switch (ak_typeid(scope)) {
    case AKT_SCENE:
      root = ((AkScene *)scope)->node;
      break;
    case AKT_NODE:
      root = scope;
      break;
    default:
      return NULL;
  }

  switch (type) {
    case AKT_NAME:
      if ((found = dae_find_node_in_tree(root, name, true, 0)))
        return found;
      if ((found = dae_find_node_in_tree(root, name, false, 0)))
        return found;
      break;
    case AKT_SIDREF:
      if ((found = dae_find_node_in_tree(root, name, true, 0)))
        return found;
      break;
    case AKT_IDREF:
      found = ak_getObjectById(doc, name);
      if (found && ak_typeid(found) == AKT_NODE)
        return found;
      break;
    default:
      break;
  }

  return NULL;
}

static
void*
dae_skeleton_scope(AkDoc * __restrict doc, const char * __restrict url) {
  const char *id;

  if (!doc || !url || !url[0])
    return NULL;

  id = url[0] == '#' ? url + 1 : url;
  return ak_getObjectById(doc, id);
}

static
AkNode*
dae_resolve_skin_joint(DAEState   * __restrict dst,
                       FListItem  * __restrict skeletons,
                       const char * __restrict name,
                       AkTypeId                type) {
  FListItem *skel;
  AkNode    *joint;

  if (!dst || !name)
    return NULL;

  if (type == AKT_IDREF) {
    void *resolved;

    resolved = ak_getObjectById(dst->doc, name);
    return resolved && ak_typeid(resolved) == AKT_NODE ? resolved : NULL;
  }

  for (skel = skeletons; skel; skel = skel->next) {
    void *scope;

    scope = dae_skeleton_scope(dst->doc, skel->data);
    if ((joint = dae_find_skin_joint_in_scope(dst->doc, scope, name, type)))
      return joint;
  }

  if (type == AKT_NAME) {
    void *resolved;

    resolved = ak_getObjectById(dst->doc, name);
    if (resolved && ak_typeid(resolved) == AKT_NODE)
      return resolved;
  }

  return NULL;
}

static
AkInstanceSkin*
dae_instance_skin_from_controller(DAEState             * __restrict dst,
                                  AkInstanceController * __restrict instCtlr,
                                  AkController         * __restrict ctlr,
                                  AkNode               * __restrict node) {
  AkSkin         *skin;
  AkSkinDAE      *skindae;
  AkInstanceSkin *instSkin;
  AkNode        **joints;
  AkInput        *jointsInp;
  AkInput        *matrixInp;
  AkAccessor     *jointsAcc;
  AkAccessor     *matrixAcc;
  AkBuffer       *jointsBuff;
  AkBuffer       *matrixBuff;
  AkFloat4x4     *invm;
  const char     *sid;
  char           *jointBase;
  char           *matrixBase;
  size_t          jointStride;
  size_t          matrixStride;
  size_t          count;
  size_t          i;

  if (!dst || !instCtlr || !ctlr || ctlr->type != AK_CONTROLLER_SKIN)
    return NULL;

  skin    = ctlr->data;
  skindae = skin ? ak_userData(skin) : NULL;
  if (!skin || !skindae)
    return NULL;

  jointsInp = skindae->joints.joints;
  matrixInp = skindae->joints.invBindMatrix;
  if (!jointsInp
      || !matrixInp
      || !(jointsAcc = jointsInp->accessor)
      || !(matrixAcc = matrixInp->accessor)
      || !(jointsBuff = jointsAcc->buffer)
      || !(matrixBuff = matrixAcc->buffer)
      || !jointsBuff->data
      || !matrixBuff->data
      || jointsAcc->count == 0
      || matrixAcc->count < jointsAcc->count)
    return NULL;

  count        = jointsAcc->count;
  jointStride  = jointsAcc->byteStride ? jointsAcc->byteStride
                                       : sizeof(const char *);
  matrixStride = matrixAcc->byteStride ? matrixAcc->byteStride
                                       : sizeof(AkFloat) * 16u;
  jointBase    = (char *)jointsBuff->data + jointsAcc->byteOffset;
  matrixBase   = (char *)matrixBuff->data + matrixAcc->byteOffset;

  joints = ak_heap_alloc(dst->heap, instCtlr, sizeof(*joints) * count);
  invm   = ak_heap_alloc(dst->heap, ctlr->data, sizeof(*invm) * count);
  if (!joints || !invm)
    return NULL;

  for (i = 0; i < count; i++) {
    sid = *(const char **)(jointBase + i * jointStride);
    joints[i] = sid
                  ? dae_resolve_skin_joint(dst,
                                           instCtlr->reserved,
                                           sid,
                                           jointsAcc->componentType)
                  : NULL;
    if (joints[i])
      joints[i]->nodeType = AK_NODE_TYPE_JOINT;

    memcpy(invm[i], matrixBase + i * matrixStride, sizeof(AkFloat) * 16u);
    glm_mat4_transpose(invm[i]);
  }

  skin->nJoints      = count;
  skin->invBindPoses = invm;
  if (!skin->joints)
    skin->joints = joints;

  /* DAE persists skeleton root as <skeleton> URL on each
     <instance_controller>; the same skin can be re-used with different
     skeletons per instance, but for a single-instance setup the first URL
     is a faithful AkSkin.skeleton hint. Fall back silently when missing —
     callers default to joints[0]. */
  if (!skin->skeleton && instCtlr->reserved) {
    const char *skelUrl = instCtlr->reserved->data;
    void       *resolved;
    if ((resolved = dae_skeleton_scope(dst->doc, skelUrl))
        && ak_typeid(resolved) == AKT_NODE) {
      skin->skeleton = resolved;
    }
  }

  instSkin                 = ak_heap_calloc(dst->heap, node, sizeof(*instSkin));
  instSkin->skin           = skin;
  instSkin->overrideJoints = joints;

  return instSkin;
}

AK_HIDE
void
dae_fixup_ctlr(DAEState * __restrict dst) {
  AkDoc        *doc;
  AkController *ctlr;

  doc  = dst->doc;
  ctlr = dst->controllers;
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
            if (!intrWeights || !intrWeights->counts)
              goto nxt_ctlr;

            primIndex     = 0;
            meshInfo      = rb_find(dst->meshInfo, mesh);

            jointswInp  = skindae->weights.joints;
            weightsInp  = skindae->weights.weights;
            if (!jointswInp || !weightsInp || !(weightsAcc = weightsInp->accessor))
              goto nxt_ctlr;

            skin->weights = ak_heap_calloc(dst->heap,
                                           ctlr->data,
                                           sizeof(void *)
                                           * mesh->primitiveCount);

            nMeshVertex = meshInfo ? meshInfo->nVertex : intrWeights->nVertex;

            flist_sp_insert(&mesh->skins, skin);

            while (prim) {
              AkAccessor    *posAcc;
              AkBoneWeights *weights; /* per-primitive weights */
              AkDuplicator  *dupl;
              size_t         count;

              if (!prim->pos || !(posAcc = prim->pos->accessor)) {
                primIndex++;
                prim = prim->next;
                continue;
              }

              dupl  = ak__docDuplicatorFind(doc, prim);
              count = 0;
              if (dupl && dupl->range)
                count = dupl->bufCount + dupl->dupCount;
              if (count == 0)
                count = posAcc->count;

              weights = ak_heap_calloc(dst->heap, ctlr->data, sizeof(*weights));

              weights->counts  = ak_heap_calloc(dst->heap,
                                                ctlr->data,
                                                count * sizeof(uint32_t));
              weights->indexes = ak_heap_calloc(dst->heap,
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
                                jointswInp->indexOffset,
                                weightsInp->indexOffset);

              skin->weights[primIndex] = weights;
              primIndex++;
              prim = prim->next;
            }

            skin->nPrims = primIndex;

            ak_free(intrWeights);
            AK_LIB_PREPEND(doc->lib.skins, skin, next);

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
              /* SIDREF source-array entries store the absolute scoped
                 path (e.g. "geom_id/sid_path"). Resolve directly via
                 ak_sid_resolve — `ak_sid_resolve_from` is for
                 already-split (id, sid) pairs and would mis-prefix
                 the path here. */
              resolved = ak_sid_resolve(&sidCtx, id, NULL);
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
          wrap                   = ak_objAlloc(dst->heap, morph,
                                               sizeof(AkGeometry *),
                                               AK_MORPHABLE_GEOMETRY,
                                               true);
          ak_objGetTarget(wrap)  = (AkGeometry *)resolved;

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
           reads meshTargets to attach AkInstanceMorph when it sees an
           <instance_controller> referring to a morph controller). */
        AK_LIB_PREPEND(doc->lib.morphs, morph, next);
        rb_insert(dst->meshTargets, baseGeom, morph);

        break;
      }
      default:
        break;
    }

  nxt_ctlr:
    ctlr = ctlr->next;
  }
}

AK_HIDE
void
dae_fixup_instctlr(DAEState * __restrict dst) {
  FListItem            *item;
  AkInstanceController *instCtlr;
  AkController         *ctlr;
  AkNode               *node;
  AkInstanceGeometry   *instGeom;

  item = dst->instCtlrs;
  while (item) {
    AkMorphDAE *morphdae;

    instCtlr = item->data;
    ctlr     = ak_instanceObject(&instCtlr->base);
    node     = instCtlr->base.node;
    instGeom = ak_heap_calloc(dst->heap, node, sizeof(*instGeom));
    instGeom->base.type = AK_INSTANCE_GEOMETRY;

    switch (ctlr->type) {
      case AK_CONTROLLER_SKIN: {
        AkInstanceSkin *instSkin;
        AkSkin         *skin;
        AkSkinDAE      *skindae;

        skin      = ctlr->data;
        skindae   = skin ? ak_userData(skin) : NULL;
        if (!skindae)
          break;

        instSkin  = dae_instance_skin_from_controller(dst,
                                                      instCtlr,
                                                      ctlr,
                                                      node);

        if (instSkin) {
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
                AkMorph         *morph2;

                morph2    = intermediate->data;
                morphdae2 = morph2 ? ak_userData(morph2) : NULL;
                if (morph2 && morph2->targetCount > 0) {
                  instMorph = ak_heap_calloc(dst->heap, node,
                                             sizeof(*instMorph));
                  instMorph->morph           = morph2;
                  instMorph->overrideWeights = NULL;
                  instGeom->morpher          = instMorph;
                }
                (void)morphdae2;
              }
              /* skin→skin nesting is exotic and the data model can't
                 represent two skinners — silently use the outer one. */
            }
            instGeom->base.object = ak_baseGeometry(&skindae->baseGeom);
          }

          /* create instance geometry for skin */
          instGeom->skinner        = instSkin;
          ak__instanceGeometryApplyBindMaterial(instGeom, instCtlr->bindMaterial);

          ak_nodeAttachInstance(node, &instGeom->base);
        }
        break;
      }
      case AK_CONTROLLER_MORPH: {
        AkInstanceMorph *instMorph;
        AkMorph         *morph;

        morph     = ctlr->data;
        morphdae  = ak_userData(morph);
        if (!morphdae)
          break;

        if (morph && morph->targetCount > 0) {
          instMorph = ak_heap_calloc(dst->heap, node, sizeof(*instMorph));
          instMorph->morph           = morph;
          instMorph->overrideWeights = NULL; /* DAE has no per-instance weights;
                                                animation drives morph.targets   */

          instGeom->morpher          = instMorph;
        }

        /* Symmetric to SKIN case: morph→skin→geom is rare but spec-allowed.
           Single-level lookahead; deeper chains collapse via ak_baseGeometry. */
        {
          void *src = ak_getObjectByUrl(&morphdae->baseGeom);
          if (src && ak_typeid(src) == AKT_CONTROLLER) {
            AkController *intermediate = src;
            if (intermediate->type == AK_CONTROLLER_SKIN) {
              AkInstanceSkin *instSkin;

              instSkin = dae_instance_skin_from_controller(dst,
                                                           instCtlr,
                                                           intermediate,
                                                           node);
              if (instSkin)
                instGeom->skinner = instSkin;
            }
          }
        }
        instGeom->base.object = ak_baseGeometry(&morphdae->baseGeom);
        ak__instanceGeometryApplyBindMaterial(instGeom, instCtlr->bindMaterial);

        ak_nodeAttachInstance(node, &instGeom->base);
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
  AkIndexArray *dupc, *dupcsum;
  AkUIntArray  *v;
  uint32_t     *pv, *pOldCountSum, *old;
  size_t       *wi, vc, d, s, pno, poo, nwsum, newidx, next, tmp;
  uint32_t     *nj, i, j, k, vcount, viStride, widx;
  bool          useDupl;

  if (!skin || !intrWeights || !weights || !weightsAcc)
    return AK_ERR;

  skindae  = ak_userData(skin);
  dupc     = NULL;
  dupcsum  = NULL;
  useDupl  = false;
  nj       = weights->counts;
  wi       = weights->indexes;
  nwsum    = 0;

  if (duplicator && duplicator->range) {
    dupc    = duplicator->range->dupc;
    dupcsum = duplicator->range->dupcsum;
    useDupl = dupc && dupcsum;
  }

  if (!nj || !wi
      || !intrWeights->counts
      || !(weightsBuff = weightsAcc->buffer)
      || !weightsBuff->data
      || !(v        = skindae->weights.v)
      || !(pv       = v->items))
    return AK_ERR;

  pOldCountSum = intrWeights->counts + intrWeights->nVertex;
  viStride     = skindae->inputCount; /* input count in <v> element */
  if (viStride == 0
      || jointOffset >= viStride
      || weightsOffset >= viStride)
    return AK_ERR;

  vc = nMeshVertex;
  if (intrWeights->nVertex < vc)
    vc = intrWeights->nVertex;
  if (useDupl && (dupc->count / 3) < vc)
    vc = dupc->count / 3;
  if (!useDupl && weights->nVertex < vc)
    vc = weights->nVertex;

#define AK_CTLR_FIX_DISPATCH(OP)                                             \
  do {                                                                       \
    switch (dupc->componentType) {                                           \
      case AKT_UBYTE:                                                        \
        switch (dupcsum->componentType) {                                    \
          case AKT_UBYTE:  OP(uint8_t, uint8_t);   break;                    \
          case AKT_USHORT: OP(uint8_t, uint16_t);  break;                    \
          case AKT_UINT:   OP(uint8_t, AkUInt);    break;                    \
          default: break;                                                    \
        }                                                                    \
        break;                                                               \
      case AKT_USHORT:                                                       \
        switch (dupcsum->componentType) {                                    \
          case AKT_UBYTE:  OP(uint16_t, uint8_t);  break;                    \
          case AKT_USHORT: OP(uint16_t, uint16_t); break;                    \
          case AKT_UINT:   OP(uint16_t, AkUInt);   break;                    \
          default: break;                                                    \
        }                                                                    \
        break;                                                               \
      case AKT_UINT:                                                         \
        switch (dupcsum->componentType) {                                    \
          case AKT_UBYTE:  OP(AkUInt, uint8_t);    break;                    \
          case AKT_USHORT: OP(AkUInt, uint16_t);   break;                    \
          case AKT_UINT:   OP(AkUInt, AkUInt);     break;                    \
          default: break;                                                    \
        }                                                                    \
        break;                                                               \
      default: break;                                                        \
    }                                                                        \
  } while (0)

#define AK_CTLR_FIX_COUNT_PASS(DUPTYPE, SUMTYPE)                             \
  do {                                                                       \
    const DUPTYPE *dupcItems_;                                               \
    const SUMTYPE *sumItems_;                                                \
                                                                             \
    dupcItems_ = (const DUPTYPE *)(const void *)dupc->items;                 \
    sumItems_  = (const SUMTYPE *)(const void *)dupcsum->items;              \
    for (i = 0; i < vc; i++) {                                               \
      if ((poo = dupcItems_[3 * i + 2]) == 0)                                \
        continue;                                                            \
                                                                             \
      pno = dupcItems_[3 * i];                                               \
      d   = dupcItems_[3 * i + 1];                                           \
      if (pno >= dupcsum->count)                                             \
        continue;                                                            \
                                                                             \
      s      = sumItems_[pno];                                               \
      vcount = ak_daeSafeWeightCount(intrWeights, v, viStride, poo - 1);     \
                                                                             \
      for (j = 0; j <= d; j++) {                                             \
        newidx = pno + j + s;                                                \
        if (newidx >= weights->nVertex)                                      \
          continue;                                                          \
        wi[newidx] = vcount;                                                 \
        nj[newidx] = vcount;                                                 \
        nwsum     += vcount;                                                 \
      }                                                                      \
    }                                                                        \
  } while (0)

#define AK_CTLR_FIX_COPY_PASS(DUPTYPE, SUMTYPE)                              \
  do {                                                                       \
    const DUPTYPE *dupcItems_;                                               \
    const SUMTYPE *sumItems_;                                                \
                                                                             \
    dupcItems_ = (const DUPTYPE *)(const void *)dupc->items;                 \
    sumItems_  = (const SUMTYPE *)(const void *)dupcsum->items;              \
    for (i = 0; i < vc; i++) {                                               \
      if ((poo = dupcItems_[3 * i + 2]) == 0)                                \
        continue;                                                            \
      pno = dupcItems_[3 * i];                                               \
      d   = dupcItems_[3 * i + 1];                                           \
      if (pno >= dupcsum->count)                                             \
        continue;                                                            \
                                                                             \
      s      = sumItems_[pno];                                               \
      vcount = ak_daeSafeWeightCount(intrWeights, v, viStride, poo - 1);     \
      old    = &pv[pOldCountSum[poo - 1] * viStride];                        \
                                                                             \
      for (j = 0; j <= d; j++) {                                             \
        tmp = pno + j + s;                                                   \
        if (tmp >= weights->nVertex)                                         \
          continue;                                                          \
                                                                             \
        newidx = wi[tmp];                                                    \
                                                                             \
        for (k = 0; k < vcount; k++) {                                       \
          widx       = old[k * viStride + weightsOffset];                    \
          iw         = &w[newidx + k];                                       \
          iw->joint  = old[k * viStride + jointOffset];                      \
          iw->weight = ak_daeReadSkinWeight(weightsAcc, widx);               \
        }                                                                    \
                                                                             \
        nwsum += vcount;                                                     \
      }                                                                      \
    }                                                                        \
  } while (0)

  /* copy to new location and duplicate if needed */
  if (useDupl) {
    AK_CTLR_FIX_DISPATCH(AK_CTLR_FIX_COUNT_PASS);
  } else {
    for (i = 0; i < vc; i++) {
      vcount = ak_daeSafeWeightCount(intrWeights, v, viStride, i);
      wi[i]  = vcount;
      nj[i]  = vcount;
      nwsum += vcount;
    }
  }

  /* prepare weight index */
  for (next = j = 0; j < weights->nVertex; j++) {
    tmp   = wi[j];
    wi[j] = next;
    next  = tmp + next;
  }

  /* now we know the size of arrays: weights, pJointsCount, npWeightsIndex */
  w     = nwsum > 0 ? ak_heap_alloc(heap, weights, sizeof(*w) * nwsum) : NULL;
  nwsum = 0;

  if (useDupl) {
    AK_CTLR_FIX_DISPATCH(AK_CTLR_FIX_COPY_PASS);
  } else {
    for (i = 0; i < vc; i++) {
      vcount = ak_daeSafeWeightCount(intrWeights, v, viStride, i);
      old    = &pv[pOldCountSum[i] * viStride];
      newidx = wi[i];

      for (k = 0; k < vcount; k++) {
        widx       = old[k * viStride + weightsOffset];
        iw         = &w[newidx + k];
        iw->joint  = old[k * viStride + jointOffset];
        iw->weight = ak_daeReadSkinWeight(weightsAcc, widx);
      }

      nwsum += vcount;
    }
  }

#undef AK_CTLR_FIX_COPY_PASS
#undef AK_CTLR_FIX_COUNT_PASS
#undef AK_CTLR_FIX_DISPATCH

  weights->weights  = w;
  weights->nWeights = nwsum;

  return AK_OK;
}
