/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_node_fixup.h"
#include <cglm.h>

/*!
 * @brief fix camera node
 * 
 * make sure camera has its own node
 *
 * @todo new node lost its id, sid and name, FIX THIS!
 */
AK_INLINE
void _assetkit_hide
ak_dae_nodeFixupCamera(AkHeap * __restrict heap,
                       AkNode * __restrict node) {
  /* node has only camera which we want */
  if (node->camera
      && !node->controller
      && !node->geometry
      && !node->light
      && !node->chld
      && !node->node) {
    node->nodeType = AK_NODE_TYPE_CAMERA_NODE;
  } else {
    /* make sure camera has its own node */
    AkNode *camNode;
    camNode = ak_heap_calloc(heap, node, sizeof(*node));

    node->nodeType = AK_NODE_TYPE_CAMERA_NODE;

    camNode->camera = node->camera;
    ak_heap_setpm(heap, node->camera, camNode);
    node->camera = NULL;

    /* duplicate all transforms before apply rotations */
    ak_transformDup(node, camNode);
  }
}

void _assetkit_hide
ak_dae_nodeFixup(AkHeap * __restrict heap,
                 AkDoc  * __restrict doc,
                 AkNode * __restrict node) {
  if (node->camera)
    ak_dae_nodeFixupCamera(heap, node);

  if ((void *)ak_opt_get(AK_OPT_COORD) != doc->coordSys)
    ak_coordCvtNodeTransforms(doc, node);
}
