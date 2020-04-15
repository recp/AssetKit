/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_lines_h_
#define __libassetkit__dae_lines_h_

#include "../common.h"

_assetkit_hide
AkLines*
dae_lines(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp,
          AkLineMode            mode);

#endif /* __libassetkit__dae_lines_h_ */
