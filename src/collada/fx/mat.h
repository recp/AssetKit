/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */


#ifndef __libassetkit__dae_fx_material__h_
#define __libassetkit__dae_fx_material__h_

#include "../../../include/ak/assetkit.h"
#include "../common.h"

AkResult _assetkit_hide
dae_material(AkXmlState * __restrict xst,
             void * __restrict memParent,
             void ** __restrict dest);

AkResult _assetkit_hide
dae_fxBindMaterial(AkXmlState * __restrict xst,
                   void * __restrict memParent,
                   AkBindMaterial ** __restrict dest);

AkResult _assetkit_hide
dae_fxInstanceMaterial(AkXmlState * __restrict xst,
                       void * __restrict memParent,
                       AkInstanceMaterial ** __restrict dest);

AkResult _assetkit_hide
dae_fxBindMaterial_tcommon(AkXmlState          * __restrict xst,
                           void                * __restrict memParent,
                           AkInstanceMaterial ** __restrict dest);

#endif /* __libassetkit__dae_fx_material__h_ */
