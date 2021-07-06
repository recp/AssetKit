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

#ifndef assetkit_material_h
#define assetkit_material_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "texture.h"

typedef enum AkGlMaterialType {
  AK_GL_MATERIAL_TYPE_EMISSION            = 1,
  AK_GL_MATERIAL_TYPE_AMBIENT             = 2,
  AK_GL_MATERIAL_TYPE_DIFFUSE             = 3,
  AK_GL_MATERIAL_TYPE_SPECULAR            = 4,
  AK_GL_MATERIAL_TYPE_AMBIENT_AND_DIFFUSE = 5
} AkGlMaterialType;

typedef enum AkOpaque {
  AK_OPAQUE_OPAQUE   = 0, /* fully opaque */
  AK_OPAQUE_A_ONE    = 1,
  AK_OPAQUE_A_ZERO   = 2,
  AK_OPAQUE_RGB_ONE  = 3,
  AK_OPAQUE_RGB_ZERO = 4,
  AK_OPAQUE_BLEND    = 5, /* Blend only */
  AK_OPAQUE_MASK     = 6, /* Activate alpha cutoff */
  AK_OPAQUE_DEFAULT  = AK_OPAQUE_OPAQUE
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
  AkColor      *color;
  AkParam      *param;
  AkTextureRef *texture;
} AkColorDesc;

typedef struct AkFloatOrParam {
  float   *val;
  AkParam *param;
} AkFloatOrParam;

typedef struct AkTransparent {
  AkColorDesc    *color;
  AkFloatOrParam *amount;
  AkOpaque        opaque;
  float           cutoff;
} AkTransparent;

typedef struct AkReflective {
  AkColorDesc    *color;
  AkFloatOrParam *amount;
} AkReflective;

typedef struct AkOcclusion {
  AkTextureRef *tex;
  float         strength;
} AkOcclusion;

typedef struct AkNormalMap {
  AkTextureRef *tex;
  float         scale;
} AkNormalMap;

typedef struct AkTechniqueFxCommon {
  AkColorDesc    *ambient;
  AkColorDesc    *emission;
  AkColorDesc    *diffuse;
  AkColorDesc    *specular;
  AkFloatOrParam *shininess;

  AkTransparent  *transparent;
  AkReflective   *reflective;
  AkFloatOrParam *indexOfRefraction;

  AkOcclusion    *occlusion;
  AkNormalMap    *normal;

  AkMaterialType  type;
  bool            doubleSided;
} AkTechniqueFxCommon;

/* Common PBR Materials */

typedef struct AkMetallicRoughness {
  AkTechniqueFxCommon base;
  AkColor             albedo;
  AkTextureRef       *albedoTex;
  AkTextureRef       *metalRoughTex;
  float               metallic;
  float               roughness;
} AkMetallicRoughness;

typedef struct AkSpecularGlossiness {
  AkTechniqueFxCommon base;
  AkColor             diffuse;
  AkColor             specular;
  AkTextureRef       *diffuseTex;
  AkTextureRef       *specGlossTex;
  float               glossiness;
} AkSpecularGlossiness;

#ifdef __cplusplus
}
#endif
#endif /* assetkit_material_h */
