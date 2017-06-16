/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_lib.h"
#include "core/ak_collada_asset.h"

void _assetkit_hide
ak_dae_lib(AkXmlState    * __restrict xst,
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

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(xst, xst->doc, &assetInf);
      if (ret == AK_OK)
        lib->inf = assetInf;

    } else if (ak_xml_eqelm(xst, lc->name)) {
      void      *libChld;
      AkResult   ret;

      libChld = NULL;
      ret = lc->chldFn(xst, xst->doc, &libChld);
      if (ret == AK_OK) {
        if (lastLibChld)
          *(void **)(lastLibChld + lc->nextOfset) = libChld;
        else
          lib->chld = libChld;

        if (lc->prevOfset > -1)
          *(void **)((char *)libChld + lc->prevOfset) = lastLibChld;

        lastLibChld = libChld;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          xst->doc,
                          nodePtr,
                          &tree,
                          NULL);
      lib->extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
}
