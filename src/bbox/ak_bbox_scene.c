/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_bbox.h"
#include <cglm/cglm.h>
#include <float.h>

static
void
ak_bbox_node(AkHeap        * __restrict heap,
             AkVisualScene * __restrict scene,
             AkNode        * __restrict node,
             mat4                       parentTrans);

static
void
ak_bbox_node(AkHeap        * __restrict heap,
             AkVisualScene * __restrict scene,
             AkNode        * __restrict node,
             mat4                       parentTrans) {
  AkFloat  (*matrixWorld)[4];

  if (!node->matrix)
    node->matrix = ak_heap_alloc(heap,
                                 node,
                                 sizeof(*node->matrix));

  if (!node->matrixWorld)
    node->matrixWorld = ak_heap_alloc(heap,
                                      node,
                                      sizeof(*node->matrixWorld));

  matrixWorld = node->matrixWorld->val;

  ak_transformCombine(node, node->matrix->val[0]);
  glm_mat4_mul(parentTrans, node->matrix->val, matrixWorld);

  if (node->geometry) {
    AkInstanceBase *geomInst;
    AkGeometry     *geom;
    AkBoundingBox   bbox;
    vec4            min, max;

    glm_vec_broadcast(FLT_MAX, bbox.min);
    glm_vec_broadcast(-FLT_MAX, bbox.max);
    min[3] = max[3] = 1.0f;

    /* find bbox for node to avoid extra calc */
    geomInst = &node->geometry->base;
    while (geomInst) {
      geom = ak_instanceObject(geomInst);
      if (geom && geom->bbox)
        ak_bbox_pick_pbox(&bbox, geom->bbox);

      geomInst = geomInst->next;
    }

    glm_vec_copy(bbox.min, min);
    glm_vec_copy(bbox.max, max);

    glm_mat4_mulv(matrixWorld, min, min);
    glm_mat4_mulv(matrixWorld, max, max);

    if (scene->bbox->isvalid) {
      ak_bbox_pick_pbox2(scene->bbox, min, max);
    } else {
      glm_vec_copy(min, scene->bbox->min);
      glm_vec_copy(max, scene->bbox->max);
      scene->bbox->isvalid = true;
    }
  }

  if (node->node) {
    AkNode *nodei;
    if ((nodei = ak_instanceObjectNode(node))) {
      do {
        ak_bbox_node(heap, scene, nodei, matrixWorld);
        nodei = nodei->next;
      } while (nodei);
    }
  }

  if (node->chld) {
    AkNode *nodei;
    nodei = node->chld;
    do {
      ak_bbox_node(heap, scene, nodei, matrixWorld);
      nodei = nodei->next;
    } while (nodei);
  }
}

void
ak_bbox_scene(struct AkVisualScene * __restrict scene) {
  mat4    trans = GLM_MAT4_IDENTITY_INIT;
  AkHeap *heap;
  AkNode *node;

  node = scene->node;
  heap = ak_heap_getheap(scene);

  if (!scene->bbox)
    scene->bbox = ak_heap_calloc(heap, scene, sizeof(*scene->bbox));

  while (node) {
    ak_bbox_node(heap, scene, node, trans);
    node = node->next;
  }
}
