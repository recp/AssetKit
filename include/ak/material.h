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

struct AkDoc;
struct AkInstanceGeometry;
struct AkMap;
struct AkMeshPrimitive;

/*!
 * @brief Named material variant from KHR_materials_variants.
 */
typedef struct AkMaterialVariant {
  struct AkMaterialVariant *next;
  const char               *name;
  struct AkTreeNode        *extras;
} AkMaterialVariant;

/*!
 * @brief Primitive material override for a document variant index.
 */
typedef struct AkMaterialVariantMapping {
  struct AkMaterialVariantMapping *next;
  struct AkMaterial               *material;
  uint32_t                         variantIndex;
} AkMaterialVariantMapping;

typedef enum AkMaterialType {
  AK_MATERIAL_TYPE_NONE                    = 0,
  AK_MATERIAL_TYPE_PHONG                   = 1,
  AK_MATERIAL_TYPE_BLINN                   = 2,
  AK_MATERIAL_TYPE_LAMBERT                 = 3,
  AK_MATERIAL_TYPE_CONSTANT                = 4,
  AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS  = 5,
  AK_MATERIAL_TYPE_PBR_SPECULAR_GLOSSINESS = 6,
  AK_MATERIAL_TYPE_PBR                     = 7,
  AK_MATERIAL_TYPE_UNLIT                   = AK_MATERIAL_TYPE_CONSTANT,
  AK_MATERIAL_TYPE_SHADER_NETWORK          = 8
} AkMaterialType;

typedef enum AkMaterialInputSource {
  AK_MATERIAL_INPUT_NONE = 0,
  AK_MATERIAL_INPUT_CONSTANT,
  AK_MATERIAL_INPUT_TEXTURE,
  AK_MATERIAL_INPUT_VERTEX_COLOR,
  AK_MATERIAL_INPUT_FACE_COLOR,
  AK_MATERIAL_INPUT_PROPERTY_INDEX,
  AK_MATERIAL_INPUT_PARAM,
  AK_MATERIAL_INPUT_SHADER_OUTPUT
} AkMaterialInputSource;

typedef enum AkMaterialInputValue {
  AK_MATERIAL_VALUE_NONE = 0,
  AK_MATERIAL_VALUE_FLOAT,
  AK_MATERIAL_VALUE_FLOAT2,
  AK_MATERIAL_VALUE_FLOAT3,
  AK_MATERIAL_VALUE_FLOAT4,
  AK_MATERIAL_VALUE_COLOR,
  AK_MATERIAL_VALUE_TEXTURE,
  AK_MATERIAL_VALUE_INDEX
} AkMaterialInputValue;

typedef enum AkMaterialSemantic {
  AK_MATERIAL_SEMANTIC_UNKNOWN = 0,
  AK_MATERIAL_SEMANTIC_BASE_COLOR,
  AK_MATERIAL_SEMANTIC_OPACITY,
  AK_MATERIAL_SEMANTIC_METALLIC,
  AK_MATERIAL_SEMANTIC_ROUGHNESS,
  AK_MATERIAL_SEMANTIC_NORMAL,
  AK_MATERIAL_SEMANTIC_OCCLUSION,
  AK_MATERIAL_SEMANTIC_EMISSIVE
} AkMaterialSemantic;

typedef enum AkMaterialInputFlags {
  AK_MATERIAL_INPUT_FLAG_NONE       = 0,
  AK_MATERIAL_INPUT_FLAG_NORMALIZED = 1u << 0,
  AK_MATERIAL_INPUT_FLAG_INVERTED   = 1u << 1
} AkMaterialInputFlags;

typedef struct AkMaterialInput {
  AkTextureRef             *texture;
  const char               *semantic;
  const char               *sourceName;
  union {
    AkColor                 color;
    AkFloat4                value;
  };
  uint32_t                  index;
  AkMaterialInputFlags      flags;
  AkTextureChannels         channels;
  AkTextureColorSpace       colorSpace;
  AkMaterialInputSource     source;
  AkMaterialInputValue      valueType;
} AkMaterialInput;

typedef enum AkMaterialFlags {
  AK_MATERIAL_FLAG_DOUBLE_SIDED = 1u << 0,
  AK_MATERIAL_FLAG_UNLIT        = 1u << 1,
  AK_MATERIAL_FLAG_ALPHA_BLEND  = 1u << 2,
  AK_MATERIAL_FLAG_ALPHA_MASK   = 1u << 3
} AkMaterialFlags;

typedef struct AkMaterialSurface {
  AkMaterialInput          *baseColor;
  AkMaterialInput          *opacity;
  AkMaterialInput          *metallic;
  AkMaterialInput          *roughness;
  AkMaterialInput          *normal;
  AkMaterialInput          *occlusion;
  AkMaterialInput          *emissive;
  struct AkMaterialFeature *features;
  AkTree                   *extras;
  AkMaterialType            type;
  uint32_t                  flags;
  uint32_t                  featureMask;
  float                     alphaCutoff;
  float                     ior;
  float                     emissiveStrength;
} AkMaterialSurface;

typedef enum AkMaterialFeatureType {
  AK_MATERIAL_FEATURE_CLEARCOAT = 1,
  AK_MATERIAL_FEATURE_SPECULAR,
  AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS,
  AK_MATERIAL_FEATURE_TRANSMISSION,
  AK_MATERIAL_FEATURE_SHEEN,
  AK_MATERIAL_FEATURE_IRIDESCENCE,
  AK_MATERIAL_FEATURE_VOLUME,
  AK_MATERIAL_FEATURE_ANISOTROPY,
  AK_MATERIAL_FEATURE_DISPERSION,
  AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION,
  AK_MATERIAL_FEATURE_SUBSURFACE,
  AK_MATERIAL_FEATURE_CLASSIC,
  AK_MATERIAL_FEATURE_SHADER_NETWORK,
  AK_MATERIAL_FEATURE_FORMAT_NATIVE
} AkMaterialFeatureType;

typedef struct AkMaterialFeature {
  struct AkMaterialFeature *next;
  AkMaterialFeatureType     type;
  uint32_t                  flags;
} AkMaterialFeature;

typedef struct AkMaterialClearcoatFeature {
  AkMaterialFeature base;
  AkMaterialInput  *factor;
  AkMaterialInput  *roughness;
  AkMaterialInput  *normal;
  float             normalScale;
} AkMaterialClearcoatFeature;

typedef struct AkMaterialSpecularFeature {
  AkMaterialFeature base;
  AkMaterialInput  *factor;
  AkMaterialInput  *color;
} AkMaterialSpecularFeature;

typedef struct AkMaterialSpecularGlossinessFeature {
  AkMaterialFeature base;
  AkMaterialInput  *diffuse;
  AkMaterialInput  *specular;
  AkMaterialInput  *glossiness;
} AkMaterialSpecularGlossinessFeature;

typedef struct AkMaterialTransmissionFeature {
  AkMaterialFeature base;
  AkMaterialInput  *factor;
} AkMaterialTransmissionFeature;

typedef struct AkMaterialSheenFeature {
  AkMaterialFeature base;
  AkMaterialInput  *color;
  AkMaterialInput  *roughness;
} AkMaterialSheenFeature;

typedef struct AkMaterialIridescenceFeature {
  AkMaterialFeature base;
  AkMaterialInput  *factor;
  AkMaterialInput  *thickness;
  float             ior;
  float             thicknessMinimum;
  float             thicknessMaximum;
} AkMaterialIridescenceFeature;

typedef struct AkMaterialVolumeFeature {
  AkMaterialFeature base;
  AkMaterialInput  *thickness;
  AkColor           attenuationColor;
  float             attenuationDistance;
} AkMaterialVolumeFeature;

typedef struct AkMaterialAnisotropyFeature {
  AkMaterialFeature base;
  AkMaterialInput  *strength;
  AkMaterialInput  *rotation;
} AkMaterialAnisotropyFeature;

typedef struct AkMaterialDispersionFeature {
  AkMaterialFeature base;
  float             dispersion;
} AkMaterialDispersionFeature;

typedef struct AkMaterialDiffuseTransmissionFeature {
  AkMaterialFeature base;
  AkMaterialInput  *factor;
  AkMaterialInput  *color;
} AkMaterialDiffuseTransmissionFeature;

typedef struct AkMaterialSubsurfaceFeature {
  AkMaterialFeature base;
  AkMaterialInput  *weight;
  AkMaterialInput  *color;
  AkMaterialInput  *radius;
  float             anisotropy;
} AkMaterialSubsurfaceFeature;

typedef struct AkMaterialClassicFeature {
  AkMaterialFeature base;
  AkMaterialInput  *ambient;
  AkMaterialInput  *diffuse;
  AkMaterialInput  *specular;
  AkMaterialInput  *emission;
  AkMaterialInput  *reflective;
  AkMaterialInput  *transparency;
  float             shininess;
  float             reflectivity;
  float             ior;
  uint32_t          illum;
} AkMaterialClassicFeature;

typedef enum AkMaterialSourceRecordType {
  AK_MATERIAL_SOURCE_RECORD_NATIVE        = 0,
  AK_MATERIAL_SOURCE_RECORD_LEGACY_EFFECT = 1
} AkMaterialSourceRecordType;

struct AkMaterial;
typedef struct AkMaterialSourceRecord {
  struct AkMaterialSourceRecord *next;
  struct AkMaterial             *material;
  AkTree                        *extra;
  void                          *payload;
  AkMaterialSourceRecordType     type;
} AkMaterialSourceRecord;

typedef struct AkMaterial {
  struct AkMaterial      *next;
  const char             *name;
  AkMaterialSurface      *surface;
  AkMaterialSourceRecord *sourceRecords;
  AkTree                 *extra;
  uint32_t                flags;
} AkMaterial;

typedef enum AkMaterialBindingScope {
  AK_MATERIAL_BIND_OBJECT = 1,
  AK_MATERIAL_BIND_PRIMITIVE,
  AK_MATERIAL_BIND_FACE,
  AK_MATERIAL_BIND_VERTEX,
  AK_MATERIAL_BIND_PROPERTY
} AkMaterialBindingScope;

typedef struct AkMaterialBinding {
  struct AkMaterialBinding     *next;
  AkMaterial                   *material;
  struct AkMaterialPropertySet *propertySet;
  uint32_t                      propertyIndex;
  uint32_t                      variantIndex;
  uint32_t                      first;
  uint32_t                      count;
  AkMaterialBindingScope        scope;
} AkMaterialBinding;

typedef enum AkMaterialPropertySetType {
  AK_MATERIAL_PROPERTY_BASE = 1,
  AK_MATERIAL_PROPERTY_COLOR,
  AK_MATERIAL_PROPERTY_COMPOSITE,
  AK_MATERIAL_PROPERTY_MULTI,
  AK_MATERIAL_PROPERTY_DISPLAY,
  AK_MATERIAL_PROPERTY_PHYSICAL,
  AK_MATERIAL_PROPERTY_FORMAT_NATIVE
} AkMaterialPropertySetType;

typedef struct AkMaterialProperty {
  const char       *name;
  AkMaterialInput  *baseColor;
  AkMaterialInput  *metallic;
  AkMaterialInput  *roughness;
  AkColor           displayColor;
  uint32_t          materialIndex;
  uint32_t          flags;
} AkMaterialProperty;

typedef struct AkMaterialPropertySet {
  struct AkMaterialPropertySet *next;
  const char                   *name;
  AkMaterialProperty           *properties;
  AkTree                       *extra;
  uint32_t                      id;
  uint32_t                      count;
  AkMaterialPropertySetType     type;
} AkMaterialPropertySet;

typedef struct AkMaterialPropertyRegistry {
  AkMaterialPropertySet *sets;
  struct AkMap          *byId;
  uint32_t               count;
} AkMaterialPropertyRegistry;

typedef struct AkResolvedMaterial {
  AkMaterial        *material;
  AkMaterialSurface *surface;
  AkMaterialBinding *binding;
  uint32_t           propertyIndex;
  uint32_t           variantIndex;
} AkResolvedMaterial;

AK_INLINE
AkTextureRef*
ak_materialInputTexture(const AkMaterialInput * __restrict input) {
  return input ? input->texture : NULL;
}

AK_INLINE
float
ak_materialInputScalar(const AkMaterialInput * __restrict input,
                       float                              fallback) {
  if (!input)
    return fallback;

  switch (input->valueType) {
    case AK_MATERIAL_VALUE_FLOAT:
    case AK_MATERIAL_VALUE_FLOAT2:
    case AK_MATERIAL_VALUE_FLOAT3:
    case AK_MATERIAL_VALUE_FLOAT4: return input->value[0];
    default:                       return fallback;
  }
}

AK_INLINE
AkTextureChannels
ak_materialInputChannels(const AkMaterialInput * __restrict input) {
  return input ? input->channels : AK_TEXTURE_CHANNEL_NONE;
}

AK_INLINE
float
ak_materialNormalScale(const AkMaterialSurface * __restrict surface) {
  return ak_materialInputScalar(surface ? surface->normal : NULL, 1.0f);
}

AK_INLINE
float
ak_materialOcclusionStrength(const AkMaterialSurface * __restrict surface) {
  return ak_materialInputScalar(surface ? surface->occlusion : NULL, 1.0f);
}

AK_INLINE
float
ak_materialOpacityFactor(const AkMaterialSurface * __restrict surface) {
  return ak_materialInputScalar(surface ? surface->opacity : NULL, 1.0f);
}

AK_INLINE
float
ak_materialMetallicFactor(const AkMaterialSurface * __restrict surface) {
  return ak_materialInputScalar(surface ? surface->metallic : NULL, 0.0f);
}

AK_INLINE
float
ak_materialRoughnessFactor(const AkMaterialSurface * __restrict surface) {
  return ak_materialInputScalar(surface ? surface->roughness : NULL, 1.0f);
}

AK_INLINE
float
ak_materialAlphaCutoff(const AkMaterialSurface * __restrict surface) {
  return surface ? surface->alphaCutoff : 0.5f;
}

AK_INLINE
float
ak_materialIor(const AkMaterialSurface * __restrict surface) {
  return surface ? surface->ior : 1.5f;
}

AK_INLINE
float
ak_materialEmissiveStrength(const AkMaterialSurface * __restrict surface) {
  return surface ? surface->emissiveStrength : 1.0f;
}

AK_INLINE
bool
ak_materialDoubleSided(const AkMaterialSurface * __restrict surface) {
  return surface && (surface->flags & AK_MATERIAL_FLAG_DOUBLE_SIDED);
}

AK_INLINE
bool
ak_materialUnlit(const AkMaterialSurface * __restrict surface) {
  return surface && (surface->flags & AK_MATERIAL_FLAG_UNLIT);
}

AK_INLINE
bool
ak_materialAlphaBlend(const AkMaterialSurface * __restrict surface) {
  return surface && (surface->flags & AK_MATERIAL_FLAG_ALPHA_BLEND);
}

AK_INLINE
bool
ak_materialAlphaMask(const AkMaterialSurface * __restrict surface) {
  return surface && (surface->flags & AK_MATERIAL_FLAG_ALPHA_MASK);
}

AK_EXPORT
AkMaterialVariant*
ak_materialVariantByName(struct AkDoc * __restrict doc,
                         const char   * __restrict name);

AK_EXPORT
bool
ak_materialTypeIsPBR(AkMaterialType type);

AK_EXPORT
bool
ak_materialTypeIsClassic(AkMaterialType type);

AK_EXPORT
bool
ak_materialTypeIsRenderable(AkMaterialType type);

AK_EXPORT
AkMaterialSemantic
ak_materialSemantic(const char * __restrict name);

AK_EXPORT
const char*
ak_materialSemanticName(AkMaterialSemantic semantic);

AK_EXPORT
const AkMaterialInput*
ak_materialInputBySemantic(AkMaterialSurface * __restrict surface,
                           AkMaterialSemantic             semantic);

AK_EXPORT
const AkMaterialInput*
ak_materialInput(AkMaterialSurface * __restrict surface,
                 const char        * __restrict semantic);

AK_EXPORT
bool
ak_materialInputFlag(const AkMaterialInput * __restrict input,
                     AkMaterialInputFlags               flag);

AK_EXPORT
bool
ak_materialInputHasFlag(const AkMaterialInput * __restrict input,
                        AkMaterialInputFlags               flag);

AK_EXPORT
AkMaterialFeature*
ak_materialFeature(AkMaterialSurface   * __restrict surface,
                   AkMaterialFeatureType            type);

AK_EXPORT
bool
ak_materialHasFeature(AkMaterialSurface   * __restrict surface,
                      AkMaterialFeatureType            type);

AK_EXPORT
AkMaterialPropertySet*
ak_materialPropertySetById(struct AkDoc * __restrict doc,
                           uint32_t                 id);

AK_EXPORT
AkMaterialProperty*
ak_materialProperty(AkMaterialPropertySet * __restrict set,
                    uint32_t                           propertyIndex);

AK_EXPORT
AkMaterialProperty*
ak_resolvedMaterialProperty(AkResolvedMaterial * __restrict resolved);

AK_EXPORT
bool
ak_materialResolve(struct AkMeshPrimitive    * __restrict prim,
                   struct AkInstanceGeometry * __restrict instance,
                   uint32_t                               variantIndex,
                   AkResolvedMaterial        * __restrict resolved);

AK_EXPORT
bool
ak_materialResolveForPrimitive(struct AkMeshPrimitive * __restrict prim,
                               uint32_t                            variantIndex,
                               AkResolvedMaterial    * __restrict  resolved);

AK_EXPORT
int32_t
ak_materialTextureSlot(struct AkMeshPrimitive    * __restrict prim,
                       struct AkInstanceGeometry * __restrict instance,
                       AkTextureRef              * __restrict texref);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_material_h */
