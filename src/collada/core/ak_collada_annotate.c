/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_annotate.h"
#include "ak_collada_value.h"

AkResult _assetkit_hide
ak_dae_annotate(AkDaeState * __restrict daestate,
                void * __restrict memParent,
                AkAnnotate ** __restrict dest) {
  AkAnnotate *annotate;

  annotate = ak_heap_calloc(daestate->heap,
                            memParent,
                            sizeof(*annotate),
                            false);

  _xml_readAttr(annotate, annotate->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_annotate);

    /* load once */
    if (!annotate->val)
      ak_dae_value(daestate,
                   annotate,
                   &annotate->val,
                   &annotate->valType);

    /* end element */
    _xml_endElement;
  } while (daestate->nodeRet);

  *dest = annotate;

  return AK_OK;
}
