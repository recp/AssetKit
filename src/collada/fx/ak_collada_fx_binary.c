/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_binary.h"

AkResult _assetkit_hide
ak_dae_fxBinary(AkXmlState * __restrict xst,
                void * __restrict memParent,
                AkBinary ** __restrict dest) {
  AkBinary *binary;

  binary = ak_heap_calloc(xst->heap,
                          memParent,
                          sizeof(*binary),
                          false);

  do {
    if (ak_xml_beginelm(xst, _s_dae_binary))
      break;

    if (_xml_eqElm(_s_dae_ref)) {
      binary->ref = ak_xml_val(xst, binary);
    } else if (_xml_eqElm(_s_dae_hex)) {
      AkHexData *hex;
      hex = ak_heap_calloc(xst->heap,
                           binary,
                           sizeof(*hex),
                           false);

      _xml_readAttr(hex, hex->format, _s_dae_format);

      if (hex->format) {
        hex->val = ak_xml_val(xst, hex);
        binary->hex = hex;
      } else {
        ak_free(hex);
      }
    } else {
      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

  *dest = binary;
  
  return AK_OK;
}
