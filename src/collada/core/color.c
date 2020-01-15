/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_color.h"

void _assetkit_hide
dae_color(xml_t   * __restrict xml,
          void    * __restrict memparent,
          bool                 read_sid,
          bool                 stack,
          AkColor * __restrict dest) {
  AkHeap *heap;
  char   *colorstr;
  void   *memp;

  heap = ak_heap_getheap(memparent);
  memp = stack ? memparent : dest;

  if (read_sid)
    sid_set(xml, heap, memp);

  colorstr = (char *)xml_string(xml);
  memset(colorstr + xml->valsize, '\0', 1);

  if (colorstr) {
    int c;
    c = ak_strtof4(&colorstr, &dest->vec);
    if (c > 0) {
      do {
        dest->vec[4 - c--] = 1.0f;
      } while (c > 0);
    }
  }
}
