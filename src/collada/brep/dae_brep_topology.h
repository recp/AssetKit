/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_brep_topology_h
#define dae_brep_topology_h

#include "../dae_common.h"

AkResult _assetkit_hide
dae_edges(AkXmlState * __restrict xst,
          void * __restrict memParent,
          AkEdges ** __restrict dest);

AkResult _assetkit_hide
dae_wires(AkXmlState * __restrict xst,
          void * __restrict memParent,
          AkWires ** __restrict dest);

AkResult _assetkit_hide
dae_faces(AkXmlState * __restrict xst,
          void * __restrict memParent,
          AkFaces ** __restrict dest);

AkResult _assetkit_hide
dae_pcurves(AkXmlState * __restrict xst,
            void * __restrict memParent,
            AkPCurves ** __restrict dest);

AkResult _assetkit_hide
dae_shells(AkXmlState * __restrict xst,
           void * __restrict memParent,
           AkShells ** __restrict dest);

AkResult _assetkit_hide
dae_solids(AkXmlState * __restrict xst,
           void * __restrict memParent,
           AkSolids ** __restrict dest);

#endif /* dae_brep_topology_h */
