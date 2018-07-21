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
ak_dae_retain_refs(AkXmlState * __restrict xst);

void _assetkit_hide
ak_dae_fixup_accessors(AkXmlState * __restrict xst);

void _assetkit_hide
ak_dae_postscript(AkXmlState * __restrict xst) {
  /* first migrate 1.4 to 1.5 */
  if (xst->version < AK_COLLADA_VERSION_150)
    ak_dae14_loadjobs_finish(xst);

  ak_dae_retain_refs(xst);

  ak_dae_fixup_accessors(xst);

  /* fixup when finished,
     because we need to collect about source/array usages
     also we can run fixups as parallels here
  */
  if (!ak_opt_get(AK_OPT_INDICES_DEFAULT))
    ak_dae_geom_fixup_all(xst->doc);

  /* now set used coordSys */
  if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) != AK_COORD_CVT_DISABLED)
    xst->doc->coordSys = (void *)ak_opt_get(AK_OPT_COORD);
}

void _assetkit_hide
ak_dae_retain_refs(AkXmlState * __restrict xst) {
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
ak_dae_fixup_accessors(AkXmlState * __restrict xst) {
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
          if (strcasecmp(param->name, _s_dae_angle) == 0) {
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
}
