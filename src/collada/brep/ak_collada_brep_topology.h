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
ak_dae_edges(AkXmlState * __restrict xst,
             void * __restrict memParent,
             AkEdges ** __restrict dest);

AkResult _assetkit_hide
ak_dae_wires(AkXmlState * __restrict xst,
             void * __restrict memParent,
             AkWires ** __restrict dest);

AkResult _assetkit_hide
ak_dae_faces(AkXmlState * __restrict xst,
             void * __restrict memParent,
             AkFaces ** __restrict dest);

AkResult _assetkit_hide
ak_dae_pcurves(AkXmlState * __restrict xst,
               void * __restrict memParent,
               AkPCurves ** __restrict dest);

AkResult _assetkit_hide
ak_dae_shells(AkXmlState * __restrict xst,
              void * __restrict memParent,
              AkShells ** __restrict dest);

AkResult _assetkit_hide
ak_dae_solids(AkXmlState * __restrict xst,
              void * __restrict memParent,
              AkSolids ** __restrict dest);

#endif /* ak_collada_brep_topology_h */
