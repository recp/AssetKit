/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"

AkResult
AK_EXPORT
ak_setId(void * __restrict objptr,
         const char * __restrict objectId) {
  ak_mem_setId(objptr, objectId);
  return AK_OK;
}

void *
AK_EXPORT
ak_getObjectById(const char * __restrict objectId) {
  AkResult ret;
  void    *foundObject;

  ret = ak_mem_getMemById(objectId,
                          &foundObject);

  return foundObject;
}
