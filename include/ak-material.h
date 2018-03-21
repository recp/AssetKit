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
  AK_OPAQUE_A_ONE    = 0, /* Default */
  AK_OPAQUE_A_ZERO   = 1,
  AK_OPAQUE_RGB_ONE  = 2,
  AK_OPAQUE_RGB_ZERO = 3,
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

typedef struct AkColorDesc {
  AkColor     *color;
  AkParam     *param;
  AkTextureRef *texture;
} AkColorDesc;

typedef struct AkFloatOrParam {
  float   *val;
  AkParam *param;
} AkFloatOrParam;

typedef struct AkTransparent {
  AkColorDesc    *color;
  AkFloatOrParam *amount;
  AkAlphaMode     mode;
  AkOpaque        opaque;
  float           cutoff;
} AkTransparent;

typedef struct AkReflective {
  AkColorDesc    *color;
  AkFloatOrParam *amount;
} AkReflective;

typedef struct AkEffectCmnTechnique {
  AkMaterialType  type;
  AkTransparent  *transparent;
  AkReflective   *reflective;
  AkFloatOrParam *indexOfRefraction;
  AkColorDesc    *ambient;
  AkColorDesc    *emission;
  AkColorDesc    *diffuse;
  AkColorDesc    *specular;
  AkFloatOrParam *shininess;
} AkEffectCmnTechnique;

/* Common materials */

typedef struct AkConstantFx {
  AkEffectCmnTechnique base;
  AkColorDesc         *emission;
} AkConstantFx;

typedef struct AkLambert {
  AkEffectCmnTechnique base;
  AkColorDesc         *emission;
  AkColorDesc         *ambient;
  AkColorDesc         *diffuse;
} AkLambert;

typedef struct AkPhong {
  AkEffectCmnTechnique base;
  AkColorDesc          *emission;
  AkColorDesc          *ambient;
  AkColorDesc          *diffuse;
  AkColorDesc          *specular;
  AkFloatOrParam       *shininess;
} AkPhong;

typedef AkPhong AkBlinn;

/* Common PBR Materials */

typedef struct AkMetallicRoughness {
  AkEffectCmnTechnique base;
  AkColor              baseColor;
  AkTextureRef        *baseColorTex;
  AkTextureRef        *metalRoughTex;
  float                metallic;
  float                roughness;
} AkMetallicRoughness;

typedef struct AkSpecularGlossiness {
  AkEffectCmnTechnique base;
  AkColor              diffuse;
  AkColor              specular;
  AkTextureRef        *diffuseTex;
  AkTextureRef        *specularGlossTex;
  float                glossiness;
} AkSpecularGlossiness;

#ifdef __cplusplus
}
#endif
#endif /* ak_material_h */
