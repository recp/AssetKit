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
    ak_heap_setpm(node->camera, camNode);
    node->camera = NULL;

    /* duplicate all transforms before apply rotations */
    ak_transformDup(node, camNode);
  }
}

void _assetkit_hide
ak_dae_nodeFixup(AkHeap        * __restrict heap,
                 AkDoc         * __restrict doc,
                 AkVisualScene * __restrict scene,
                 AkNode        * __restrict node) {
  AkCoordSys *coordSys, *newCoordSys;
  bool        hasCoordSys;

  newCoordSys = (void *)ak_opt_get(AK_OPT_COORD);

  /* step 1: fixup camera node */
  if (node->camera)
    ak_dae_nodeFixupCamera(heap, node);

  /* step 2: fixup coord sys  */
  if ((hasCoordSys = ak_hasCoordSys(node)))
    coordSys = ak_getCoordSys(node);
  else
    coordSys = doc->coordSys;

  if (newCoordSys == coordSys)
    return;

  switch (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE)) {
    case AK_COORD_CVT_FIX_TRANSFORM: {
      if (hasCoordSys
          /* don't change nodes in library_nodes */
          || (!node->parent && scene)) {
        AkCoordSys *oldCoordSys;
        oldCoordSys = ak_getCoordSys(node);

        if (!node->transform) {
          node->transform = ak_heap_calloc(heap,
                                           node,
                                           sizeof(*node->transform));
        }

        /* fixup transform[s] */
        ak_coordFindTransform(node->transform,
                              oldCoordSys,
                              newCoordSys);
      }
      break;
    }
    case AK_COORD_CVT_ALL:
      if (node->transform)
        ak_coordCvtNodeTransforms(doc, node);
      break;
    default:
      break;
  }
}
