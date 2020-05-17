/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "node.h"
#include <cglm/cglm.h>

/*!
 * @brief fix camera, light
 *
 * @todo new node lost its id, sid and name, FIX THIS!
 */
void _assetkit_hide
dae_nodeFixupFixedCoord(AkHeap * __restrict heap,
                        AkNode * __restrict node) {
  AkNode *newNode;

  if (!node->camera
       && !node->light)
      return;

  if (!node->geometry
      && !node->chld
      && !node->node) {
    node->flags |= AK_NODEF_FIXED_COORD;
    return;
  }

  /* move to new node, if we move to new child node we would not neeed to,
   duplicate transform but the node may be used by sid path, so we couldn't
   move to child, instead we have to duplicate transfroms :(
   */

  newNode = ak_heap_calloc(heap, node, sizeof(*newNode));
  newNode->nodeType = AK_NODE_TYPE_NODE;

  if (node->camera) {
    AkInstanceBase *inst;
    inst = node->camera;
    while (inst) {
      ak_heap_setpm(inst, newNode);
      inst = inst->next;
    }

    newNode->camera = node->camera;
    node->camera    = NULL;
  }

  if (node->light) {
    AkInstanceBase *inst;
    inst = node->light;
    while (inst) {
      ak_heap_setpm(inst, newNode);
      inst = inst->next;
    }

    newNode->light = node->light;
    node->light    = NULL;
  }

  /* duplicate all transforms before apply rotations */
  ak_transformDup(node, newNode);
}

void _assetkit_hide
dae_nodeFixup(AkHeap * __restrict heap,
              AkNode * __restrict node) {
  if (node->camera || node->light)
    dae_nodeFixupFixedCoord(heap, node);

  ak_fixNodeCoordSys(node);
}
