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

#include "ak-cam.h"
#include "ak-light.h"

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

#ifdef __cplusplus
}
#endif
#endif /* ak_lib_h */
