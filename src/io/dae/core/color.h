/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_color_h
#define dae_color_h

#include "../common.h"

void _assetkit_hide
dae_color(xml_t   * __restrict xml,
          void    * __restrict memparent,
          bool                 read_sid,
          bool                 stack,
          AkColor * __restrict dest);

#endif /* dae_color_h */
