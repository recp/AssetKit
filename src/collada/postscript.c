/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "postscript.h"
#include "../xml.h"

#include "1.4/dae14.h"

#include "fixup/geom.h"
#include "fixup/angle.h"
#include "fixup/tex.h"

void _assetkit_hide
dae_retain_refs(DAEState * __restrict dst);

void _assetkit_hide
dae_fixup_accessors(DAEState * __restrict dst);

void _assetkit_hide
dae_fixup_ctlr(DAEState * __restrict dst);

void _assetkit_hide
dae_fixup_instctlr(DAEState * __restrict dst);

void _assetkit_hide
dae_pre_mesh(DAEState * __restrict dst);

void _assetkit_hide
dae_pre_walk(RBTree *tree, RBNode *rbnode);

void _assetkit_hide
dae_input_walk(RBTree * __restrict tree, RBNode * __restrict rbnode);

void _assetkit_hide
dae_postscript(DAEState * __restrict dst) {
  /* first migrate 1.4 to 1.5 */
  if (dst->version < AK_COLLADA_VERSION_150)
    dae14_loadjobs_finish(dst);

  dae_retain_refs(dst);
  rb_walk(dst->inputmap, dae_input_walk);
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
  dae_fixAngles(dst);
  
  if (dst->doc && dst->doc->lib.visualScenes) {
    for (AkVisualScene *vscn = (void *)dst->doc->lib.visualScenes->chld;
         vscn;
         vscn = (void *)vscn->base.next) {
      if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) != AK_COORD_CVT_DISABLED)
        ak_fixSceneCoordSys(vscn);
    }
  }
}

void _assetkit_hide
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
      ret = ak_heap_getNodeByURL(dst->heap, url, &hnode);
      if (ret == AK_OK) {
        /* retain <source> and source arrays ... */
        refc = ak_heap_ext_add(dst->heap, hnode, AK_HEAP_NODE_FLAGS_REFC);
//        it->url->ptr = ak__alignof(hnode);

        (*refc)++;
      }
    }

    it = it->next;
    alc->free(tofree);
  }
}

void _assetkit_hide
dae_input_walk(RBTree * __restrict tree, RBNode * __restrict rbnode) {
  AkAccessor *acc;
  AkBuffer   *buff;
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
  buff          = ak_getObjectByUrl(&acc->source);
  acc->buffer   = buff;

  /* TODO: */
//  ak_free(src);
//  ak_free(url);
//
//  rb_destroy(tree);
}

void _assetkit_hide
dae_fixup_accessors(DAEState * __restrict dst) {
  FListItem   *item;
  AkAccessor  *acc;
  AkBuffer    *buff;

  item = dst->accessors;
  while (item) {
    acc = item->data;
    if ((buff = ak_getObjectByUrl(&acc->source))) {
      size_t itemSize;

      acc->componentType = (AkTypeId)buff->reserved;
      acc->type          = ak_typeDesc(acc->componentType);

      if (acc->type)
        itemSize = acc->type->size;
      else
        goto cont;

      acc->byteStride = acc->stride * itemSize;
      acc->byteLength = acc->count  * acc->stride * itemSize;
      acc->byteOffset = acc->offset * itemSize;
      acc->bound = acc->stride; /* TODO: will be removed soon */
      
      acc->componentBytes = acc->type->size;
    }

  cont:
    item = item->next;
  }

  flist_sp_destroy(&dst->accessors);
}

void _assetkit_hide
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

void _assetkit_hide
dae_pre_mesh(DAEState * __restrict dst) {
  rb_walk(dst->meshInfo, dae_pre_walk);
}

_assetkit_hide
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
  nwsum   = count = 0;

  if (!(weightsBuff = ak_getObjectByUrl(&weightsAcc->source))
      || !(pWeights = weightsBuff->data)
      || !(v        = skin->reserved[4])
      || !(pv       = v->items))
    return AK_ERR;

#ifdef DEBUG
  assert(nMeshVertex == intrWeights->nVertex);
#endif

  pOldCount    = intrWeights->counts;
  pOldCountSum = intrWeights->counts + intrWeights->nVertex;
  viStride     = skin->reserved2; /* input count in <v> element */

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

void _assetkit_hide
dae_fixup_instctlr(DAEState * __restrict dst) {
  FListItem            *item;
  AkInstanceController *instCtlr;
  AkController         *ctlr;
  AkContext             ctx = { .doc = dst->doc };

  item = dst->instCtlrs;
  while (item) {
    instCtlr = item->data;

    ctlr = ak_instanceObject(&instCtlr->base);

    switch (ctlr->data->type) {
      case AK_CONTROLLER_SKIN: {
        AkSkin     *skin;
        AkNode    **joints;
        AkInput    *jointsInp,  *matrixInp;
        AkAccessor *jointsAcc,  *matrixAcc;
        AkBuffer   *jointsBuff, *matrixBuff;
        FListItem  *skel;
        const char *sid, **it;
        AkFloat    *mit;
        AkFloat4x4 *invm;
        size_t      count, i;

        skin      = ak_objGet(ctlr->data);
        jointsInp = skin->reserved[0];
        matrixInp = skin->reserved[1];
        invm      = NULL;
        joints    = NULL;

        if ((jointsAcc = jointsInp->accessor)) {
          matrixAcc  = matrixInp->accessor;

          jointsBuff = ak_getObjectByUrl(&jointsAcc->source);
          matrixBuff = ak_getObjectByUrl(&matrixAcc->source);

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
            sid = it[i];

            if ((skel = instCtlr->reserved)) {
               do {
                if ((joints[i] = ak_sid_resolve_from(&ctx,
                                                     skel->data,
                                                     sid,
                                                     NULL))) {
                  break;
                }
               } while ((skel = skel->next));
            }

            /* move invBindMatrix to new location */
            memcpy(invm[i], mit + 16 * i, sizeof(AkFloat) * 16);
            glm_mat4_transpose(invm[i]);
          }

          instCtlr->joints   = joints;
          skin->nJoints      = count;
          skin->invBindPoses = invm;
        }
      }
    }
    item = item->next;
  }
}

void _assetkit_hide
dae_fixup_ctlr(DAEState * __restrict dst) {
  AkDoc        *doc;
  AkController *ctlr;

  doc  = dst->doc;
  ctlr = (void *)dst->doc->lib.controllers->chld;
  while (ctlr) {
    switch (ctlr->data->type) {
      case AK_CONTROLLER_SKIN: {
        AkSkin     *skin;
        AkGeometry *geom;

        skin = ak_objGet(ctlr->data);
        if (!(geom = ak_skinBaseGeometry(skin))) {
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

            jointswInp  = skin->reserved[2];
            weightsInp  = skin->reserved[3];

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
              weights->indexes  = ak_heap_alloc(dst->heap,
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

//            /* free up old data */
//            srci = skin->source;
//            while (srci) {
//              void *tofree;
//
//              tofree = srci;
//              srci = srci->next;
//
//              ak_release(tofree);
//            }
//
//            ak_free(skin->reserved[0]);
//            ak_free(skin->reserved[1]);
//            ak_free(skin->reserved[2]);
//            ak_free(skin->reserved[3]);
//            ak_free(skin->reserved[4]);
//
//            memset(skin->reserved, 0, sizeof(void *) * 5);

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
