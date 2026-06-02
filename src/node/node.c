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

#include "../common.h"
#include "../../include/ak/assetkit.h"
#include "../../include/ak/node.h"
#include "../../include/ak/coord-util.h"
#include "../../include/ak/instance.h"
#include <assert.h>
#include <string.h>

AK_EXPORT
void
ak_addSubNode(AkNode * __restrict parent,
              AkNode * __restrict subnode,
              bool                fixCoordSys) {
  assert(parent != subnode);

  if (subnode->parent) {
    if (subnode->parent->chld == subnode) {
      if (subnode->next)
        subnode->parent->chld = subnode->next;
      else
        subnode->parent->chld = subnode->prev;
    }
  }

  if (subnode->next)
    subnode->next->prev = subnode->prev;

  if (subnode->prev)
    subnode->prev->next = subnode->next;

  if (parent->chld)
    parent->chld->prev = subnode;

  subnode->next   = parent->chld;
  subnode->prev   = NULL;
  subnode->parent = parent;
  parent->chld    = subnode;

  /* fix node transforms after attached to new node */
  if (fixCoordSys)
    ak_fixNodeCoordSys(subnode);
}

AK_EXPORT
AkNode *
ak_nodeMake(AkDoc      * __restrict doc,
            AkNode     * __restrict parent,
            const char * __restrict name) {
  AkHeap *heap;
  AkNode *node;
  void   *memparent;

  if (!doc) return NULL;

  heap      = ak_heap_getheap(doc);
  memparent = parent ? (void *)parent : (void *)doc;
  node      = ak_heap_calloc(heap, memparent, sizeof(*node));
  node->visible = true;

  if (name)
    node->name = ak_heap_strdup(heap, node, name);

  /* Attach to the parent's children chain. fixCoordSys=false — the
     caller is presumed to know the desired orientation; baking
     coord-sys conversions into a programmatically created node
     would surprise more often than help. */
  if (parent)
    ak_addSubNode(parent, node, false);

  return node;
}

AK_EXPORT
AkNode *
ak_nodeFindChildByName(AkNode     * __restrict parent,
                       const char * __restrict name) {
  AkNode *child;

  if (!parent || !name) return NULL;

  for (child = parent->chld; child; child = child->next) {
    if (child->name && strcmp(child->name, name) == 0)
      return child;
  }

  return NULL;
}

AK_EXPORT
AkNode *
ak_nodeFindOrMakeChild(AkDoc      * __restrict doc,
                       AkNode     * __restrict parent,
                       const char * __restrict name) {
  AkNode *existing;

  existing = ak_nodeFindChildByName(parent, name);
  if (existing) return existing;

  return ak_nodeMake(doc, parent, name);
}

AK_EXPORT
AkInstanceBase *
ak_nodeAttachCamera(AkNode   * __restrict node,
                    AkCamera * __restrict cam) {
  AkHeap         *heap;
  AkInstanceBase *inst, *head;

  if (!node || !cam) return NULL;

  heap       = ak_heap_getheap(node);
  inst       = ak_instanceMake(heap, node, cam);
  inst->type = AK_INSTANCE_CAMERA;
  /* `ak_instanceMake` doesn't backfill `node`; do it here so
     downstream APIs (`ak_instanceName`, list-unlink, move-to-subnode)
     don't see a NULL parent. */
  inst->node = node;

  /* Chain in front of any existing camera instance(s) on the node.
     Maintain `prev` on the existing head so the chain stays
     doubly-linked. */
  head         = node->camera;
  inst->next   = head;
  inst->prev   = NULL;
  if (head) {
    head->prev = inst;
  }
  node->camera = inst;

  return inst;
}

AK_EXPORT
AkInstanceBase *
ak_nodeAttachLight(AkNode  * __restrict node,
                   AkLight * __restrict light) {
  AkHeap         *heap;
  AkInstanceBase *inst, *head;

  if (!node || !light) return NULL;

  heap       = ak_heap_getheap(node);
  inst       = ak_instanceMake(heap, node, light);
  inst->type = AK_INSTANCE_LIGHT;
  inst->node = node;

  head        = node->light;
  inst->next  = head;
  inst->prev  = NULL;
  if (head) {
    head->prev = inst;
  }
  node->light = inst;

  return inst;
}

AK_EXPORT
void
ak_nodeSetTransformMatrix(AkNode * __restrict node,
                          const float         matrix[16]) {
  AkHeap   *heap;
  AkObject *obj;
  AkMatrix *mat;

  if (!node || !matrix) return;

  heap = ak_heap_getheap(node);

  /* Wrap the matrix in a typed AkObject (AKT_MATRIX). The inline
     payload `data[]` is sized to hold one AkMatrix; ak_objAlloc
     also sets pData to point at it. */
  obj = ak_objAlloc(heap, node, sizeof(AkMatrix), AKT_MATRIX, true);
  mat = (AkMatrix *)ak_objGet(obj);
  memcpy(mat->val, matrix, sizeof(float) * 16);

  /* Lazily allocate the AkTransform shell — newly-created nodes
     (e.g. via ak_nodeMake) have transform == NULL until the first
     write. */
  if (!node->transform)
    node->transform = ak_heap_calloc(heap, node, sizeof(AkTransform));

  /* Replace any existing transform item chain with the single
     matrix. The AKT_MATRIX form composes the same world-pose, so
     callers don't have to track a TRS chain themselves. */
  node->transform->item = obj;
}

AK_EXPORT
AkNode *
ak_sceneFindRoot(AkScene    * __restrict scene,
                 const char * __restrict name) {
  AkNode *node;

  if (!scene || !name) return NULL;

  for (node = scene->node; node; node = node->next) {
    if (node->name && strcmp(node->name, name) == 0)
      return node;
  }

  return NULL;
}

AK_EXPORT
AkNode *
ak_sceneFindOrMakeRoot(AkDoc      * __restrict doc,
                       AkScene    * __restrict scene,
                       const char * __restrict name) {
  AkHeap *heap;
  AkNode *node;
  AkNode *last;

  if (!doc || !scene) return NULL;

  if ((node = ak_sceneFindRoot(scene, name)))
    return node;

  /* Allocate the new root node parented on the visual scene so its
     lifetime tracks the scene's. ak_nodeMake parents under another
     AkNode, which is the wrong shape here — we want a root, not a
     child — so we allocate directly. */
  heap = ak_heap_getheap(doc);
  node = ak_heap_calloc(heap, scene, sizeof(*node));
  node->visible = true;
  if (name)
    node->name = ak_heap_strdup(heap, node, name);

  /* Append to the visual scene's root chain. Empty scene → become
     the head; otherwise walk to the tail and link in. */
  if (!scene->node) {
    scene->node = node;
  } else {
    for (last = scene->node; last->next; last = last->next) { }
    last->next = node;
    node->prev = last;
  }

  return node;
}
