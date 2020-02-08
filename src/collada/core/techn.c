/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "techn.h"
#include "../common.h"

AkTechnique*
dae_techn(xml_t  * __restrict xml,
          AkHeap * __restrict heap,
          void   * __restrict memp) {
  AkTechnique *techn;

  techn          = ak_heap_calloc(heap, memp, sizeof(*techn));
  techn->profile = xmla_strdup_by(xml, heap, _s_dae_profile, techn);
  techn->xmlns   = xmla_strdup_by(xml, heap, _s_dae_xmlns, techn);
  techn->chld    = tree_fromxml(heap, techn, xml);

  return techn;
}
