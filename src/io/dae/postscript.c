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

#include "postscript.h"
#include "../../xml.h"

#include "1.4/dae14.h"

#include "fixup/geom.h"
#include "fixup/angle.h"
#include "fixup/tex.h"

void AK_HIDE
dae_retain_refs(DAEState * __restrict dst);

void AK_HIDE
dae_fixup_accessors(DAEState * __restrict dst);

void AK_HIDE
dae_fixup_ctlr(DAEState * __restrict dst);

void AK_HIDE
dae_fixup_instctlr(DAEState * __restrict dst);

void AK_HIDE
dae_pre_mesh(DAEState * __restrict dst);

void AK_HIDE
dae_pre_walk(RBTree *tree, RBNode *rbnode);

void AK_HIDE
dae_input_walk(RBTree *tree, RBNode *rbnode);

void AK_HIDE
dae_postscript(DAEState * __restrict dst) {
  /* first migrate 1.4 to 1.5 */
  if (dst->version < AK_COLLADA_VERSION_150)
    dae14_loadjobs_finish(dst);

  dae_retain_refs(dst);
  rb_walk(dst->inputmap, dae_input_walk);
  dae_fixAngles(dst);
  dae_fixup_accessors(dst);
  dae_pre_mesh(dst);

  /* fixup when finished,
     because we need to collect about source/array usages
     also we can run fixups as parallels here
  */
  if (!ak_opt_get(AK_OPT_INDICES_DEFAULT))
    dae_geom_fixup_all(dst->doc);

  /* fixup morph and skin because order of vertices may be changed */
  if (dst->doc->lib.controllers) {
    dae_fixup_ctlr(dst);
    dae_fixup_instctlr(dst);
  }

  /* now set used coordSys */
  if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) != AK_COORD_CVT_DISABLED)
    dst->doc->coordSys = (void *)ak_opt_get(AK_OPT_COORD);

  dae_fix_textures(dst);
  
  if (dst->doc && dst->doc->lib.visualScenes) {
    for (AkVisualScene *vscn = (void *)dst->doc->lib.visualScenes->chld;
         vscn;
         vscn = (void *)vscn->base.next) {
      if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) != AK_COORD_CVT_DISABLED)
        ak_fixSceneCoordSys(vscn);
    }
  }
}

void AK_HIDE
dae_retain_refs(DAEState * __restrict dst) {
  AkHeapAllocator *alc;
  AkURLQueue      *it, *tofree;
  AkURL           *url;
  AkHeapNode      *hnode;
  int             *refc;
  AkResult         ret;

  alc = dst->heap->allocator;
  it  = dst->urlQueue;

  while (it) {
    url    = it->url;
    tofree = it;

    /* currently only retain objects in this doc */
    if (it->url->doc == dst->doc) {
      hnode = NULL;
      ret   = ak_heap_getNodeByURL(dst->heap, url, &hnode);
      if (ret == AK_OK && hnode) {
        /* retain <source> and source arrays ... */
        refc         = ak_heap_ext_add(dst->heap, hnode, AK_HEAP_NODE_FLAGS_REFC);
        it->url->ptr = ak__alignas(hnode);

        (*refc)++;
      }
    }

    it = it->next;
    alc->free(tofree);
  }
}

void AK_HIDE
dae_input_walk(RBTree *tree, RBNode *rbnode) {
  AkAccessor *acc;
  AkSource   *src;
  AkInput    *inp;
  AkURL      *url;

  AK__UNUSED(tree);

  inp = rbnode->key;
  url = rbnode->val;
  
  if (!(src = ak_getObjectByUrl(url)))
    return;

  acc           = src->tcommon;
  inp->accessor = acc;

  ak_retain(acc);

  /* TODO: */
//  ak_free(src);
//  ak_free(url);
//
//  rb_destroy(tree);
}

void AK_HIDE
dae_fixup_accessors(DAEState * __restrict dst) {
  AkHeap        *heap;
  AkDoc         *doc;
  FListItem     *item;
  AkAccessor    *acc;
  AkAccessorDAE *accdae;
  AkBuffer      *buff;
  AkTypeDesc    *type;

  item = dst->accessors;
  heap = dst->heap;
  doc  = dst->doc;
  
  while (item) {
    acc         = item->data;
    accdae      = ak_userData(acc);
    buff        = ak_getObjectByUrl(&accdae->source);
    acc->buffer = buff;

    if ((buff = ak_getObjectByUrl(&accdae->source))) {
      AkBuffer    *newbuff;
      AkDataParam *dp;
      char        *olditms, *newitms;
      uint32_t     i, j, count, dpoff, componentBytes;
      size_t       oldByteStride, newByteStride;

      acc->componentType = (AkTypeId)(uintptr_t)ak_userData(buff);

      if ((type = ak_typeDesc(acc->componentType)))
        componentBytes = type->size;
      else
        goto cont;

      count               = acc->count;
      acc->byteStride     = accdae->stride * componentBytes;
      acc->byteLength     = count * accdae->stride * componentBytes;
      acc->byteOffset     = accdae->offset * componentBytes;
      accdae->bound       = accdae->stride;

      acc->fillByteSize   = accdae->bound * componentBytes;
      acc->componentCount = accdae->bound;
      acc->componentBytes = componentBytes;

      /*--------------------------------------------------------------------*

         eliminate / remove Data Params e.g. X, Y, Z
         to make Accessor more small and cleaner
       
       *--------------------------------------------------------------------*/
      
      /* the buffer is used more than one place, so duplicate data */
      /* TODO: check param that has empty name */
      if (acc->buffer && ak_refc(buff) > 1) {
        oldByteStride   = acc->byteStride;
        newByteStride   = accdae->bound * componentBytes;
        newbuff         = ak_heap_calloc(heap, doc, sizeof(*newbuff));
        newbuff->length = count * newByteStride;
        newbuff->data   = ak_heap_calloc(heap, newbuff, newbuff->length);

        newitms         = (char *)newbuff->data;
        olditms         = (char *)buff->data + acc->byteOffset;
        
        for (i = 0; i < count; i++) {
          j     = 0;
          dpoff = 0;
          dp    = accdae->param;

          while (dp) {
            if (dp->name) {
              memcpy(newitms + newByteStride * i + componentBytes * j++,
                     olditms + oldByteStride * i + dpoff,
                     dp->type.size);
            }

            dpoff += dp->type.size;
            dp     = dp->next;
          }
        }
        
        ak_release(acc->buffer);
        ak_retain(newbuff);

        acc->buffer = newbuff;
      }

      ak_heap_ext_rm(heap, ak__alignof(buff), AK_HEAP_NODE_FLAGS_USR);
    }

    ak_heap_ext_rm(heap, ak__alignof(accdae), AK_HEAP_NODE_FLAGS_USR);
    ak_free(accdae);

  cont:
    item = item->next;
  }

  flist_sp_destroy(&dst->accessors);
}

void AK_HIDE
dae_pre_walk(RBTree *tree, RBNode *rbnode) {
  AkDaeMeshInfo *mi;
  AkSource      *posSrc;
  AkAccessor    *posAcc;

  AK__UNUSED(tree);

  mi     = rbnode->val;
  posSrc = NULL;
  posAcc = NULL;

  if (!(posAcc = mi->pos->accessor))
    return;

  mi->nVertex = posAcc->count;
}

void AK_HIDE
dae_pre_mesh(DAEState * __restrict dst) {
  rb_walk(dst->meshInfo, dae_pre_walk);
}

AK_HIDE
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

void AK_HIDE
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
          joints     = ak_heap_alloc(dst->heap,
                                     instCtlr,
                                     sizeof(void **) * count);

          invm       = ak_heap_alloc(dst->heap,
                                     ctlr->data,
                                     sizeof(mat4) * count);

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

          skin->nJoints      = count;
          skin->invBindPoses = invm;

          instSkin->skin           = skin;
          instSkin->overrideJoints = joints;
          
          /* create instance geometry for skin */
          instGeom->skinner      = instSkin;
          instGeom->bindMaterial = instCtlr->bindMaterial;
          instGeom->base.object  = skindae->baseGeom.ptr;
          ak_heap_setpm(instCtlr->bindMaterial, instGeom);
          
          instGeom->base.next = (AkInstanceBase *)node->geometry;
          if (node->geometry)
            node->geometry->base.prev = (AkInstanceBase *)instGeom;
          node->geometry = instGeom;
        }
      }
      default: break;
    }
    item = item->next;
  }
}

void AK_HIDE
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
        if (!(geom = ak_baseGeometry(&skindae->baseGeom))) {
          goto nxt_ctlr;
        }

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
      default:
        break;
    }

  nxt_ctlr:
    ctlr = ctlr->next;
  }
}
