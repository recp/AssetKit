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
ak_dae_material(AkHeap * __restrict heap,
                void * __restrict memParent,
                xmlTextReaderPtr reader,
                AkMaterial ** __restrict dest);

AkResult _assetkit_hide
ak_dae_fxBindMaterial(AkHeap * __restrict heap,
                      void * __restrict memParent,
                      xmlTextReaderPtr reader,
                      AkBindMaterial ** __restrict dest);

AkResult _assetkit_hide
ak_dae_fxInstanceMaterial(AkHeap * __restrict heap,
                          void * __restrict memParent,
                          xmlTextReaderPtr reader,
                          AkInstanceMaterial ** __restrict dest);

#endif /* __libassetkit__ak_collada_fx_material__h_ */
