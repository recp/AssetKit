/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_binary.h"
#include "../aio_collada_common.h"

int _assetio_hide
aio_dae_fxBinary(void * __restrict memParent,
                 xmlTextReaderPtr __restrict reader,
                 aio_binary ** __restrict dest) {
  aio_binary    *binary;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  binary = aio_calloc(memParent, sizeof(*binary), 1);

  do {
    _xml_beginElement(_s_dae_binary);

    if (_xml_eqElm(_s_dae_ref)) {
      _xml_readText(binary, binary->ref);
    } else if (_xml_eqElm(_s_dae_hex)) {
      aio_hex_data *hex;
      hex = aio_calloc(binary, sizeof(*hex), 1);

      _xml_readAttr(hex, hex->format, _s_dae_format);

      if (hex->format) {
        _xml_readText(hex, hex->val);
        binary->hex = hex;
      } else {
        aio_free(hex);
      }
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = binary;
  
  return 0;
}
