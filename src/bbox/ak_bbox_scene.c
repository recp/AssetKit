/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_bbox.h"
#include <cglm.h>

static
void
ak_bbox_node(AkVisualScene * __restrict scene,
             AkNode        * __restrict node,
             mat4                       parentTrans) {
  mat4 trans;
  ak_transformCombine(node, trans[0]);
  glm_mat4_mul(parentTrans, trans, trans);

  if (node->geometry) {
    AkInstanceBase *geomInst;
    AkGeometry     *geom;
    AkBoundingBox   bbox;
    vec4            min, max;

    glm_vec_broadcast(0.0f, bbox.min);
    glm_vec_broadcast(0.0f, bbox.max);

    /* find bbox for node to avoid extra calc */
    geomInst = &node->geometry->base;
    while (geomInst) {
      geom = ak_instanceObject(geomInst);
      if (geom)
        ak_bbox_pick_pbox(&bbox, geom->bbox);

      geomInst = geomInst->next;
    }

    glm_vec_copy(bbox.min, min);
    glm_vec_copy(bbox.max, max);

    /* this is position ? */
    min[3] = max[3] = 1;
    glm_mat4_mulv(trans, min, min);
    glm_mat4_mulv(trans, max, max);

    ak_bbox_pick_pbox2(scene->bbox, min, max);
  }

  if (node->node) {
    AkNode *nodei;
    if ((nodei = ak_instanceObjectNode(node)))
      ak_bbox_node(scene, nodei, trans);
  }

  if (node->chld) {
    AkNode *nodei;
    nodei = node->chld;
    do {
      ak_bbox_node(scene, nodei, trans);
      nodei = nodei->next;
    } while (nodei);
  }
}

void
ak_bbox_scene(struct AkVisualScene * __restrict scene) {
  mat4 idmat = GLM_MAT4_IDENTITY_INIT;
  AkHeap *heap;
  AkNode *node;
  node = scene->node;

  heap = ak_heap_getheap(scene);

  if (!scene->bbox)
    scene->bbox = ak_heap_calloc(heap,
                                 scene,
                                 sizeof(*scene->bbox));
  while (node) {
    ak_bbox_node(scene, node, idmat);
    node = node->next;
  }
}
