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

#include "common.h"

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
    ak_heap_setpm(objectId,
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
