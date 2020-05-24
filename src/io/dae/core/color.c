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

#include "color.h"

void _assetkit_hide
dae_color(xml_t   * __restrict xml,
          void    * __restrict memparent,
          bool                 read_sid,
          bool                 stack,
          AkColor * __restrict dest) {
  AkHeap       *heap;
  void         *memp;
  unsigned long c;

  heap = ak_heap_getheap(memparent);
  memp = stack ? memparent : dest;

  if (read_sid)
    sid_set(xml, heap, memp);
  
  c = xml_strtof_fast(xml->val, dest->vec, 4);
  
  if (c > 0) {
    do {
      dest->vec[4 - c--] = 1.0f;
    } while (c > 0);
  }
  
  glm_vec4_clamp(dest->vec, 0.0f, 1.0f);
}
