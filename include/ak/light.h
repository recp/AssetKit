/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_light_h
#define ak_light_h
#ifdef __cplusplus
extern "C" {
#endif

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
} AkLightBase;

typedef AkLightBase AkAmbientLight;
typedef AkLightBase AkDirectionalLight;

typedef struct AkPointLight {
  AkLightBase base;
  float       constAttn;
  float       linearAttn;
  float       quadAttn;
} AkPointLight;

typedef struct AkSpotLight {
  AkLightBase base;
  float       constAttn;
  float       linearAttn;
  float       quadAttn;
  float       falloffAngle;
  float       falloffExp;
} AkSpotLight;

typedef struct AkLight {
  /* const char * id; */
  const char     *name;
  AkLightBase    *tcommon;
  AkTechnique    *technique;
  AkTree         *extra;
  struct AkLight *next;
} AkLight;

AK_EXPORT
AkLight*
ak_defaultLight(void * __restrict memparent);
  
#ifdef __cplusplus
}
#endif
#endif /* ak_light_h */
