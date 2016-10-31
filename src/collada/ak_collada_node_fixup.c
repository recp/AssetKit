/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_node_fixup.h"
#include <cglm.h>

AK_INLINE
void _assetkit_hide
ak_dae_nodeFixupCameraTrans(AkDoc  * __restrict doc,
                            AkNode * __restrict node) {
  if (node->transform
      && node->transform->type == AK_NODE_TRANSFORM_TYPE_MATRIX) {
    AkMatrix *matrix;
    matrix = ak_objGet(node->transform);
    ak_coordFixCamOri(doc->coordSys,
                      ak_defaultCoordSys(),
                      matrix->val);
  }
}

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
                       AkDoc  * __restrict doc,
                       AkNode * __restrict node) {
  /* node has only camera which we want */
  if (node->camera
      && !node->controller
      && !node->geometry
      && !node->light
      && !node->chld
      && !node->node) {
    ak_dae_nodeFixupCameraTrans(doc, node);
  } else {
    /* make sure camera has its own node */
    AkNode *camNode;
    camNode = ak_heap_calloc(heap, node, sizeof(*node), true);

    camNode->camera = node->camera;
    ak_heap_setpm(heap, node->camera, camNode);
    node->camera = NULL;

    /* duplicate all transforms before apply rotations */
    ak_transformDup(node, camNode);

    /* fix orientation */
    ak_dae_nodeFixupCameraTrans(doc, camNode);
  }
}

void _assetkit_hide
ak_dae_nodeFixup(AkHeap * __restrict heap,
                 AkDoc  * __restrict doc,
                 AkNode * __restrict node) {
  if (node->camera) {
    ak_dae_nodeFixupCamera(heap, doc, node);
  }
}
