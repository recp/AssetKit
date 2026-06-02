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

#include "bbox.h"
#include <cglm/cglm.h>
#include <float.h>

static
void
ak_bbox_node(AkHeap  * __restrict heap,
             AkScene * __restrict scene,
             AkNode  * __restrict node,
             mat4                 parentTrans,
             bool                 cacheWorld);

static
void
ak_bbox_pick_transformed(AkBoundingBox * __restrict sceneBox,
                         AkBoundingBox * __restrict localBox,
                         mat4                       matrixWorld) {
  vec3 box[2], transformed[2];

  if (!sceneBox || !localBox || !localBox->isvalid)
    return;

  glm_vec3_copy(localBox->min, box[0]);
  glm_vec3_copy(localBox->max, box[1]);
  glm_aabb_transform(box, matrixWorld, transformed);
  ak_bbox_pick_pbox2(sceneBox, transformed[0], transformed[1]);
}

static
void
ak_bbox_node(AkHeap  * __restrict heap,
             AkScene * __restrict scene,
             AkNode  * __restrict node,
             mat4                 parentTrans,
             bool                 cacheWorld) {
  mat4      matrixWorldStack;
  AkFloat  (*matrixWorld)[4];

  if (!node)
    return;

  if (!node->matrix)
    node->matrix = ak_heap_alloc(heap,
                                 node,
                                 sizeof(*node->matrix));

  if (cacheWorld) {
    if (!node->matrixWorld)
      node->matrixWorld = ak_heap_alloc(heap,
                                        node,
                                        sizeof(*node->matrixWorld));
    matrixWorld = node->matrixWorld->val;
  } else {
    matrixWorld = matrixWorldStack;
  }

  ak_transformCombine(node->transform, node->matrix->val[0]);
  glm_mat4_mul(parentTrans, node->matrix->val, matrixWorld);

  if (node->geometry) {
    AkInstanceBase *geomInst;
    AkGeometry     *geom;
    AkBoundingBox   bbox;

    ak_bbox_invalidate(&bbox);

    /* find bbox for node to avoid extra calc */
    geomInst = &node->geometry->base;
    while (geomInst) {
      geom = ak_instanceObject(geomInst);
      if (geom && geom->gdata && (!geom->bbox || !geom->bbox->isvalid))
        ak_bbox_geom(geom);
      if (geom && geom->bbox && geom->bbox->isvalid)
        ak_bbox_pick_pbox(&bbox, geom->bbox);

      geomInst = geomInst->next;
    }

    ak_bbox_pick_transformed(scene->bbox, &bbox, matrixWorld);
  }

  if (node->nodeRefs) {
    AkNodeRef *nodeRef;

    for (nodeRef = node->nodeRefs; nodeRef; nodeRef = nodeRef->next) {
      AkNode *nodei;
      if ((nodei = ak_nodeRefTarget(nodeRef)))
        ak_bbox_node(heap, scene, nodei, matrixWorld, false);
    }
  }

  if (node->chld) {
    AkNode *nodei;
    nodei = node->chld;
    do {
      ak_bbox_node(heap, scene, nodei, matrixWorld, cacheWorld);
      nodei = nodei->next;
    } while (nodei);
  }
}

void
ak_bbox_scene(struct AkScene * __restrict scene) {
  mat4    trans = GLM_MAT4_IDENTITY_INIT;
  AkHeap *heap;

  if (!scene)
    return;

  heap = ak_heap_getheap(scene);

  if (!scene->bbox)
    scene->bbox = ak_heap_calloc(heap, scene, sizeof(*scene->bbox));

  ak_bbox_invalidate(scene->bbox);

  if (scene->node)
    ak_bbox_node(heap, scene, scene->node, trans, true);
}
