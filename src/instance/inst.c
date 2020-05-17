/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "../id.h"

AK_EXPORT
AkInstanceBase*
ak_instanceMake(AkHeap * __restrict heap,
                void   * __restrict memparent,
                void   * __restrict object) {
  AkInstanceBase *instance;
  const char     *id;

  if (!object || !memparent || !heap)
    return NULL;

  instance = ak_heap_calloc(heap,
                            memparent,
                            sizeof(*instance));

  /* we already have the object */
  instance->object = object;

  /* target must have id or we will generate an id for it */
  id = ak_getId(object);
  if (!id)
    id = ak_id_gen(heap,
                   object,
                   NULL);

  ak_url_init_with_id(heap->allocator,
                      instance,
                      (char *)id,
                      &instance->url);

  return instance;
}

AK_EXPORT
void *
ak_instanceObject(AkInstanceBase *instanceBase) {
  if (!instanceBase)
    return NULL;

  if (!instanceBase->object)
    instanceBase->object = ak_getObjectByUrl(&instanceBase->url);

  return instanceBase->object;
}

AK_EXPORT
AkNode *
ak_instanceObjectNode(AkNode * node) {
  AkInstanceBase *instanceBase;

  instanceBase = &node->node->base;

  if (!instanceBase->object)
    instanceBase->object = ak_getObjectByUrl(&instanceBase->url);

  return instanceBase->object;
}

AK_EXPORT
AkGeometry *
ak_instanceObjectGeom(AkNode * node) {
  AkInstanceBase *instanceBase;

  instanceBase = &node->geometry->base;

  if (!instanceBase->object)
    instanceBase->object = ak_getObjectByUrl(&instanceBase->url);

  return instanceBase->object;
}

AK_EXPORT
AkGeometry *
ak_instanceObjectGeomId(AkDoc * __restrict doc,
                        const char * id) {
  AkNode         *node;
  AkInstanceBase *instanceBase;

  node = ak_getObjectById(doc, id);
  if (!node)
    return NULL;

  instanceBase = &node->geometry->base;

  if (!instanceBase->object)
    instanceBase->object = ak_getObjectByUrl(&instanceBase->url);

  return instanceBase->object;
}

AK_EXPORT
AkNode*
ak_instanceMoveToSubNode(AkNode * __restrict node,
                         AkInstanceBase     *inst) {
  AkHeap *heap;
  AkNode *subNode;
  size_t  off;

  heap    = ak_heap_getheap(node);
  subNode = ak_heap_calloc(heap, node, sizeof(*node));

  switch (inst->type) {
    case AK_INSTANCE_GEOMETRY:
      off = offsetof(AkNode, geometry);
      break;
    case AK_INSTANCE_LIGHT:
      off = offsetof(AkNode, light);
      break;
    case AK_INSTANCE_CAMERA:
      off = offsetof(AkNode, camera);
      break;
    case AK_INSTANCE_NODE:
      off = offsetof(AkNode, node);
      break;
    default:
      ak_free(subNode);
      return NULL;
      break;
  }

  if (inst->prev)
    inst->prev->next = inst->next;

  if (inst->next)
    inst->next->prev = inst->prev;

  if (*(void **)((char *)node + off) == inst)
    *(void **)((char *)node + off) = NULL;

  *(void **)((char *)subNode + off) = inst;

  inst->node = subNode;
  ak_heap_setpm(inst, subNode);

  ak_addSubNode(node, subNode, false);

  return subNode;
}

AK_EXPORT
AkNode*
ak_instanceMoveToSubNodeIfNeeded(AkNode * __restrict node,
                                 AkInstanceBase     *inst) {
  void           *instObj,  *parentObject;
  AkCoordSys     *coordSys, *instCoordSys;
  AkInstanceBase *insti;
  AkInstanceBase *instArray[] = {(AkInstanceBase *)node->geometry,
                                 (AkInstanceBase *)node->node,
                                 node->camera,
                                 node->light};
  int             i, instArrayLen;

  instObj = ak_instanceObject(inst);
  if (!instObj)
    goto ret;

  /* check if node coordsys is equal to instance */
  coordSys     = ak_getCoordSys(node);
  instCoordSys = ak_getCoordSys(instObj);

  if (!ak_hasCoordSys(instObj))
    goto ret;

  parentObject = node->parent;
  if (!parentObject)
    parentObject = ak_mem_parent(node);

  if ((ak_hasCoordSys(node)
       && coordSys != instCoordSys)
      || (!node->parent && ak_typeid(parentObject) == AKT_SCENE))
    goto move;

  /* check all instances coord sys in this node */
  instArrayLen = AK_ARRAY_LEN(instArray);
  for (i = 0; i < instArrayLen; i++) {
    insti = instArray[i];
    while (insti) {
      if (insti != inst) {
        void *instObji;
        instObji = ak_instanceObject(insti);
        if (!ak_hasCoordSys(instObji)
            || ak_getCoordSys(instObji) != instCoordSys) {
          goto move;
        }
      }
      insti = insti->next;
    }
  }

ret:
  return NULL;
move:
  return ak_instanceMoveToSubNode(node, inst);
}
