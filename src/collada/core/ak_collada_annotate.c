/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_annotate.h"
#include "ak_collada_value.h"

AkResult _assetkit_hide
ak_dae_annotate(AkHeap * __restrict heap,
                void * __restrict memParent,
                xmlTextReaderPtr reader,
                AkAnnotate ** __restrict dest) {
  AkAnnotate   *annotate;
  const xmlChar *nodeName;
  int nodeType;
  int nodeRet;

  annotate = ak_heap_calloc(heap, memParent, sizeof(*annotate), false);

  _xml_readAttr(annotate, annotate->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_annotate);

    /* load once */
    if (!annotate->val)
      ak_dae_value(heap,
                   annotate,
                   reader,
                   &annotate->val,
                   &annotate->valType);

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = annotate;

  return AK_OK;
}
