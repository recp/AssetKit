/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "../ak_id.h"

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
