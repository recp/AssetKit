/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_vertices_h_
#define __libassetkit__ak_collada_vertices_h_

#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_vertices(AkHeap * __restrict heap,
                void * __restrict memParent,
                xmlTextReaderPtr reader,
                AkVertices ** __restrict dest);

#endif /* __libassetkit__ak_collada_vertices_h_ */
