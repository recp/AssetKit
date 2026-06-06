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

#ifndef assetkit_light_h
#define assetkit_light_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

typedef enum AkLightType {
  AK_LIGHT_TYPE_AMBIENT     = 1,
  AK_LIGHT_TYPE_DIRECTIONAL = 2,
  AK_LIGHT_TYPE_POINT       = 3,
  AK_LIGHT_TYPE_SPOT        = 4,
  AK_LIGHT_TYPE_CUSTOM      = 5
} AkLightType;

typedef struct AkLightBase {
  AkLightType type;
  uint32_t    ctype; /* custom type, because type always is custom */
  AkColor     color;
  AkFloat3    direction;
  float       intensity;
  float       range;
} AkLightBase;

typedef struct AkLightAttenuation {
  float constant;
  float linear;
  float quadratic;
} AkLightAttenuation;

typedef AkLightBase AkAmbientLight;
typedef AkLightBase AkDirectionalLight;

typedef struct AkPointLight {
  AkLightBase        base;
  AkLightAttenuation attenuation;
} AkPointLight;

typedef struct AkSpotLight {
  AkLightBase        base;
  AkLightAttenuation attenuation;
  float              innerConeAngle;
  float              outerConeAngle;
  float              coneFalloffExponent;
} AkSpotLight;

typedef struct AkLight {
  /* const char * id; */
  const char     *name;
  AkLightBase    *data;
  AkTree         *extra;
  struct AkLight *next;
} AkLight;

AK_EXPORT
AkLight*
ak_defaultLight(void * __restrict memparent);

/*!
 * @brief Allocate a light of the given type with sensible defaults
 *        (white color, downward direction for directional/spot,
 *        unit intensity, infinite range, 45° spot outer cone)
 *        and register it in the document's lights library.
 *
 * Wires up AkLight + the matching data variant (AkLightBase for
 * ambient/directional/point, or AkSpotLight) in one call.
 * Pair with ak_nodeAttachLight() to expose the light in the scene
 * tree.
 *
 * @param[in]  doc        document the light will live in (required)
 * @param[in]  memparent  heap parent for ownership (NULL → doc)
 * @param[in]  type       AK_LIGHT_TYPE_AMBIENT/DIRECTIONAL/POINT/SPOT
 *
 * @return Newly allocated AkLight, or NULL on failure (unsupported
 *         type, allocation failure, etc.)
 */
AK_EXPORT
AkLight *
ak_lightMake(AkDoc * __restrict doc,
             void  * __restrict memparent,
             AkLightType type);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_light_h */
