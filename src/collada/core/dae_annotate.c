/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_annotate.h"
#include "dae_value.h"

AkResult _assetkit_hide
dae_annotate(AkXmlState * __restrict xst,
             void * __restrict memParent,
             AkAnnotate ** __restrict dest) {
  AkAnnotate   *annotate;
  AkXmlElmState xest;

  annotate = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*annotate));

  annotate->name = ak_xml_attr(xst, annotate, _s_dae_name);

  ak_xest_init(xest, _s_dae_annotate)

  do {
    if (ak_xml_begin(&xest))
      break;

    /* load once */
    if (!annotate->val) {
      AkValue *val;
      AkResult ret;

      ret = dae_value(xst, annotate, &val);

      if (ret == AK_OK)
        annotate->val = val;
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = annotate;

  return AK_OK;
}
