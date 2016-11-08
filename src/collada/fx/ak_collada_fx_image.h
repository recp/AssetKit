/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */


#ifndef __libassetkit__ak_collada_fx_image__h_
#define __libassetkit__ak_collada_fx_image__h_

#include "../../../include/assetkit.h"
#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_fxImage(AkDaeState * __restrict daestate,
               void * __restrict memParent,
               AkImage ** __restrict dest);

AkResult _assetkit_hide
ak_dae_fxInstanceImage(AkDaeState * __restrict daestate,
                       void * __restrict memParent,
                       AkInstanceImage ** __restrict dest);

#endif /* __libassetkit__ak_collada_fx_image__h_ */
