/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_collada_brep_topology_h
#define ak_collada_brep_topology_h

#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_edges(AkHeap * __restrict heap,
             void * __restrict memParent,
             xmlTextReaderPtr reader,
             AkEdges ** __restrict dest);

AkResult _assetkit_hide
ak_dae_wires(AkHeap * __restrict heap,
             void * __restrict memParent,
             xmlTextReaderPtr reader,
             AkWires ** __restrict dest);

AkResult _assetkit_hide
ak_dae_faces(AkHeap * __restrict heap,
             void * __restrict memParent,
             xmlTextReaderPtr reader,
             AkFaces ** __restrict dest);

AkResult _assetkit_hide
ak_dae_pcurves(AkHeap * __restrict heap,
               void * __restrict memParent,
               xmlTextReaderPtr reader,
               AkPCurves ** __restrict dest);

AkResult _assetkit_hide
ak_dae_shells(AkHeap * __restrict heap,
              void * __restrict memParent,
              xmlTextReaderPtr reader,
              AkShells ** __restrict dest);

AkResult _assetkit_hide
ak_dae_solids(AkHeap * __restrict heap,
              void * __restrict memParent,
              xmlTextReaderPtr reader,
              AkSolids ** __restrict dest);

#endif /* ak_collada_brep_topology_h */
