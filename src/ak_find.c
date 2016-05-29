/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"

void *
AK_EXPORT
ak_getId(const char * __restrict objptr) {
  return ak_mem_getId(objptr);
}

AkResult
AK_EXPORT
ak_setId(void * __restrict objptr,
         const char * __restrict objectId) {
  ak_mem_setId(objptr, objectId);
  return AK_OK;
}

void *
AK_EXPORT
ak_getObjectById(AkDoc * __restrict doc,
                 const char * __restrict objectId) {
  AkResult ret;
  void    *foundObject;

  ret = ak_mem_getMemById(doc,
                          objectId,
                          &foundObject);

  return foundObject;
}

void *
AK_EXPORT
ak_getObjectByUrl(AkDoc * __restrict doc,
                  const char * __restrict objectUrl) {
  if (*objectUrl == '#')
    return ak_getObjectById(doc, objectUrl + 1);

  return NULL;
}
