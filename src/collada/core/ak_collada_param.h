/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_param__h_
#define __libassetkit__ak_collada_param__h_

#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_newparam(AkXmlState * __restrict xst,
                void * __restrict memParent,
                AkNewParam ** __restrict dest);

AkResult _assetkit_hide
ak_dae_param(AkXmlState * __restrict xst,
             void * __restrict memParent,
             AkParamType paramType,
             AkParam ** __restrict dest);

AkResult _assetkit_hide
ak_dae_setparam(AkXmlState * __restrict xst,
                void * __restrict memParent,
                AkSetParam ** __restrict dest);

#endif /* __libassetkit__ak_collada_param__h_ */
