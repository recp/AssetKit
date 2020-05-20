/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_lines_h
#define dae_lines_h

#include "../common.h"

_assetkit_hide
AkLines*
dae_lines(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp,
          AkLineMode            mode);

#endif /* dae_lines_h */
