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

#ifndef ak_src_material_legacy_h
#define ak_src_material_legacy_h

#define AK_INSTANCE_EFFECT ((AkInstanceType)7)

typedef struct AkBind             AkBind;
typedef struct AkBindVertexInput  AkBindVertexInput;
typedef struct AkEffect           AkEffect;
typedef struct AkEvaluateScene    AkEvaluateScene;
typedef struct AkInstanceMaterial AkInstanceMaterial;
typedef struct AkNewParam         AkNewParam;
typedef struct AkParam            AkParam;
typedef struct AkProfile          AkProfile;
typedef struct AkRender           AkRender;
typedef struct AkTechnique        AkTechnique;
typedef struct AkTechniqueFx      AkTechniqueFx;
typedef struct AkTechniqueHint    AkTechniqueHint;

typedef struct AkTechnique {
  const char         *profile;
  const char         *xmlns;
  AkTree             *chld;
  AkTechnique        *next;
} AkTechnique;

typedef struct AkParam {
  const char *ref;
  AkParam    *prev;
  AkParam    *next;
} AkParam;

typedef struct AkEvaluateTarget {
  AkParam        *param;
  AkInstanceBase *instanceImage;
  unsigned long   index;
  unsigned long   slice;
  unsigned long   mip;
  AkFace          face;
} AkEvaluateTarget;

typedef enum AkProfileType {
  AK_PROFILE_TYPE_UNKOWN =-1,
  AK_PROFILE_TYPE_COMMON = 0,
  AK_PROFILE_TYPE_GLTF   = 6
} AkProfileType;

typedef struct AkColorDesc {
  AkColor      *color;
  AkParam      *param;
  AkTextureRef *texture;
} AkColorDesc;

typedef enum AkOpaque {
  AK_OPAQUE_OPAQUE   = 0,
  AK_OPAQUE_A_ONE    = 1,
  AK_OPAQUE_A_ZERO   = 2,
  AK_OPAQUE_RGB_ONE  = 3,
  AK_OPAQUE_RGB_ZERO = 4,
  AK_OPAQUE_BLEND    = 5,
  AK_OPAQUE_MASK     = 6,
  AK_OPAQUE_DEFAULT  = AK_OPAQUE_OPAQUE
} AkOpaque;

typedef struct AkTransparent {
  AkColorDesc *color;
  float        amount;
  AkOpaque     opaque;
  float        cutoff;
} AkTransparent;

typedef struct AkReflective {
  AkColorDesc *color;
  float        amount;
} AkReflective;

typedef struct AkMaterialSpecularProp {
  AkTextureRef     *specularTex;
  AkColorDesc      *color;
  union {
    float           strength;
    float           shininess;
  };
  AkTextureChannels textureChannels;
} AkMaterialSpecularProp;

typedef struct AkMaterialEmissionProp {
  AkColorDesc color;
  float       strength;
} AkMaterialEmissionProp;

typedef struct AkTechniqueFxCommon {
  AkColorDesc                   *ambient;
  AkMaterialEmissionProp        *emission;
  union {
    AkColorDesc                 *diffuse;
    AkColorDesc                 *albedo;
  };
  AkColorDesc                   *constantDiffuse;
  AkMaterialSpecularProp        *specular;
  AkReflective                  *reflective;
  AkTransparent                 *transparent;
  float                          ior;
  AkMaterialType                 type;
  bool                           doubleSided;
} AkTechniqueFxCommon;

typedef struct AkTechniqueFx {
  AkTechniqueFxCommon  *common;
  AkTree               *extra;
  AkTechniqueFx        *next;
} AkTechniqueFx;

typedef struct AkTechniqueOverride {
  const char *ref;
  const char *pass;
} AkTechniqueOverride;

typedef struct AkTechniqueHint {
  AkTechniqueHint *next;
  const char      *platform;
  const char      *ref;
  const char      *profile;
  AkProfileType    profileType;
} AkTechniqueHint;

typedef struct AkProfile {
  AkNewParam     *newparam;
  AkTechniqueFx  *technique;
  AkTree         *extra;
  AkProfile      *next;
  AkProfileType   type;
} AkProfile;

typedef AkProfile AkProfileCommon;
typedef AkProfile AkProfileGLTF;

typedef struct AkEffect {
  const char         *name;
  AkNewParam         *newparam;
  AkProfile          *profile;
  AkTree             *extra;
  AkEffect           *next;
  AkProfileType       bestProfile;
} AkEffect;

typedef struct AkInstanceEffect {
  AkInstanceBase   base;
  AkTechniqueHint *techniqueHint;
} AkInstanceEffect;

typedef struct AkBindMaterial {
  AkParam            *param;
  AkInstanceMaterial *tcommon;
  AkTechnique        *technique;
  AkTree             *extra;
} AkBindMaterial;

typedef struct AkBind {
  const char *semantic;
  const char *target;
  AkBind     *next;
} AkBind;

typedef struct AkBindVertexInput {
  AkBindVertexInput *next;
  const char        *semantic;
  const char        *inputSemantic;
  AkUInt             inputSet;
} AkBindVertexInput;

typedef struct AkInstanceMaterial {
  AkInstanceBase       base;
  const char          *symbol;
  AkTechniqueOverride *techniqueOverride;
  AkBind              *bind;
  AkBindVertexInput   *bindVertexInput;
} AkInstanceMaterial;

typedef struct AkRender {
  const char         *name;
  const char         *cameraNode;
  AkStringArrayL     *layer;
  AkInstanceMaterial *instanceMaterial;
  AkTree             *extra;
  AkRender           *next;
} AkRender;

typedef struct AkEvaluateScene {
  const char             *name;
  AkRender               *render;
  AkTree                 *extra;
  AkEvaluateScene        *next;
  AkBool                  enable;
} AkEvaluateScene;

AK_HIDE
AkProfile*
ak_profile(AkEffect * __restrict effect,
           AkProfile       * __restrict after);

AK_HIDE
AkProfileType
ak_profileType(AkEffect * __restrict effect);

AK_HIDE
uint32_t
ak_supportedProfiles(AkProfileType **profileTypes);

AK_HIDE
void
ak_setSupportedProfiles(AkProfileType profileTypes[],
                        uint32_t      count);

AK_HIDE
const char*
ak_platform(void);

AK_HIDE
void
ak_setPlatform(const char platform[64]);

AK_HIDE
AkProfileCommon*
ak_getProfileCommon(AkEffect * __restrict effect);

AK_HIDE
AkTechniqueFxCommon*
ak_getProfileTechniqueCommon(AkEffect * __restrict effect);

AK_HIDE
AkEffect*
ak_effectForBindMaterial(AkBindMaterial      * __restrict bindMat,
                         AkMeshPrimitive     * __restrict meshPrim,
                         AkInstanceMaterial ** __restrict foundInstMat);

AK_HIDE
void
ak__instanceGeometryApplyBindMaterial(AkInstanceGeometry * __restrict instance,
                                      AkBindMaterial     * __restrict bindMat);

#endif /* ak_src_material_legacy_h */
