/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"

AK_EXPORT
void *
ak_getId(void * __restrict objptr) {
  return ak_mem_getId((void *)objptr);
}

AK_EXPORT
AkResult
ak_setId(void * __restrict objptr,
         const char * __restrict objectId) {
  ak_mem_setId(objptr, (void *)objectId);
  return AK_OK;
}

AK_EXPORT
AkResult
ak_moveId(void * __restrict objptrOld,
          void * __restrict objptrNew) {
  char *objectId;

  objectId = ak_getId(objptrOld);
  if (objectId) {
    ak_heap_setpm(ak_heap_getheap(objptrOld),
                  objectId,
                  objptrNew);

    ak_setId(objptrOld, NULL);
    ak_setId(objptrNew, objectId);
  }
  return AK_OK;
}

AK_EXPORT
void *
ak_getObjectById(AkDoc * __restrict doc,
                 const char * __restrict objectId) {
  void *foundObject;

  ak_mem_getMemById(doc,
                    (void *)objectId,
                    &foundObject);

  return foundObject;
}
