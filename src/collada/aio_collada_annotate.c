/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_annotate.h"
#include "aio_collada_common.h"
#include "aio_collada_value.h"

int _assetio_hide
aio_dae_annotate(xmlTextReaderPtr __restrict reader,
                 aio_annotate ** __restrict dest) {
  aio_annotate  *annotate;
  const xmlChar *nodeName;
  int nodeType;
  int nodeRet;

  annotate = aio_calloc(sizeof(*annotate), 1);

  _xml_readAttr(annotate->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_annotate);

    /* load once */
    if (!annotate->val)
      aio_dae_value(reader,
                    &annotate->val,
                    &annotate->val_type);

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = annotate;

  return 0;
}
