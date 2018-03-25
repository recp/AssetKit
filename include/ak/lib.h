/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_lib_h
#define ak_lib_h
#ifdef __cplusplus
extern "C" {
#endif

#include "cam.h"
#include "light.h"

AK_EXPORT
AkGeometry *
ak_libFirstGeom(AkDoc * __restrict doc);

AK_EXPORT
AkResult
ak_libAddCamera(AkDoc    * __restrict doc,
                AkCamera * __restrict cam);

AK_EXPORT
AkResult
ak_libAddLight(AkDoc   * __restrict doc,
               AkLight * __restrict light);

AK_EXPORT
void
ak_libInsertInto(AkLibItem *lib,
                 void      *item,
                 int32_t    prevOffset,
                 int32_t    nextOffset);

AK_EXPORT
AkLibItem*
ak_libFirstOrCreat(AkDoc * __restrict doc,
                   uint32_t           itemOffset);

AK_EXPORT
AkLibItem*
ak_libImageFirstOrCreat(AkDoc * __restrict doc);

#ifdef __cplusplus
}
#endif
#endif /* ak_lib_h */
