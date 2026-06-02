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

#include "../common.h"
#include "../string_fast.h"
#include "../strpool.h"
#include "internal.h"
#include "../../include/ak/material.h"

static bool
ak__materialInputNameEq(const AkMaterialInput * __restrict input,
                        const char            * __restrict name) {
  return input && input->semantic && strcmp(input->semantic, name) == 0;
}

static const AkMaterialInput*
ak__materialInputByName(AkMaterialSurface * __restrict surface,
                        const char        * __restrict semantic) {
  AkMaterialFeature *feature;
  AkMaterialInput   *input;

#define AK_MATERIAL_MATCH_INPUT(INPUT)                                       \
  do {                                                                       \
    input = INPUT;                                                           \
    if (ak__materialInputNameEq(input, semantic))                            \
      return input;                                                          \
  } while (0)

  AK_MATERIAL_MATCH_INPUT(surface->baseColor);
  AK_MATERIAL_MATCH_INPUT(surface->opacity);
  AK_MATERIAL_MATCH_INPUT(surface->metallic);
  AK_MATERIAL_MATCH_INPUT(surface->roughness);
  AK_MATERIAL_MATCH_INPUT(surface->normal);
  AK_MATERIAL_MATCH_INPUT(surface->occlusion);
  AK_MATERIAL_MATCH_INPUT(surface->emissive);

  feature = surface->features;
  while (feature) {
    switch (feature->type) {
      case AK_MATERIAL_FEATURE_CLEARCOAT: {
        AkMaterialClearcoatFeature *f = (void*)feature;
        AK_MATERIAL_MATCH_INPUT(f->factor);
        AK_MATERIAL_MATCH_INPUT(f->roughness);
        AK_MATERIAL_MATCH_INPUT(f->normal);
        break;
      }
      case AK_MATERIAL_FEATURE_SPECULAR: {
        AkMaterialSpecularFeature *f = (void*)feature;
        AK_MATERIAL_MATCH_INPUT(f->factor);
        AK_MATERIAL_MATCH_INPUT(f->color);
        break;
      }
      case AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS: {
        AkMaterialSpecularGlossinessFeature *f = (void*)feature;
        AK_MATERIAL_MATCH_INPUT(f->diffuse);
        AK_MATERIAL_MATCH_INPUT(f->specular);
        AK_MATERIAL_MATCH_INPUT(f->glossiness);
        break;
      }
      case AK_MATERIAL_FEATURE_TRANSMISSION: {
        AkMaterialTransmissionFeature *f = (void*)feature;
        AK_MATERIAL_MATCH_INPUT(f->factor);
        break;
      }
      case AK_MATERIAL_FEATURE_SHEEN: {
        AkMaterialSheenFeature *f = (void*)feature;
        AK_MATERIAL_MATCH_INPUT(f->color);
        AK_MATERIAL_MATCH_INPUT(f->roughness);
        break;
      }
      case AK_MATERIAL_FEATURE_IRIDESCENCE: {
        AkMaterialIridescenceFeature *f = (void*)feature;
        AK_MATERIAL_MATCH_INPUT(f->factor);
        AK_MATERIAL_MATCH_INPUT(f->thickness);
        break;
      }
      case AK_MATERIAL_FEATURE_VOLUME: {
        AkMaterialVolumeFeature *f = (void*)feature;
        AK_MATERIAL_MATCH_INPUT(f->thickness);
        break;
      }
      case AK_MATERIAL_FEATURE_ANISOTROPY: {
        AkMaterialAnisotropyFeature *f = (void*)feature;
        AK_MATERIAL_MATCH_INPUT(f->strength);
        AK_MATERIAL_MATCH_INPUT(f->rotation);
        break;
      }
      case AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION: {
        AkMaterialDiffuseTransmissionFeature *f = (void*)feature;
        AK_MATERIAL_MATCH_INPUT(f->factor);
        AK_MATERIAL_MATCH_INPUT(f->color);
        break;
      }
      case AK_MATERIAL_FEATURE_SUBSURFACE: {
        AkMaterialSubsurfaceFeature *f = (void*)feature;
        AK_MATERIAL_MATCH_INPUT(f->weight);
        AK_MATERIAL_MATCH_INPUT(f->color);
        AK_MATERIAL_MATCH_INPUT(f->radius);
        break;
      }
      case AK_MATERIAL_FEATURE_CLASSIC: {
        AkMaterialClassicFeature *f = (void*)feature;
        AK_MATERIAL_MATCH_INPUT(f->ambient);
        AK_MATERIAL_MATCH_INPUT(f->diffuse);
        AK_MATERIAL_MATCH_INPUT(f->specular);
        AK_MATERIAL_MATCH_INPUT(f->emission);
        AK_MATERIAL_MATCH_INPUT(f->reflective);
        AK_MATERIAL_MATCH_INPUT(f->transparency);
        break;
      }
      default:
        break;
    }
    feature = feature->next;
  }

#undef AK_MATERIAL_MATCH_INPUT

  return NULL;
}

AK_EXPORT
bool
ak_materialTypeIsPBR(AkMaterialType type) {
  switch (type) {
    case AK_MATERIAL_TYPE_PBR:
    case AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS:
    case AK_MATERIAL_TYPE_PBR_SPECULAR_GLOSSINESS:
      return true;
    default:
      return false;
  }
}

AK_EXPORT
bool
ak_materialTypeIsClassic(AkMaterialType type) {
  switch (type) {
    case AK_MATERIAL_TYPE_PHONG:
    case AK_MATERIAL_TYPE_BLINN:
    case AK_MATERIAL_TYPE_LAMBERT:
      return true;
    default:
      return false;
  }
}

AK_EXPORT
bool
ak_materialTypeIsRenderable(AkMaterialType type) {
  return type != AK_MATERIAL_TYPE_NONE;
}

AK_EXPORT
AkMaterialSemantic
ak_materialSemantic(const char * __restrict name) {
  size_t len;

  if (!name)
    return AK_MATERIAL_SEMANTIC_UNKNOWN;

  len = strlen(name);
  switch (len) {
    case 5:
      if (ak_str_eq_packed_fast(name, len, _s_ak_alpha_u64, _s_ak_alpha_len))
        return AK_MATERIAL_SEMANTIC_OPACITY;
      break;
    case 6:
      if (ak_str_eq_packed_fast(name, len, _s_ak_albedo_u64, _s_ak_albedo_len))
        return AK_MATERIAL_SEMANTIC_BASE_COLOR;
      if (ak_str_eq_packed_fast(name, len, _s_ak_normal_u64, _s_ak_normal_len))
        return AK_MATERIAL_SEMANTIC_NORMAL;
      break;
    case 7:
      if (ak_str_eq_packed_fast(name, len, _s_ak_diffuse_u64, _s_ak_diffuse_len))
        return AK_MATERIAL_SEMANTIC_BASE_COLOR;
      if (ak_str_eq_packed_fast(name, len, _s_ak_opacity_u64, _s_ak_opacity_len))
        return AK_MATERIAL_SEMANTIC_OPACITY;
      break;
    case 8:
      if (ak_str_eq_packed_fast(name, len, _s_ak_metallic_u64, _s_ak_metallic_len))
        return AK_MATERIAL_SEMANTIC_METALLIC;
      if (ak_str_eq_packed_fast(name, len, _s_ak_emissive_u64, _s_ak_emissive_len))
        return AK_MATERIAL_SEMANTIC_EMISSIVE;
      if (ak_str_eq_packed_fast(name, len, _s_ak_emission_u64, _s_ak_emission_len))
        return AK_MATERIAL_SEMANTIC_EMISSIVE;
      break;
    case 9:
      if (ak_str_load8_fast(name) == _s_ak_baseColor_u64_prefix
          && name[8] == _s_ak_baseColor_last)
        return AK_MATERIAL_SEMANTIC_BASE_COLOR;
      if (ak_str_load8_fast(name) == _s_ak_metalness_u64_prefix
          && name[8] == _s_ak_metalness_last)
        return AK_MATERIAL_SEMANTIC_METALLIC;
      if (ak_str_load8_fast(name) == _s_ak_roughness_u64_prefix
          && name[8] == _s_ak_roughness_last)
        return AK_MATERIAL_SEMANTIC_ROUGHNESS;
      if (ak_str_load8_fast(name) == _s_ak_occlusion_u64_prefix
          && name[8] == _s_ak_occlusion_last)
        return AK_MATERIAL_SEMANTIC_OCCLUSION;
      break;
    default:
      break;
  }

  return AK_MATERIAL_SEMANTIC_UNKNOWN;
}

AK_EXPORT
const char*
ak_materialSemanticName(AkMaterialSemantic semantic) {
  switch (semantic) {
    case AK_MATERIAL_SEMANTIC_BASE_COLOR: return _s_ak_baseColor;
    case AK_MATERIAL_SEMANTIC_OPACITY:    return _s_ak_opacity;
    case AK_MATERIAL_SEMANTIC_METALLIC:   return _s_ak_metallic;
    case AK_MATERIAL_SEMANTIC_ROUGHNESS:  return _s_ak_roughness;
    case AK_MATERIAL_SEMANTIC_NORMAL:     return _s_ak_normal;
    case AK_MATERIAL_SEMANTIC_OCCLUSION:  return _s_ak_occlusion;
    case AK_MATERIAL_SEMANTIC_EMISSIVE:   return _s_ak_emissive;
    default:                              return NULL;
  }
}

AK_EXPORT
const AkMaterialInput*
ak_materialInputBySemantic(AkMaterialSurface * __restrict surface,
                           AkMaterialSemantic             semantic) {
  if (!surface)
    return NULL;

  switch (semantic) {
    case AK_MATERIAL_SEMANTIC_BASE_COLOR: return surface->baseColor;
    case AK_MATERIAL_SEMANTIC_OPACITY:    return surface->opacity;
    case AK_MATERIAL_SEMANTIC_METALLIC:   return surface->metallic;
    case AK_MATERIAL_SEMANTIC_ROUGHNESS:  return surface->roughness;
    case AK_MATERIAL_SEMANTIC_NORMAL:     return surface->normal;
    case AK_MATERIAL_SEMANTIC_OCCLUSION:  return surface->occlusion;
    case AK_MATERIAL_SEMANTIC_EMISSIVE:   return surface->emissive;
    default:                              return NULL;
  }
}

AK_EXPORT
const AkMaterialInput*
ak_materialInput(AkMaterialSurface * __restrict surface,
                 const char        * __restrict semantic) {
  AkMaterialSemantic known;

  if (!surface || !semantic)
    return NULL;

  known = ak_materialSemantic(semantic);
  if (known != AK_MATERIAL_SEMANTIC_UNKNOWN)
    return ak_materialInputBySemantic(surface, known);

  return ak__materialInputByName(surface, semantic);
}

AK_EXPORT
bool
ak_materialInputFlag(const AkMaterialInput * __restrict input,
                     AkMaterialInputFlags               flag) {
  return input && (input->flags & flag) == flag;
}

AK_EXPORT
bool
ak_materialInputHasFlag(const AkMaterialInput * __restrict input,
                        AkMaterialInputFlags               flag) {
  return ak_materialInputFlag(input, flag);
}

AK_EXPORT
AkMaterialFeature*
ak_materialFeature(AkMaterialSurface   * __restrict surface,
                   AkMaterialFeatureType            type) {
  AkMaterialFeature *feature;

  if (!surface)
    return NULL;

  feature = surface->features;
  while (feature) {
    if (feature->type == type)
      return feature;
    feature = feature->next;
  }

  return NULL;
}

AK_EXPORT
bool
ak_materialHasFeature(AkMaterialSurface   * __restrict surface,
                      AkMaterialFeatureType            type) {
  return ak_materialFeature(surface, type) != NULL;
}

AK_EXPORT
AkMaterialPropertySet*
ak_materialPropertySetById(AkDoc    * __restrict doc,
                           uint32_t              id) {
  AkMaterialPropertySet *set;

  if (!doc)
    return NULL;

  if (doc->materialProperties.byId
      && (set = ak_map_find(doc->materialProperties.byId,
                            (void *)(uintptr_t)id)))
    return set;

  set = doc->materialProperties.sets;
  while (set) {
    if (set->id == id)
      return set;
    set = set->next;
  }

  return NULL;
}

AK_EXPORT
AkMaterialProperty*
ak_materialProperty(AkMaterialPropertySet * __restrict set,
                    uint32_t                          propertyIndex) {
  if (!set || !set->properties || propertyIndex >= set->count)
    return NULL;

  return set->properties + propertyIndex;
}

AK_EXPORT
AkMaterialProperty*
ak_resolvedMaterialProperty(AkResolvedMaterial * __restrict resolved) {
  if (!resolved || !resolved->binding)
    return NULL;

  return ak_materialProperty(resolved->binding->propertySet,
                             resolved->propertyIndex);
}

static
void
ak__materialResolvedSet(AkResolvedMaterial * __restrict resolved,
                        AkMaterial         * __restrict material,
                        AkMaterialBinding  * __restrict binding,
                        uint32_t                        propertyIndex,
                        uint32_t                        variantIndex) {
  resolved->material      = material;
  resolved->surface       = material ? material->surface : NULL;
  resolved->binding       = binding;
  resolved->propertyIndex = propertyIndex;
  resolved->variantIndex  = variantIndex;
}

AK_EXPORT
bool
ak_materialResolveForPrimitive(AkMeshPrimitive   * __restrict prim,
                               uint32_t                      variantIndex,
                               AkResolvedMaterial * __restrict resolved) {
  AkMaterialBinding        *binding, *fallbackBinding;
  AkMaterialVariantMapping *mapping;
  uint32_t                  noVariant;

  if (!resolved)
    return false;

  memset(resolved, 0, sizeof(*resolved));
  if (!prim)
    return false;

  noVariant       = UINT32_MAX;
  fallbackBinding = NULL;

  binding = prim->materialBindings;
  while (binding) {
    if (binding->scope == AK_MATERIAL_BIND_PRIMITIVE) {
      if (variantIndex != noVariant && binding->variantIndex == variantIndex) {
        ak__materialResolvedSet(resolved,
                                binding->material,
                                binding,
                                binding->propertyIndex,
                                variantIndex);
        return resolved->material || resolved->surface || binding->propertySet;
      }

      if (binding->variantIndex == noVariant && !fallbackBinding)
        fallbackBinding = binding;
    }

    binding = binding->next;
  }

  if (variantIndex != noVariant) {
    mapping = prim->variantMappings;
    while (mapping) {
      if (mapping->variantIndex == variantIndex) {
        ak__materialResolvedSet(resolved,
                                mapping->material,
                                NULL,
                                0,
                                variantIndex);
        return resolved->material != NULL;
      }

      mapping = mapping->next;
    }
  }

  if (fallbackBinding) {
    ak__materialResolvedSet(resolved,
                            fallbackBinding->material,
                            fallbackBinding,
                            fallbackBinding->propertyIndex,
                            noVariant);
    return resolved->material || resolved->surface || fallbackBinding->propertySet;
  }

  ak__materialResolvedSet(resolved, prim->material, NULL, 0, noVariant);
  return resolved->material != NULL;
}

AK_EXPORT
bool
ak_materialResolve(AkMeshPrimitive    * __restrict prim,
                   AkInstanceGeometry * __restrict instance,
                   uint32_t                         variantIndex,
                   AkResolvedMaterial * __restrict resolved) {
  uint32_t noVariant;

  if (!resolved)
    return false;

  noVariant = UINT32_MAX;
  if (ak_materialResolveForPrimitive(prim, variantIndex, resolved)) {
    if (resolved->surface || variantIndex != noVariant)
      return true;
  }

#if defined(AK_INTERNAL_BUILD)
  if (prim && instance && ak__instanceGeometryBindMaterial(instance) && variantIndex == noVariant) {
    AkInstanceMaterial *instMat;
    AkMaterial         *material;

    instMat = NULL;
    if (ak_effectForBindMaterial(ak__instanceGeometryBindMaterial(instance), prim, &instMat)
        && instMat
        && (material = ak_instanceObject(&instMat->base))
        && material->surface) {
      ak__materialResolvedSet(resolved, material, NULL, 0, noVariant);
      return true;
    }
  }
#else
  (void)instance;
#endif

  return resolved->material != NULL;
}

static
int32_t
ak__materialTextureSlot(AkTextureRef * __restrict texref) {
  int32_t slot;

  if (!texref)
    return 0;

  slot = texref->slot;
  if (texref->transform && texref->transform->slot > -1)
    slot = texref->transform->slot;

  return slot >= 0 ? slot : 0;
}

AK_EXPORT
int32_t
ak_materialTextureSlot(AkMeshPrimitive    * __restrict prim,
                       AkInstanceGeometry * __restrict instance,
                       AkTextureRef       * __restrict texref) {
  int32_t slot;

  slot = ak__materialTextureSlot(texref);

#if defined(AK_INTERNAL_BUILD)
  if (prim && instance && ak__instanceGeometryBindMaterial(instance) && texref && texref->texcoord) {
    AkInstanceMaterial *instMat;
    AkBindVertexInput  *bvi;

    instMat = NULL;
    (void)ak_effectForBindMaterial(ak__instanceGeometryBindMaterial(instance), prim, &instMat);
    if (instMat) {
      for (bvi = instMat->bindVertexInput; bvi; bvi = bvi->next) {
        if (bvi->semantic && strcmp(bvi->semantic, texref->texcoord) == 0) {
          slot = (int32_t)bvi->inputSet;
          break;
        }
      }
    }
  }
#else
  (void)prim;
  (void)instance;
#endif

  return slot >= 0 ? slot : 0;
}

AK_HIDE
AkMaterialSourceRecord*
ak_materialLegacyEffectRecord(AkMaterial * __restrict material) {
  AkMaterialSourceRecord *record;

  if (!material)
    return NULL;

  record = material->sourceRecords;
  while (record) {
    if (record->type == AK_MATERIAL_SOURCE_RECORD_LEGACY_EFFECT)
      return record;
    record = record->next;
  }

  return NULL;
}

AK_HIDE
AkInstanceEffect*
ak_materialInstanceEffect(AkMaterial * __restrict material) {
  AkMaterialSourceRecord *record;

  record = ak_materialLegacyEffectRecord(material);
  if (!record)
    return NULL;

  return record->payload;
}

AK_HIDE
AkEffect*
ak_materialEffect(AkMaterial * __restrict material) {
  AkInstanceEffect *instEffect;

  instEffect = ak_materialInstanceEffect(material);
  if (!instEffect)
    return NULL;

  return ak_instanceObject(&instEffect->base);
}

AK_HIDE
void
ak_materialSetInstanceEffect(AkHeap           * __restrict heap,
                             void             * __restrict parent,
                             AkMaterial       * __restrict material,
                             AkInstanceEffect * __restrict instEffect) {
  AkMaterialSourceRecord *record;
  AkInstanceEffect       *head;

  if (!heap || !material || !instEffect)
    return;

  record = ak_materialLegacyEffectRecord(material);
  if (!record) {
    record          = ak_heap_calloc(heap,
                                     parent ? parent : material,
                                     sizeof(*record));
    record->next    = material->sourceRecords;
    record->material = material;
    record->type    = AK_MATERIAL_SOURCE_RECORD_LEGACY_EFFECT;
    material->sourceRecords = record;
  }

  head = record->payload;
  if (head) {
    head->base.prev        = &instEffect->base;
    instEffect->base.next  = &head->base;
  }

  record->payload = instEffect;
}

static
void
ak__materialFeaturePush(AkMaterialSurface * __restrict surface,
                        AkMaterialFeature * __restrict feature) {
  if (!surface || !feature)
    return;

  feature->next      = surface->features;
  surface->features  = feature;

  if ((uint32_t)feature->type < 32)
    surface->featureMask |= 1u << (uint32_t)feature->type;
}

static
AkMaterialInput*
ak__materialInputAlloc(AkHeap      * __restrict heap,
                       void        * __restrict parent,
                       const char  * __restrict semantic) {
  AkMaterialInput *input;

  input           = ak_heap_calloc(heap, parent, sizeof(*input));
  input->semantic = semantic;
  input->source   = AK_MATERIAL_INPUT_CONSTANT;

  return input;
}

static
AkMaterialInput*
ak__materialInputFromColorDesc(AkHeap       * __restrict heap,
                               void         * __restrict parent,
                               const char   * __restrict semantic,
                               AkColorDesc  * __restrict desc,
                               AkTextureColorSpace       colorSpace,
                               AkTextureChannels         channels) {
  AkMaterialInput *input;

  if (!desc || (!desc->color && !desc->texture && !desc->param))
    return NULL;

  input             = ak__materialInputAlloc(heap, parent, semantic);
  input->sourceName = desc->param ? desc->param->ref : NULL;
  input->texture    = desc->texture;
  input->channels   = channels;
  input->colorSpace = colorSpace;
  input->valueType  = AK_MATERIAL_VALUE_COLOR;

  if (desc->color) {
    input->color = *desc->color;
  } else {
    input->color.rgba.R = 1.0f;
    input->color.rgba.G = 1.0f;
    input->color.rgba.B = 1.0f;
    input->color.rgba.A = 1.0f;
  }

  if (desc->texture)
    input->source = AK_MATERIAL_INPUT_TEXTURE;
  else if (desc->param)
    input->source = AK_MATERIAL_INPUT_PARAM;

  return input;
}

static
AkMaterialInput*
ak__materialInputFromScalarTexture(AkHeap             * __restrict heap,
                                   void               * __restrict parent,
                                   const char         * __restrict semantic,
                                   float                           value,
                                   AkTextureRef       * __restrict texture,
                                   AkTextureColorSpace             colorSpace,
                                   AkTextureChannels               channels) {
  AkMaterialInput *input;

  input             = ak__materialInputAlloc(heap, parent, semantic);
  input->texture    = texture;
  input->channels   = channels;
  input->colorSpace = colorSpace;
  input->valueType  = AK_MATERIAL_VALUE_FLOAT;
  input->value[0]   = value;

  if (texture)
    input->source = AK_MATERIAL_INPUT_TEXTURE;

  return input;
}

static
bool
ak__materialTransparentUsesRGB(AkOpaque opaque) {
  return opaque == AK_OPAQUE_RGB_ONE || opaque == AK_OPAQUE_RGB_ZERO;
}

static
bool
ak__materialTransparentInverts(AkOpaque opaque) {
  return opaque == AK_OPAQUE_A_ZERO || opaque == AK_OPAQUE_RGB_ZERO;
}

static
AkTextureChannels
ak__materialTransparentChannels(AkOpaque opaque) {
  if (ak__materialTransparentUsesRGB(opaque))
    return AK_TEXTURE_CHANNEL_RGB;

  return AK_TEXTURE_CHANNEL_A;
}

static
float
ak__materialTransparentColorFactor(AkTransparent * __restrict transparent) {
  AkColor *color;

  if (!transparent
      || !transparent->color
      || !(color = transparent->color->color))
    return 1.0f;

  if (ak__materialTransparentUsesRGB(transparent->opaque)) {
    return glm_clamp_zo(glm_luminance(color->vec));
  }

  return glm_clamp_zo(color->rgba.A);
}

static
float
ak__materialTransparentOpacity(AkTransparent * __restrict transparent,
                               bool                       hasTexture) {
  float opacity;

  if (!transparent)
    return 1.0f;

  opacity = glm_clamp_zo(transparent->amount)
            * ak__materialTransparentColorFactor(transparent);

  if (!hasTexture && ak__materialTransparentInverts(transparent->opaque))
    opacity = 1.0f - opacity;

  return glm_clamp_zo(opacity);
}

static
AkMaterialInput*
ak__materialInputFromSpecularProp(AkHeap                 * __restrict heap,
                                  void                   * __restrict parent,
                                  const char             * __restrict semantic,
                                  AkMaterialSpecularProp * __restrict prop) {
  AkMaterialInput *input;

  if (!prop)
    return NULL;

  input = ak__materialInputFromColorDesc(heap,
                                         parent,
                                         semantic,
                                         prop->color,
                                         AK_TEXTURE_COLORSPACE_SRGB,
                                         prop->textureChannels);
  if (input)
    input->texture = input->texture ? input->texture : prop->specularTex;
  else if (prop->specularTex)
    input = ak__materialInputFromScalarTexture(heap,
                                               parent,
                                               semantic,
                                               prop->strength,
                                               prop->specularTex,
                                               AK_TEXTURE_COLORSPACE_SRGB,
                                               prop->textureChannels);

  return input;
}

AK_HIDE
AkMaterial*
ak_materialDefaultVertexColorAlpha(AkDoc * __restrict doc, bool alphaBlend) {
  AkHeap            *heap;
  AkMaterial        *material;
  AkMaterialSurface *surface;
  AkMaterialInput   *input;

  if (!doc)
    return NULL;

  for (material = doc->lib.materials.first; material; material = material->next) {
    surface = material->surface;
    if (surface
        && surface->baseColor
        && surface->baseColor->source == AK_MATERIAL_INPUT_VERTEX_COLOR
        && ((surface->flags & AK_MATERIAL_FLAG_ALPHA_BLEND) != 0) == alphaBlend)
      return material;
  }

  heap = ak_heap_getheap(doc);

  material = ak_heap_calloc(heap, doc, sizeof(*material));
  surface  = ak_heap_calloc(heap, material, sizeof(*surface));
  input    = ak__materialInputAlloc(heap, surface, _s_ak_baseColor);

  input->source       = AK_MATERIAL_INPUT_VERTEX_COLOR;
  input->valueType    = AK_MATERIAL_VALUE_COLOR;
  input->channels     = AK_TEXTURE_CHANNEL_RGBA;
  input->colorSpace   = AK_TEXTURE_COLORSPACE_SRGB;
  input->color.rgba.R = 1.0f;
  input->color.rgba.G = 1.0f;
  input->color.rgba.B = 1.0f;
  input->color.rgba.A = 1.0f;

  surface->baseColor        = input;
  surface->type             = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->alphaCutoff      = 0.5f;
  surface->ior              = 1.5f;
  surface->emissiveStrength = 1.0f;

  if (alphaBlend)
    surface->flags |= AK_MATERIAL_FLAG_ALPHA_BLEND;

  surface->metallic = ak__materialInputFromScalarTexture(heap,
                                                         surface,
                                                         _s_ak_metallic,
                                                         0.0f,
                                                         NULL,
                                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                                         AK_TEXTURE_CHANNEL_NONE);
  surface->roughness = ak__materialInputFromScalarTexture(heap,
                                                          surface,
                                                          _s_ak_roughness,
                                                          1.0f,
                                                          NULL,
                                                          AK_TEXTURE_COLORSPACE_LINEAR,
                                                          AK_TEXTURE_CHANNEL_NONE);

  material->name      = alphaBlend ? _s_ak_materialVertexColorAlpha : _s_ak_materialVertexColor;
  material->surface   = surface;
  AK_LIB_PREPEND(doc->lib.materials, material, next);

  return material;
}

AK_HIDE
AkMaterial*
ak_materialDefaultVertexColor(AkDoc * __restrict doc) {
  return ak_materialDefaultVertexColorAlpha(doc, false);
}

AK_HIDE
AkMaterialSurface*
ak_materialSurfaceFromTechniqueCommon(AkHeap              * __restrict heap,
                                      void                * __restrict parent,
                                      AkTechniqueFxCommon * __restrict common) {
  AkMaterialSurface       *surface;
  AkMaterialClassicFeature *classic;

  if (!heap || !parent || !common)
    return NULL;

  surface              = ak_heap_calloc(heap, parent, sizeof(*surface));
  surface->type        = common->type;
  surface->alphaCutoff = 0.5f;
  surface->ior         = common->ior;
  surface->emissiveStrength = 1.0f;

  if (common->doubleSided)
    surface->flags |= AK_MATERIAL_FLAG_DOUBLE_SIDED;

  if (common->type == AK_MATERIAL_TYPE_UNLIT)
    surface->flags |= AK_MATERIAL_FLAG_UNLIT;

  surface->baseColor = ak__materialInputFromColorDesc(heap,
                                                      surface,
                                                      _s_ak_baseColor,
                                                      common->albedo ? common->albedo : common->constantDiffuse,
                                                      AK_TEXTURE_COLORSPACE_SRGB,
                                                      AK_TEXTURE_CHANNEL_RGBA);
  if (!surface->baseColor
      && common->type == AK_MATERIAL_TYPE_CONSTANT
      && common->transparent
      && common->transparent->color) {
    surface->baseColor = ak__materialInputFromColorDesc(heap,
                                                        surface,
                                                        _s_ak_baseColor,
                                                        common->transparent->color,
                                                        AK_TEXTURE_COLORSPACE_SRGB,
                                                        AK_TEXTURE_CHANNEL_RGBA);
  }

  if (common->emission)
    surface->emissive = ak__materialInputFromColorDesc(heap,
                                                       surface,
                                                       _s_ak_emissive,
                                                       &common->emission->color,
                                                       AK_TEXTURE_COLORSPACE_SRGB,
                                                       AK_TEXTURE_CHANNEL_RGB);

  if (common->emission)
    surface->emissiveStrength = common->emission->strength;

  if (common->transparent) {
    AkTransparent     *transparent;
    AkTextureRef      *opacityTex;
    AkTextureChannels  opacityChannels;
    float              opacity;

    transparent      = common->transparent;
    opacityTex       = transparent->color ? transparent->color->texture : NULL;
    opacityChannels  = ak__materialTransparentChannels(transparent->opaque);
    opacity          = ak__materialTransparentOpacity(transparent, opacityTex != NULL);

    surface->opacity = ak__materialInputFromScalarTexture(heap,
                                                          surface,
                                                          _s_ak_opacity,
                                                          opacity,
                                                          opacityTex,
                                                          AK_TEXTURE_COLORSPACE_LINEAR,
                                                          opacityChannels);
    surface->alphaCutoff = common->transparent->cutoff;

    if (opacityTex && ak__materialTransparentInverts(transparent->opaque))
      surface->opacity->flags |= AK_MATERIAL_INPUT_FLAG_INVERTED;

    if (common->transparent->opaque == AK_OPAQUE_MASK)
      surface->flags |= AK_MATERIAL_FLAG_ALPHA_MASK;
    else if (common->transparent->opaque == AK_OPAQUE_BLEND
             || opacityTex
             || opacity < 0.999f)
      surface->flags |= AK_MATERIAL_FLAG_ALPHA_BLEND;
  }

  if (ak_materialTypeIsClassic(common->type)
      || common->ambient
      || common->specular
      || common->reflective) {
    classic            = ak_heap_calloc(heap, surface, sizeof(*classic));
    classic->base.type = AK_MATERIAL_FEATURE_CLASSIC;
    classic->ambient   = ak__materialInputFromColorDesc(heap,
                                                      classic,
                                                      _s_ak_ambient,
                                                      common->ambient,
                                                      AK_TEXTURE_COLORSPACE_SRGB,
                                                      AK_TEXTURE_CHANNEL_RGB);
    classic->diffuse  = surface->baseColor;
    classic->specular = ak__materialInputFromSpecularProp(heap,
                                                          classic,
                                                          _s_ak_specular,
                                                          common->specular);
    classic->emission = surface->emissive;
    classic->reflective = common->reflective
      ? ak__materialInputFromColorDesc(heap,
                                       classic,
                                       _s_ak_reflective,
                                       common->reflective->color,
                                       AK_TEXTURE_COLORSPACE_SRGB,
                                       AK_TEXTURE_CHANNEL_RGB)
      : NULL;

    classic->transparency = common->transparent
      ? ak__materialInputFromColorDesc(heap,
                                       classic,
                                       _s_ak_transparency,
                                       common->transparent->color,
                                       AK_TEXTURE_COLORSPACE_SRGB,
                                       ak__materialTransparentChannels(common->transparent->opaque))
      : NULL;
    if (classic->transparency
        && classic->transparency->texture
        && ak__materialTransparentInverts(common->transparent->opaque)) {
      classic->transparency->flags |= AK_MATERIAL_INPUT_FLAG_INVERTED;
    }

    classic->shininess    = common->specular ? common->specular->shininess : 0.0f;
    classic->reflectivity = common->reflective ? common->reflective->amount : 0.0f;
    classic->ior          = common->ior;

    ak__materialFeaturePush(surface, &classic->base);
  }

  return surface;
}

AK_EXPORT
AkMaterialVariant*
ak_materialVariantByName(AkDoc       * __restrict doc,
                         const char  * __restrict name) {
  AkMaterialVariant *it;

  if (!doc || !name)
    return NULL;

  it = doc->materialVariants;
  while (it) {
    if (it->name && strcmp(it->name, name) == 0)
      return it;
    it = it->next;
  }

  return NULL;
}

AK_HIDE
AkEffect*
ak_effectForBindMaterial(AkBindMaterial      * __restrict bindMat,
                         AkMeshPrimitive     * __restrict meshPrim,
                         AkInstanceMaterial ** __restrict foundInstMat) {
  AkMaterial         *material;
  AkInstanceMaterial *materialInst;
  AkGeometry         *geom;
  AkMap              *materialMap;
  AkMapItem          *mi;
  AkEffect           *effect;

  if (!meshPrim || !meshPrim->mesh || !meshPrim->mesh->geom)
    return NULL;

  geom         = meshPrim->mesh->geom;
  materialInst = bindMat->tcommon;
  materialMap  = geom->materialMap;

  while (materialInst) {
    /* there is symbol, bind only to specified primitive */
    if (materialInst->symbol) {
      if (!materialMap)
        return NULL;

      mi = ak_map_find(materialMap, (void *)materialInst->symbol);
      while (mi) {
        if ((AkMeshPrimitive *)mi->data == meshPrim) {
          material = ak_instanceObject(&materialInst->base);
          if (material && (effect = ak_materialEffect(material))) {
            *foundInstMat = materialInst;
            return effect;
          } else {
            return NULL;
          }
        }
        mi = mi->next;
      }
    }

    /* bind to whole geometry, TODO: is this OK ? */
    else {
      material = ak_instanceObject(&materialInst->base);
      if (material && (effect = ak_materialEffect(material))) {
        *foundInstMat = materialInst;
        return effect;
      } else {
        return NULL;
      }
    }

    materialInst = (AkInstanceMaterial *)materialInst->base.next;
  }

  return NULL;
}
