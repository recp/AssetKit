/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_annotate.h"
#include "ak_collada_value.h"

AkResult _assetkit_hide
ak_dae_annotate(AkXmlState * __restrict xst,
                void * __restrict memParent,
                AkAnnotate ** __restrict dest) {
  AkAnnotate *annotate;

  annotate = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*annotate));

  annotate->name = ak_xml_attr(xst, annotate, _s_dae_name);

  do {
    if (ak_xml_beginelm(xst, _s_dae_annotate))
      break;

    /* load once */
    if (!annotate->val)
      ak_dae_value(xst,
                   annotate,
                   &annotate->val,
                   &annotate->valType);

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);

  *dest = annotate;

  return AK_OK;
}
