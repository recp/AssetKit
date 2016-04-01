/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_mesh_h_
#define __libassetkit__ak_collada_mesh_h_

#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_mesh(void * __restrict memParent,
            xmlTextReaderPtr reader,
            const char * elm,
            AkMesh ** __restrict dest,
            bool asObject);

#endif /* __libassetkit__ak_collada_mesh_h_ */
