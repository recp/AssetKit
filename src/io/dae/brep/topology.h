/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
