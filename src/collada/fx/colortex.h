/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_color_or_tex__h_
#define __libassetkit__dae_color_or_tex__h_

#include "../../../include/ak/assetkit.h"
#include "../dae_common.h"

AkResult _assetkit_hide
dae_colorOrTex(AkXmlState   * __restrict xst,
               void         * __restrict memParent,
               const char   * elm,
               AkColorDesc ** __restrict dest);

#endif /* __libassetkit__dae_color_or_tex__h_ */
