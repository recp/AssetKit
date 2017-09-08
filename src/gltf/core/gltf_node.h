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
gltf_nodes(AkGLTFState * __restrict gst);

AkNode* _assetkit_hide
gltf_node(AkGLTFState * __restrict gst,
          void        * __restrict memParent,
          json_t      * __restrict jnode);

#endif /* gltf_node_h */
