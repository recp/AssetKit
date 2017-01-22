/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_material_h
#define ak_material_h
#ifdef __cplusplus
extern "C" {
#endif

typedef struct AkFxColorOrTex {
  AkColor     * color;
  AkParam     * param;
  AkFxTexture * texture;
  AkOpaque      opaque;
} AkFxColorOrTex;

typedef AkFxColorOrTex AkAmbient;
typedef AkFxColorOrTex AkDiffuse;
typedef AkFxColorOrTex AkEmission;
typedef AkFxColorOrTex AkReflective;
typedef AkFxColorOrTex AkSpecular;
typedef AkFxColorOrTex AkTransparent;

typedef struct AkFxFloatOrParam {
  float   *val;
  AkParam *param;
} AkFxFloatOrParam;

typedef AkFxFloatOrParam AkIndexOfRefraction;
typedef AkFxFloatOrParam AkReflectivity;
typedef AkFxFloatOrParam AkShininess;
typedef AkFxFloatOrParam AkTransparency;

typedef struct AkConstantFx {
  AkEmission          * emission;
  AkReflective        * reflective;
  AkReflectivity      * reflectivity;
  AkTransparent       * transparent;
  AkTransparency      * transparency;
  AkIndexOfRefraction * indexOfRefraction;
} AkConstantFx;

typedef struct AkLambert {
  AkEmission          * emission;
  AkAmbient           * ambient;
  AkDiffuse           * diffuse;
  AkReflective        * reflective;
  AkReflectivity      * reflectivity;
  AkTransparent       * transparent;
  AkTransparency      * transparency;
  AkIndexOfRefraction * indexOfRefraction;
} AkLambert;

typedef struct AkPhong {
  AkEmission          * emission;
  AkAmbient           * ambient;
  AkDiffuse           * diffuse;
  AkSpecular          * specular;
  AkShininess         * shininess;
  AkReflective        * reflective;
  AkReflectivity      * reflectivity;
  AkTransparent       * transparent;
  AkTransparency      * transparency;
  AkIndexOfRefraction * indexOfRefraction;
} AkPhong;

typedef AkPhong AkBlinn;

#ifdef __cplusplus
}
#endif
#endif /* ak_material_h */
