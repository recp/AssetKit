/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"

AK_EXPORT
void *
ak_instanceObject(AkDoc * __restrict doc,
                  AkInstanceBase *instanceBase) {
  if (!instanceBase->object)
    instanceBase->object = ak_getObjectByUrl(&instanceBase->url);

  return instanceBase->object;
}

AK_EXPORT
AkGeometry *
ak_instanceObjectGeom(AkDoc * __restrict doc,
                      AkNode * node) {
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
