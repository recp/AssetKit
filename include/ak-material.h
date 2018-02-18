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

typedef enum AkAlphaMode {
  AK_ALPHA_OPAQUE,
  AK_ALPHA_MASK,
  AK_ALPHA_BLEND
} AkAlphaMode;

typedef enum AkOpaque {
  AK_OPAQUE_A_ONE    = 0,
  AK_OPAQUE_RGB_ZERO = 1,
  AK_OPAQUE_A_ZERO   = 2,
  AK_OPAQUE_RGB_ONE  = 3,
  AK_OPAQUE_DEFAULT  = AK_OPAQUE_A_ONE
} AkOpaque;

typedef enum AkMaterialType {
  AK_MATERIAL_PHONG              = 1,
  AK_MATERIAL_BLINN              = 2,
  AK_MATERIAL_LAMBERT            = 3,
  AK_MATERIAL_CONSTANT           = 4,
  AK_MATERIAL_METALLIC_ROUGHNESS = 5,  /* PBR Material */
  AK_MATERIAL_SPECULAR_GLOSSINES = 6   /* PBR Material */
} AkMaterialType;

typedef struct AkFxColorOrTex {
  AkColor     *color;
  AkParam     *param;
  AkFxTexture *texture;
  AkOpaque     opaque;
} AkFxColorOrTex;

typedef struct AkFxFloatOrParam {
  float   *val;
  AkParam *param;
} AkFxFloatOrParam;

typedef struct AkTransparent {
  AkFxColorOrTex   *color;
  AkFxFloatOrParam *amount;
  AkAlphaMode       mode;
  float             cutoff;
} AkTransparent;

typedef struct AkReflective {
  AkFxColorOrTex   *color;
  AkFxFloatOrParam *amount;
} AkReflective;

typedef struct AkEffectCmnTechnique {
  AkMaterialType    type;
  AkTransparent    *transparent;
  AkReflective     *reflective;
  AkFxFloatOrParam *indexOfRefraction;
} AkEffectCmnTechnique;

/* Common materials */

typedef struct AkConstantFx {
  AkEffectCmnTechnique base;
  AkFxColorOrTex      *emission;
} AkConstantFx;

typedef struct AkLambert {
  AkEffectCmnTechnique base;
  AkFxColorOrTex      *emission;
  AkFxColorOrTex      *ambient;
  AkFxColorOrTex      *diffuse;
} AkLambert;

typedef struct AkPhong {
  AkEffectCmnTechnique base;
  AkFxColorOrTex      *emission;
  AkFxColorOrTex      *ambient;
  AkFxColorOrTex      *diffuse;
  AkFxColorOrTex      *specular;
  AkFxFloatOrParam    *shininess;
} AkPhong;

typedef AkPhong AkBlinn;

/* Common PBR Materials */

typedef struct AkMetallicRoughness {
  AkEffectCmnTechnique base;
  AkColor              baseColor;
  AkFxTexture         *baseTexture;
  AkFxTexture         *metallicRoughnessTexture;
  float                metallic;
  float                roughness;
} AkMetallicRoughness;

typedef struct AkSpecularGlossiness {
  AkEffectCmnTechnique base;
  AkColor              diffuse;
  AkColor              specular;
  AkFxTexture         *diffuseTexture;
  AkFxTexture         *specularGlossinessTexture;
  float                glossiness;
} AkSpecularGlossiness;

#ifdef __cplusplus
}
#endif
#endif /* ak_material_h */
