/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_brep_topology_h
#define dae_brep_topology_h

#include "../common.h"

AkEdges* _assetkit_hide
dae_edges(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

AkWires* _assetkit_hide
dae_wires(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

AkFaces* _assetkit_hide
dae_faces(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

AkPCurves* _assetkit_hide
dae_pcurves(DAEState * __restrict dst,
            xml_t    * __restrict xml,
            void     * __restrict memp);

AkShells* _assetkit_hide
dae_shells(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           void     * __restrict memp);

AkSolids* _assetkit_hide
dae_solids(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           void     * __restrict memp);

#endif /* dae_brep_topology_h */
