/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_lib.h"
#include "core/dae_asset.h"

void _assetkit_hide
dae_lib(AkXmlState    * __restrict xst,
        AkLibChldDesc * __restrict lc) {
  AkLibItem    *lib;
  char         *lastLibChld;
  AkXmlElmState xest;

  lib = ak_heap_calloc(xst->heap,
                       xst->doc,
                       sizeof(*lib));
  if (lc->lastItem)
    lc->lastItem->next = lib;
  else
    *(void **)(((char *)&xst->doc->lib) + lc->libOffset) = lib;

  lc->lastItem = lib;

  ak_xml_readid(xst, lib);
  lib->name = ak_xml_attr(xst, lib, _s_dae_name);

  lastLibChld = NULL;

  ak_xest_init(xest, lc->libname)

  do {
    if (ak_xml_begin(&xest))
      break;

  after_skip:
    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      (void)dae_assetInf(xst, xst->doc, NULL);
    } else if (ak_xml_eqelm(xst, lc->name)) {
      void      *libChld;
      AkResult   ret;

      libChld = NULL;
      ret = lc->chldFn(xst, xst->doc, &libChld);
      if (ret == AK_OK && libChld) {
        if (lastLibChld)
          *(void **)(lastLibChld + lc->nextOfset) = libChld;
        else
          lib->chld = libChld;

        if (lc->prevOfset > -1)
          *(void **)((char *)libChld + lc->prevOfset) = lastLibChld;

        lastLibChld = libChld;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, xst->doc, &lib->extra);
    } else {
      ak_xml_skipelm(xst);
      goto after_skip;
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
}
