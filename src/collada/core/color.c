/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "color.h"

void _assetkit_hide
dae_color(xml_t   * __restrict xml,
          void    * __restrict memparent,
          bool                 read_sid,
          bool                 stack,
          AkColor * __restrict dest) {
  AkHeap *heap;
  void   *memp;
  int     c;

  heap = ak_heap_getheap(memparent);
  memp = stack ? memparent : dest;

  if (read_sid)
    sid_set(xml, heap, memp);
  
  c = xml_strtof_fast(xml, dest->vec, 4);
  if (c > 0) {
    do {
      dest->vec[4 - c--] = 1.0f;
    } while (c > 0);
  }
}
