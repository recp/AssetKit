/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_mesh_h_
#define __libassetkit__dae_mesh_h_

#include "../dae_common.h"

AkResult _assetkit_hide
ak_dae_mesh(AkXmlState * __restrict xst,
            void * __restrict memParent,
            const char * elm,
            AkMesh ** __restrict dest,
            bool asObject);

#endif /* __libassetkit__dae_mesh_h_ */
