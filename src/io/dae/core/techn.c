/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
