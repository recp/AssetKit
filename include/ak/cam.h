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

#ifndef assetkit_cam_h
#define assetkit_cam_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

struct AkLibraryItemBase;

typedef enum AkProjectionType {
  AK_PROJECTION_PERSPECTIVE  = 0, /* default */
  AK_PROJECTION_ORTHOGRAPHIC = 1,
  AK_PROJECTION_OTHER        = 2
} AkProjectionType;

typedef struct AkProjection {
  AkProjectionType type;
  uint32_t         tag;
} AkProjection;

typedef struct AkPerspective {
  AkProjection base;
  float        xfov;
  float        yfov;
  float        aspectRatio;
  float        znear;
  float        zfar;
} AkPerspective;

typedef struct AkOrthographic {
  AkProjection base;
  float        xmag;
  float        ymag;
  float        aspectRatio;
  float        znear;
  float        zfar;
} AkOrthographic;

typedef struct AkOptics {
  AkProjection *tcommon;
  AkTechnique  *technique;
} AkOptics;

typedef struct AkImager {
  AkTechnique *technique;
  AkTree      *extra;
} AkImager;

typedef struct AkCamera {
  /* const char * id; */
  AkOneWayIterBase base;
  const char      *name;
  AkOptics        *optics;
  AkImager        *imager;
  AkTree          *extra;
} AkCamera;

/*!
 * @brief Top camera in scene if available
 *
 * pass NULL which parameter is not wanted and reduce overhead of calc these 
 * params, only pass param[s] which you need
 *
 * @warning the returned projection matrix's aspect ratio is same as exported
 *
 * @param[in]  doc        document (required)
 * @param[out] camera     found camera (optional)
 * @param[out] matrix     transform matrix (optional) of camera in world
 * @param[out] projMatrix proj matrix (optional)
 *
 * @return AK_OK if camera found else AK_EFOUND
 */
AK_EXPORT
AkResult
ak_firstCamera(AkDoc     * __restrict doc,
               AkCamera ** camera,
               float     * matrix,
               float     * projMatrix);

AK_EXPORT
const AkCamera*
ak_defaultCamera(void * __restrict memparent);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_cam_h */
