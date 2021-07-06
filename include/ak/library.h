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

#ifndef assetkit_lib_h
#define assetkit_lib_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "cam.h"
#include "light.h"

typedef struct AkLibrary {
  /* const char * id; */

  struct AkLibrary *next;
  const char       *name;
  AkTree           *extra;
  AkOneWayIterBase *chld;
  uint64_t          count;
} AkLibrary;

AK_EXPORT
AkGeometry *
ak_libFirstGeom(AkDoc * __restrict doc);

AK_EXPORT
AkResult
ak_libAddCamera(AkDoc * __restrict doc, AkCamera * __restrict cam);

AK_EXPORT
AkResult
ak_libAddLight(AkDoc * __restrict doc, AkLight * __restrict light);

AK_EXPORT
void
ak_libInsertInto(AkLibrary *lib, void *item, int32_t prevoff, int32_t nextoff);

AK_EXPORT
AkLibrary*
ak_libFirstOrCreat(AkDoc * __restrict doc, uint32_t itemOffset);

AK_EXPORT
AkLibrary*
ak_libImageFirstOrCreat(AkDoc * __restrict doc);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_lib_h */
