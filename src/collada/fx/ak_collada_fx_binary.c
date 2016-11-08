/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_binary.h"

AkResult _assetkit_hide
ak_dae_fxBinary(AkDaeState * __restrict daestate,
                void * __restrict memParent,
                AkBinary ** __restrict dest) {
  AkBinary *binary;

  binary = ak_heap_calloc(daestate->heap,
                          memParent,
                          sizeof(*binary),
                          false);

  do {
    _xml_beginElement(_s_dae_binary);

    if (_xml_eqElm(_s_dae_ref)) {
      _xml_readText(binary, binary->ref);
    } else if (_xml_eqElm(_s_dae_hex)) {
      AkHexData *hex;
      hex = ak_heap_calloc(daestate->heap,
                           binary,
                           sizeof(*hex),
                           false);

      _xml_readAttr(hex, hex->format, _s_dae_format);

      if (hex->format) {
        _xml_readText(hex, hex->val);
        binary->hex = hex;
      } else {
        ak_free(hex);
      }
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (daestate->nodeRet);

  *dest = binary;
  
  return AK_OK;
}
