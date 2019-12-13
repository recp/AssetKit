/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_technique.h"
#include "../dae_common.h"

AkTechnique*
dae_technique(xml_t  * __restrict xml,
              AkHeap * __restrict heap,
              void   * __restrict memparent) {
  AkTechnique *technique;

  technique          = ak_heap_calloc(heap, memparent, sizeof(*technique));
  technique->profile = xmla_strdup_by(xml, heap, _s_dae_profile, technique);
  technique->xmlns   = xmla_strdup_by(xml, heap, _s_dae_xmlns, technique);
  technique->chld    = tree_fromxml(heap, technique, xml);

  return technique;
}
