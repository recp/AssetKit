/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef gltf_node_h
#define gltf_node_h

#include "../gltf_common.h"

void _assetkit_hide
gltf_nodes(json_t * __restrict jnode,
           void   * __restrict userdata);

AkNode* _assetkit_hide
gltf_node(AkGLTFState * __restrict gst,
          void        * __restrict memParent,
          json_t      * __restrict jnode,
          AkNode     ** __restrict nodechld);

void _assetkit_hide
gltf_bindMaterials(AkGLTFState        * __restrict gst,
                   AkInstanceGeometry * __restrict instGeom,
                   int32_t                         meshIndex);

#endif /* gltf_node_h */
