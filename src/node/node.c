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

static
AkInstanceBase *
ak__nodePrependInstance(AkInstanceBase *head,
                        AkInstanceBase *inst) {
  inst->prev = NULL;
  inst->next = head;

  if (head)
    head->prev = inst;

  return inst;
}

static
AkNodeRef *
ak__nodePrependRef(AkNodeRef *head,
                   AkNodeRef *ref) {
  ref->prev = NULL;
  ref->next = head;

  if (head)
    head->prev = ref;

  return ref;
}

AK_EXPORT
AkInstanceBase *
ak_nodeAttachInstance(AkNode         * __restrict node,
                      AkInstanceBase * __restrict inst) {
  if (!node || !inst)
    return NULL;

  switch (inst->type) {
    case AK_INSTANCE_GEOMETRY:
      inst->node = node;
      ak_heap_setpm(inst, node);
      node->geometry = (AkInstanceGeometry *)
        ak__nodePrependInstance((AkInstanceBase *)node->geometry, inst);
      break;
    case AK_INSTANCE_CAMERA:
      inst->node = node;
      ak_heap_setpm(inst, node);
      node->camera = ak__nodePrependInstance(node->camera, inst);
      break;
    case AK_INSTANCE_LIGHT:
      inst->node = node;
      ak_heap_setpm(inst, node);
      node->light = ak__nodePrependInstance(node->light, inst);
      break;
    default:
      return NULL;
  }

  return inst;
}

AK_EXPORT
AkInstanceGeometry *
ak_nodeAttachGeometry(AkNode     * __restrict node,
                      AkGeometry * __restrict geometry) {
  AkHeap             *heap;
  AkInstanceGeometry *inst;

  if (!node || !geometry)
    return NULL;

  heap = ak_heap_getheap(node);
  inst = ak_instanceMakeGeom(heap, node, geometry);

  return (AkInstanceGeometry *)ak_nodeAttachInstance(node, &inst->base);
}

AK_EXPORT
AkNode *
ak_nodeRefTarget(AkNodeRef * __restrict ref) {
  void *target;

  if (!ref)
    return NULL;

  if (ref->target)
    return ref->target;

  if (!ref->reserved)
    return NULL;

  target = ak_getObjectByUrl((AkURL *)ref->reserved);
  if (target && ak_typeid(target) == AKT_NODE)
    ref->target = target;

  return ref->target;
}

AK_EXPORT
AkNodeRef *
ak_nodeAttachNodeRef(AkNode * __restrict owner,
                     AkNode * __restrict target) {
  AkHeap    *heap;
  AkNodeRef *ref;

  if (!owner)
    return NULL;

  heap = ak_heap_getheap(owner);
  ref  = ak_heap_calloc(heap, owner, sizeof(*ref));

  ref->owner  = owner;
  ref->target = target;

  owner->nodeRefs = ak__nodePrependRef(owner->nodeRefs, ref);

  return ref;
}

AK_EXPORT
AkInstanceBase *
ak_nodeAttachCamera(AkNode   * __restrict node,
                    AkCamera * __restrict cam) {
  AkHeap         *heap;
  AkInstanceBase *inst;

  if (!node || !cam) return NULL;

  heap       = ak_heap_getheap(node);
  inst       = ak_instanceMake(heap, node, cam);
  inst->type = AK_INSTANCE_CAMERA;

  return ak_nodeAttachInstance(node, inst);
}

AK_EXPORT
AkInstanceBase *
ak_nodeAttachLight(AkNode  * __restrict node,
                   AkLight * __restrict light) {
  AkHeap         *heap;
  AkInstanceBase *inst;

  if (!node || !light) return NULL;

  heap       = ak_heap_getheap(node);
  inst       = ak_instanceMake(heap, node, light);
  inst->type = AK_INSTANCE_LIGHT;

  return ak_nodeAttachInstance(node, inst);
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

  if (scene->node) {
    AkNodeRef *ref;

    for (ref = scene->node->nodeRefs; ref; ref = ref->next) {
      node = ak_nodeRefTarget(ref);
      if (node && node->name && strcmp(node->name, name) == 0)
        return node;
    }
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

  if (!doc || !scene) return NULL;

  if ((node = ak_sceneFindRoot(scene, name)))
    return node;

  heap = ak_heap_getheap(doc);

  if (!scene->node) {
    scene->node = ak_heap_calloc(heap, scene, sizeof(*scene->node));
    ak_setypeid(scene->node, AKT_NODE);
    scene->node->visible = true;
  }

  node = ak_heap_calloc(heap, doc, sizeof(*node));
  ak_setypeid(node, AKT_NODE);
  node->visible = true;
  if (name)
    node->name = ak_heap_strdup(heap, node, name);

  AK_LIB_PREPEND(doc->lib.nodes, node, docNext);
  ak_nodeAttachNodeRef(scene->node, node);

  return node;
}
