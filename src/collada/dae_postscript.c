/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_postscript.h"
#include "dae_geom_fixup.h"
#include "../memory_common.h"
#include "../xml.h"

#include "1.4/dae14.h"

void _assetkit_hide
dae_retain_refs(AkXmlState * __restrict xst);

void _assetkit_hide
dae_fixup_accessors(AkXmlState * __restrict xst);

void _assetkit_hide
dae_pre_mesh(AkXmlState * __restrict xst);

void _assetkit_hide
dae_pre_walk(RBTree *tree, RBNode *rbnode);

void _assetkit_hide
dae_postscript(AkXmlState * __restrict xst) {
  /* first migrate 1.4 to 1.5 */
  if (xst->version < AK_COLLADA_VERSION_150)
    dae14_loadjobs_finish(xst);

  dae_retain_refs(xst);
  dae_fixup_accessors(xst);
  dae_pre_mesh(xst);

  /* fixup when finished,
     because we need to collect about source/array usages
     also we can run fixups as parallels here
  */
  if (!ak_opt_get(AK_OPT_INDICES_DEFAULT))
    dae_geom_fixup_all(xst->doc);

  /* now set used coordSys */
  if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) != AK_COORD_CVT_DISABLED)
    xst->doc->coordSys = (void *)ak_opt_get(AK_OPT_COORD);
}

void _assetkit_hide
dae_retain_refs(AkXmlState * __restrict xst) {
  AkHeapAllocator *alc;
  AkURLQueue      *it, *tofree;
  AkURL           *url;
  AkHeapNode      *hnode;
  int             *refc;
  AkResult         ret;

  alc = xst->heap->allocator;
  it  = xst->urlQueue;

  while (it) {
    url    = it->url;
    tofree = it;

    /* currently only retain objects in this doc */
    if (it->url->doc == xst->doc) {
      ret = ak_heap_getNodeByURL(xst->heap, url, &hnode);
      if (ret == AK_OK) {
        /* retain <source> and source arrays ... */
        refc = ak_heap_ext_add(xst->heap,
                               hnode,
                               AK_HEAP_NODE_FLAGS_REFC);
        (*refc)++;
      }
    }

    it = it->next;
    alc->free(tofree);
  }
}

void _assetkit_hide
dae_fixup_accessors(AkXmlState * __restrict xst) {
  FListItem   *item;
  AkAccessor  *acc;
  AkBuffer    *buff;
  AkDataParam *param;

  item = xst->accessors;
  while (item) {
    acc = item->data;
    if ((buff = ak_getObjectByUrl(&acc->source))) {
      size_t itemSize;

      acc->itemTypeId = (AkTypeId)buff->reserved;
      acc->type       = ak_typeDesc(acc->itemTypeId);

      if (acc->type)
        itemSize = acc->type->size;
      else
        goto cont;

      acc->byteStride = acc->stride * itemSize;
      acc->byteLength = acc->count  * acc->stride * itemSize;
      acc->byteOffset = acc->offset * itemSize;

      /* convert ANGLE arrays to radians */
      if (acc->itemTypeId == AKT_FLOAT && (param = acc->param)) {
        float *pbuff;
        size_t paramOffset, i, count;

        paramOffset = 0;
        count       = buff->length / itemSize;
        pbuff       = buff->data;
        do {
          if (param->name && strcasecmp(param->name, _s_dae_angle) == 0) {
            /* TODO: use SIMD */
            for (i = 0; i < count; i++)
              glm_make_rad(&pbuff[i + paramOffset]);
          }
          paramOffset++;
        } while ((param = param->next));
      }
    }

  cont:
    item = item->next;
  }

  flist_sp_destroy(&xst->accessors);
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

  if (!((posSrc = ak_getObjectByUrl(&mi->pos->source))
      && (posAcc = posSrc->tcommon)))
    return;

  mi->nVertex = posAcc->count;
}

void _assetkit_hide
dae_pre_mesh(AkXmlState * __restrict xst) {
  rb_walk(xst->meshInfo, dae_pre_walk);
}
}
