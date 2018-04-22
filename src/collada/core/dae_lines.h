/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_lines_h_
#define __libassetkit__dae_lines_h_

#include "../dae_common.h"

AkResult _assetkit_hide
ak_dae_lines(AkXmlState * __restrict xst,
             void     * __restrict   memParent,
             AkLineMode              mode,
             AkLines ** __restrict   dest);

#endif /* __libassetkit__dae_lines_h_ */
