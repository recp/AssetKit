/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_param__h_
#define __libassetkit__ak_collada_param__h_

#include "../../include/assetkit.h"
#include "ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_newparam(void * __restrict memParent,
                 xmlTextReaderPtr reader,
                 ak_newparam ** __restrict dest);

AkResult _assetkit_hide
ak_dae_param(void * __restrict memParent,
              xmlTextReaderPtr reader,
              AkParamType param_type,
              ak_param ** __restrict dest);

AkResult _assetkit_hide
ak_dae_setparam(void * __restrict memParent,
                 xmlTextReaderPtr reader,
                 ak_setparam ** __restrict dest);

#endif /* __libassetkit__ak_collada_param__h_ */
