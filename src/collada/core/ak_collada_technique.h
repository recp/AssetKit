/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_technique__h_
#define __libassetkit__ak_collada_technique__h_

#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_technique(AkXmlState * __restrict xst,
                 void * __restrict memParent,
                 AkTechnique ** __restrict dest);

AkResult _assetkit_hide
ak_dae_techniquec(AkXmlState * __restrict xst,
                  void * __restrict memParent,
                  AkTechniqueCommon ** __restrict dest);

#endif /* __libassetkit__ak_collada_technique__h_ */
