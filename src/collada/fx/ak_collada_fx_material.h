/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */


#ifndef __libassetkit__ak_collada_fx_material__h_
#define __libassetkit__ak_collada_fx_material__h_

#include "../../../include/assetkit.h"
#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_material(AkXmlState * __restrict xst,
                void * __restrict memParent,
                void ** __restrict dest);

AkResult _assetkit_hide
ak_dae_fxBindMaterial(AkXmlState * __restrict xst,
                      void * __restrict memParent,
                      AkBindMaterial ** __restrict dest);

AkResult _assetkit_hide
ak_dae_fxInstanceMaterial(AkXmlState * __restrict xst,
                          void * __restrict memParent,
                          AkInstanceMaterial ** __restrict dest);

AkResult _assetkit_hide
ak_dae_fxBindMaterial_tcommon(AkXmlState          * __restrict xst,
                              void                * __restrict memParent,
                              AkInstanceMaterial ** __restrict dest);

#endif /* __libassetkit__ak_collada_fx_material__h_ */
