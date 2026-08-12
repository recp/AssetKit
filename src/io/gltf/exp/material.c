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

#include "material.h"
#include "extra.h"
#include "image.h"
#include "plan.h"
#include "../strpool.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

static
bool
gltf_float_eq(float a, float b) {
  return fabsf(a - b) < 0.000001f;
}

static
int
gltf_wrap_mode(AkWrapMode wrap) {
  switch (wrap) {
    case AK_WRAP_MODE_CLAMP:       return 33071;
    case AK_WRAP_MODE_MIRROR:
    case AK_WRAP_MODE_MIRROR_ONCE: return 33648;
    case AK_WRAP_MODE_BORDER:      return 33071;
    case AK_WRAP_MODE_WRAP:
    default:                       return 10497;
  }
}

static
int
gltf_min_filter(AkMinFilter filter, AkMipFilter mipFilter) {
  switch (filter) {
    case AK_MINFILTER_LINEAR_MIPMAP_NEAREST:  return 9985;
    case AK_MINFILTER_LINEAR_MIPMAP_LINEAR:   return 9987;
    case AK_MINFILTER_NEAREST_MIPMAP_NEAREST: return 9984;
    case AK_MINFILTER_NEAREST_MIPMAP_LINEAR:  return 9986;
    case AK_MINFILTER_NONE:
    case AK_MINFILTER_NEAREST:
      switch (mipFilter) {
        case AK_MIPFILTER_NEAREST: return 9984;
        case AK_MIPFILTER_LINEAR:  return 9986;
        case AK_MIPFILTER_NONE:
        case AK_MIPFILTER_UNSPECIFIED:
        default:                   return 9728;
      }
    case AK_MINFILTER_LINEAR:
      switch (mipFilter) {
        case AK_MIPFILTER_NEAREST: return 9985;
        case AK_MIPFILTER_LINEAR:  return 9987;
        case AK_MIPFILTER_NONE:
        case AK_MIPFILTER_UNSPECIFIED:
        default:                   return 9729;
      }
    case AK_MINFILTER_UNSPECIFIED:
    case AK_MINFILTER_ANISOTROPIC:
    default:                        return 0;
  }
}

static
int
gltf_mag_filter(AkMagFilter filter) {
  switch (filter) {
    case AK_MAGFILTER_NONE:
    case AK_MAGFILTER_NEAREST: return 9728;
    case AK_MAGFILTER_LINEAR:  return 9729;
    case AK_MAGFILTER_UNSPECIFIED:
    default:                   return 0;
  }
}

static
void
gltf_w_float_array(GLTFExpWriter * __restrict w,
                   const float   * __restrict vals,
                   uint32_t                   count) {
  uint32_t i;

  gltf_w_ch(w, '[');
  for (i = 0; i < count; i++) {
    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_w_float(w, vals[i]);
  }
  gltf_w_ch(w, ']');
}

static
void
gltf_material_input_rgba(const AkMaterialInput * __restrict input,
                         float                              fallback,
                         float                * __restrict out) {
  out[0] = fallback;
  out[1] = fallback;
  out[2] = fallback;
  out[3] = 1.0f;

  if (!input)
    return;

  switch (input->valueType) {
    case AK_MATERIAL_VALUE_COLOR:
    case AK_MATERIAL_VALUE_FLOAT4:
      out[0] = input->color.vec[0];
      out[1] = input->color.vec[1];
      out[2] = input->color.vec[2];
      out[3] = input->color.vec[3];
      break;
    case AK_MATERIAL_VALUE_FLOAT3:
      out[0] = input->value[0];
      out[1] = input->value[1];
      out[2] = input->value[2];
      break;
    case AK_MATERIAL_VALUE_FLOAT2:
      out[0] = input->value[0];
      out[1] = input->value[1];
      break;
    case AK_MATERIAL_VALUE_FLOAT:
      out[0] = input->value[0];
      out[1] = input->value[0];
      out[2] = input->value[0];
      break;
    default:
      break;
  }
}

static
bool
gltf_rgba_default(const float * __restrict val,
                  float                    r,
                  float                    g,
                  float                    b,
                  float                    a) {
  return gltf_float_eq(val[0], r)
         && gltf_float_eq(val[1], g)
         && gltf_float_eq(val[2], b)
         && gltf_float_eq(val[3], a);
}

static
bool
gltf_material_input_const_noop(const AkMaterialInput * __restrict input) {
  if (!input)
    return true;

  return !input->texture
         && (input->source == AK_MATERIAL_INPUT_NONE
             || input->source == AK_MATERIAL_INPUT_CONSTANT)
         && input->flags == AK_MATERIAL_INPUT_FLAG_NONE;
}

static
bool
gltf_material_input_scalar_default(const AkMaterialInput * __restrict input,
                                   float                              fallback) {
  return gltf_material_input_const_noop(input)
         && gltf_float_eq(ak_materialInputScalar(input, fallback), fallback);
}

static
bool
gltf_material_input_rgba_default(const AkMaterialInput * __restrict input,
                                 float                              fallback,
                                 float                              r,
                                 float                              g,
                                 float                              b,
                                 float                              a) {
  float val[4];

  if (!gltf_material_input_const_noop(input))
    return false;

  gltf_material_input_rgba(input, fallback, val);
  return gltf_rgba_default(val, r, g, b, a);
}

bool
gltf_material_is_default_noop(AkMaterial * __restrict material) {
  AkMaterialSurface *surface;

  if (!material)
    return false;

  surface = material->surface;
  if (material->name
      || material->extra
      || material->flags
      || !surface)
    return false;

  if (surface->extras
      || surface->features
      || surface->featureMask
      || surface->flags
      || (surface->type != AK_MATERIAL_TYPE_NONE
          && surface->type != AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS)
      || !gltf_float_eq(surface->alphaCutoff, 0.5f)
      || !gltf_float_eq(surface->ior, 1.5f)
      || !gltf_float_eq(surface->emissiveStrength, 1.0f))
    return false;

  return gltf_material_input_rgba_default(surface->baseColor,
                                          1.0f,
                                          1.0f,
                                          1.0f,
                                          1.0f,
                                          1.0f)
         && gltf_material_input_scalar_default(surface->opacity, 1.0f)
         && gltf_material_input_scalar_default(surface->metallic, 1.0f)
         && gltf_material_input_scalar_default(surface->roughness, 1.0f)
         && gltf_material_input_scalar_default(surface->normal, 1.0f)
         && gltf_material_input_scalar_default(surface->occlusion, 1.0f)
         && gltf_material_input_rgba_default(surface->emissive,
                                             0.0f,
                                             0.0f,
                                             0.0f,
                                             0.0f,
                                             1.0f);
}

static
GLTFExpIndex
gltf_texture_index(GLTFExpState * __restrict st,
                   AkTexture    * __restrict texture) {
  return gltf_ptrs_index(&st->textures, texture);
}

static
void
gltf_write_extras_member(GLTFExpWriter * __restrict w,
                         bool          * __restrict comma,
                         AkTree        * __restrict extra) {
  if (!gltf_extra_has_json_extras(extra))
    return;

  if (*comma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_extras, _s_gltf_extras_len);
  gltf_write_extra_json_extras(w, extra);
  *comma = true;
}

static
bool
gltf_texture_info_core_extension_skip(const char * __restrict name,
                                      size_t                  nameLen,
                                      void * __restrict       userdata) {
  (void)userdata;
  return ak_str_eq_fast(name,
                          nameLen,
                          _s_gltf_KHR_texture_transform,
                          _s_gltf_KHR_texture_transform_len);
}

static
bool
gltf_texture_core_extension_skip(const char * __restrict name,
                                 size_t                  nameLen,
                                 void * __restrict       userdata) {
  (void)userdata;
  return ak_str_eq_fast(name,
                          nameLen,
                          _s_gltf_KHR_texture_basisu,
                          _s_gltf_KHR_texture_basisu_len)
         || ak_str_eq_fast(name,
                             nameLen,
                             _s_gltf_EXT_texture_webp,
                             _s_gltf_EXT_texture_webp_len);
}

static
bool
gltf_material_core_extension_skip(const char * __restrict name,
                                  size_t                  nameLen,
                                  void * __restrict       userdata) {
  (void)userdata;
  return ak_str_eq_fast(name,
                          nameLen,
                          _s_gltf_KHR_materials_unlit,
                          _s_gltf_KHR_materials_unlit_len)
         || ak_str_eq_fast(name,
                             nameLen,
                             _s_gltf_KHR_materials_emissive_strength,
                             _s_gltf_KHR_materials_emissive_strength_len)
         || ak_str_eq_fast(name,
                             nameLen,
                             _s_gltf_KHR_materials_ior,
                             _s_gltf_KHR_materials_ior_len)
         || ak_str_eq_fast(name,
                             nameLen,
                             _s_gltf_KHR_materials_clearcoat,
                             _s_gltf_KHR_materials_clearcoat_len)
         || ak_str_eq_fast(name,
                             nameLen,
                             _s_gltf_KHR_materials_specular,
                             _s_gltf_KHR_materials_specular_len)
         || ak_str_eq_fast(name,
                             nameLen,
                             _s_gltf_KHR_materials_pbrSpecularGlossiness,
                             _s_gltf_KHR_materials_pbrSpecularGlossiness_len)
         || ak_str_eq_fast(name,
                             nameLen,
                             _s_gltf_KHR_materials_transmission,
                             _s_gltf_KHR_materials_transmission_len)
         || ak_str_eq_fast(name,
                             nameLen,
                             _s_gltf_KHR_materials_sheen,
                             _s_gltf_KHR_materials_sheen_len)
         || ak_str_eq_fast(name,
                             nameLen,
                             _s_gltf_KHR_materials_iridescence,
                             _s_gltf_KHR_materials_iridescence_len)
         || ak_str_eq_fast(name,
                             nameLen,
                             _s_gltf_KHR_materials_volume,
                             _s_gltf_KHR_materials_volume_len)
         || ak_str_eq_fast(name,
                             nameLen,
                             _s_gltf_KHR_materials_anisotropy,
                             _s_gltf_KHR_materials_anisotropy_len)
         || ak_str_eq_fast(name,
                             nameLen,
                             _s_gltf_KHR_materials_dispersion,
                             _s_gltf_KHR_materials_dispersion_len)
         || ak_str_eq_fast(name,
                             nameLen,
                             _s_gltf_KHR_materials_diffuse_transmission,
                             _s_gltf_KHR_materials_diffuse_transmission_len)
         || ak_str_eq_fast(name,
                             nameLen,
                             _s_gltf_KHR_materials_volume_scatter,
                             _s_gltf_KHR_materials_volume_scatter_len);
}

static
AkTextureRef*
gltf_material_writable_texture(GLTFExpState          * __restrict st,
                               const AkMaterialInput * __restrict input) {
  AkTextureRef *texref;
  AkTextureChannels inputChannels;
  AkTextureChannels texrefChannels;

  texref = ak_materialInputTexture(input);
  if (!texref || !texref->texture)
    return NULL;

  inputChannels  = input ? input->channels : AK_TEXTURE_CHANNEL_NONE;
  texrefChannels = texref->channels;
  if (inputChannels != AK_TEXTURE_CHANNEL_NONE
      && texrefChannels != AK_TEXTURE_CHANNEL_NONE
      && (texrefChannels & inputChannels) != inputChannels)
    return NULL;

  return gltf_texture_index(st, texref->texture) != GLTF_EXP_INDEX_NONE
           ? texref
           : NULL;
}

static
uint32_t
gltf_texture_source_extension(AkTexture * __restrict texture) {
  AkImageSource *source;

  source = texture && texture->image ? gltf_image_source(texture->image) : NULL;
  if (!source)
    return 0;

  if (gltf_image_mime_or_uri_is(source,
                                _s_gltf_mime_image_ktx2,
                                _s_gltf_mime_image_ktx2_len,
                                _s_gltf_ext_ktx2,
                                _s_gltf_ext_ktx2_len))
    return GLTF_EXP_TEX_EXT_BASISU;

  if (gltf_image_mime_or_uri_is(source,
                                _s_gltf_mime_image_webp,
                                _s_gltf_mime_image_webp_len,
                                _s_gltf_ext_webp,
                                _s_gltf_ext_webp_len))
    return GLTF_EXP_TEX_EXT_WEBP;

  return 0;
}

static
bool
gltf_texture_binding_equal(GLTFExpState       * __restrict st,
                           AkMeshPrimitive    * __restrict aPrim,
                           AkInstanceGeometry * __restrict aInst,
                           AkMeshPrimitive    * __restrict bPrim,
                           AkInstanceGeometry * __restrict bInst,
                           AkTextureRef       * __restrict texref) {
  int32_t aSlot;
  int32_t bSlot;
  int32_t aTransformSlot;
  int32_t bTransformSlot;

  if (!texref || !texref->texture)
    return true;

  aSlot = ak_materialTextureSlot(aPrim, aInst, texref);
  bSlot = ak_materialTextureSlot(bPrim, bInst, texref);
  if (aSlot < 0)
    aSlot = 0;
  if (bSlot < 0)
    bSlot = 0;
  if (gltf_texcoord_export_set(st, aPrim, aSlot)
      != gltf_texcoord_export_set(st, bPrim, bSlot))
    return false;

  aTransformSlot = -1;
  bTransformSlot = -1;
  if (texref->transform && texref->transform->slot > -1) {
    if (gltf_texcoord_source_set_valid(st,
                                       aPrim,
                                       texref->transform->slot))
      aTransformSlot = gltf_texcoord_export_set(st,
                                                aPrim,
                                                texref->transform->slot);
    if (gltf_texcoord_source_set_valid(st,
                                       bPrim,
                                       texref->transform->slot))
      bTransformSlot = gltf_texcoord_export_set(st,
                                                bPrim,
                                                texref->transform->slot);
  }

  return aTransformSlot == bTransformSlot;
}

static
bool
gltf_material_input_binding_equal(
  GLTFExpState          * __restrict st,
  AkMeshPrimitive       * __restrict aPrim,
  AkInstanceGeometry    * __restrict aInst,
  AkMeshPrimitive       * __restrict bPrim,
  AkInstanceGeometry    * __restrict bInst,
  const AkMaterialInput * __restrict input) {
  return gltf_texture_binding_equal(st,
                                    aPrim,
                                    aInst,
                                    bPrim,
                                    bInst,
                                    ak_materialInputTexture(input));
}

static
bool
gltf_material_feature_binding_equal(
  GLTFExpState       * __restrict st,
  AkMeshPrimitive    * __restrict aPrim,
  AkInstanceGeometry * __restrict aInst,
  AkMeshPrimitive    * __restrict bPrim,
  AkInstanceGeometry * __restrict bInst,
  AkMaterialFeature  * __restrict feature) {
#define GLTF_INPUT_BINDING_EQUAL(INPUT)                                     \
  gltf_material_input_binding_equal(st, aPrim, aInst, bPrim, bInst, (INPUT))

  switch (feature->type) {
    case AK_MATERIAL_FEATURE_CLEARCOAT: {
      AkMaterialClearcoatFeature *f = (AkMaterialClearcoatFeature *)feature;
      return GLTF_INPUT_BINDING_EQUAL(f->factor)
             && GLTF_INPUT_BINDING_EQUAL(f->roughness)
             && GLTF_INPUT_BINDING_EQUAL(f->normal);
    }
    case AK_MATERIAL_FEATURE_SPECULAR: {
      AkMaterialSpecularFeature *f = (AkMaterialSpecularFeature *)feature;
      return GLTF_INPUT_BINDING_EQUAL(f->factor)
             && GLTF_INPUT_BINDING_EQUAL(f->color);
    }
    case AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS: {
      AkMaterialSpecularGlossinessFeature *f;
      f = (AkMaterialSpecularGlossinessFeature *)feature;
      return GLTF_INPUT_BINDING_EQUAL(f->diffuse)
             && GLTF_INPUT_BINDING_EQUAL(f->specular)
             && GLTF_INPUT_BINDING_EQUAL(f->glossiness);
    }
    case AK_MATERIAL_FEATURE_TRANSMISSION: {
      AkMaterialTransmissionFeature *f;
      f = (AkMaterialTransmissionFeature *)feature;
      return GLTF_INPUT_BINDING_EQUAL(f->factor);
    }
    case AK_MATERIAL_FEATURE_SHEEN: {
      AkMaterialSheenFeature *f = (AkMaterialSheenFeature *)feature;
      return GLTF_INPUT_BINDING_EQUAL(f->color)
             && GLTF_INPUT_BINDING_EQUAL(f->roughness);
    }
    case AK_MATERIAL_FEATURE_IRIDESCENCE: {
      AkMaterialIridescenceFeature *f;
      f = (AkMaterialIridescenceFeature *)feature;
      return GLTF_INPUT_BINDING_EQUAL(f->factor)
             && GLTF_INPUT_BINDING_EQUAL(f->thickness);
    }
    case AK_MATERIAL_FEATURE_VOLUME: {
      AkMaterialVolumeFeature *f = (AkMaterialVolumeFeature *)feature;
      return GLTF_INPUT_BINDING_EQUAL(f->thickness);
    }
    case AK_MATERIAL_FEATURE_ANISOTROPY: {
      AkMaterialAnisotropyFeature *f;
      f = (AkMaterialAnisotropyFeature *)feature;
      return GLTF_INPUT_BINDING_EQUAL(f->strength)
             && GLTF_INPUT_BINDING_EQUAL(f->rotation);
    }
    case AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION: {
      AkMaterialDiffuseTransmissionFeature *f;
      f = (AkMaterialDiffuseTransmissionFeature *)feature;
      return GLTF_INPUT_BINDING_EQUAL(f->factor)
             && GLTF_INPUT_BINDING_EQUAL(f->color);
    }
    case AK_MATERIAL_FEATURE_SUBSURFACE: {
      AkMaterialSubsurfaceFeature *f;
      f = (AkMaterialSubsurfaceFeature *)feature;
      return GLTF_INPUT_BINDING_EQUAL(f->weight)
             && GLTF_INPUT_BINDING_EQUAL(f->color)
             && GLTF_INPUT_BINDING_EQUAL(f->radius);
    }
    case AK_MATERIAL_FEATURE_CLASSIC: {
      AkMaterialClassicFeature *f = (AkMaterialClassicFeature *)feature;
      return GLTF_INPUT_BINDING_EQUAL(f->ambient)
             && GLTF_INPUT_BINDING_EQUAL(f->diffuse)
             && GLTF_INPUT_BINDING_EQUAL(f->specular)
             && GLTF_INPUT_BINDING_EQUAL(f->emission)
             && GLTF_INPUT_BINDING_EQUAL(f->reflective)
             && GLTF_INPUT_BINDING_EQUAL(f->transparency);
    }
    default:
      return true;
  }

#undef GLTF_INPUT_BINDING_EQUAL
}

bool
gltf_material_binding_equal(GLTFExpState       * __restrict st,
                            AkMaterial         * __restrict material,
                            AkMeshPrimitive    * __restrict aPrim,
                            AkInstanceGeometry * __restrict aInst,
                            AkMeshPrimitive    * __restrict bPrim,
                            AkInstanceGeometry * __restrict bInst) {
  AkMaterialSurface *surface;
  AkMaterialFeature *feature;

  if (aPrim == bPrim && aInst == bInst)
    return true;

  surface = material ? material->surface : NULL;
  if (!surface)
    return true;

  if (!gltf_material_input_binding_equal(st,
                                         aPrim,
                                         aInst,
                                         bPrim,
                                         bInst,
                                         surface->baseColor)
      || !gltf_material_input_binding_equal(st,
                                            aPrim,
                                            aInst,
                                            bPrim,
                                            bInst,
                                            surface->opacity)
      || !gltf_material_input_binding_equal(st,
                                            aPrim,
                                            aInst,
                                            bPrim,
                                            bInst,
                                            surface->metallic)
      || !gltf_material_input_binding_equal(st,
                                            aPrim,
                                            aInst,
                                            bPrim,
                                            bInst,
                                            surface->roughness)
      || !gltf_material_input_binding_equal(st,
                                            aPrim,
                                            aInst,
                                            bPrim,
                                            bInst,
                                            surface->normal)
      || !gltf_material_input_binding_equal(st,
                                            aPrim,
                                            aInst,
                                            bPrim,
                                            bInst,
                                            surface->occlusion)
      || !gltf_material_input_binding_equal(st,
                                            aPrim,
                                            aInst,
                                            bPrim,
                                            bInst,
                                            surface->emissive))
    return false;

  for (feature = surface->features; feature; feature = feature->next) {
    if (!gltf_material_feature_binding_equal(st,
                                             aPrim,
                                             aInst,
                                             bPrim,
                                             bInst,
                                             feature))
      return false;
  }

  return true;
}

GLTFExpIndex
gltf_material_index(GLTFExpState * __restrict st,
                    AkMaterial   * __restrict material,
                    AkMeshPrimitive * __restrict prim,
                    AkInstanceGeometry * __restrict inst) {
  GLTFExpIndex index;
  uintptr_t    encoded;

  if (!material)
    return GLTF_EXP_INDEX_NONE;

  encoded = (uintptr_t)rb_find(st->materials.map, material);
  index = encoded ? (GLTFExpIndex)(encoded - 1u) : GLTF_EXP_INDEX_NONE;
  while (index != GLTF_EXP_INDEX_NONE) {
    GLTFExpMaterialOut *entry;

    entry = &st->materials.items[index];
    if (gltf_material_binding_equal(st,
                                    material,
                                    entry->primitive,
                                    entry->instance,
                                    prim,
                                    inst))
      return index;
    index = entry->nextVariant;
  }

  return GLTF_EXP_INDEX_NONE;
}

static
uint32_t
gltf_material_feature_extension_mask(AkMaterialFeature * __restrict feature) {
  switch (feature->type) {
    case AK_MATERIAL_FEATURE_CLEARCOAT:
      return GLTF_EXP_MAT_EXT_CLEARCOAT;
    case AK_MATERIAL_FEATURE_SPECULAR:
      return GLTF_EXP_MAT_EXT_SPECULAR;
    case AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS:
      return GLTF_EXP_MAT_EXT_SPECULAR_GLOSSINESS;
    case AK_MATERIAL_FEATURE_TRANSMISSION:
      return GLTF_EXP_MAT_EXT_TRANSMISSION;
    case AK_MATERIAL_FEATURE_SHEEN:
      return GLTF_EXP_MAT_EXT_SHEEN;
    case AK_MATERIAL_FEATURE_IRIDESCENCE:
      return GLTF_EXP_MAT_EXT_IRIDESCENCE;
    case AK_MATERIAL_FEATURE_VOLUME:
      return GLTF_EXP_MAT_EXT_VOLUME;
    case AK_MATERIAL_FEATURE_ANISOTROPY:
      return GLTF_EXP_MAT_EXT_ANISOTROPY;
    case AK_MATERIAL_FEATURE_DISPERSION:
      return GLTF_EXP_MAT_EXT_DISPERSION;
    case AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION:
      return GLTF_EXP_MAT_EXT_DIFFUSE_TRANSMISSION;
    case AK_MATERIAL_FEATURE_SUBSURFACE:
      return GLTF_EXP_MAT_EXT_VOLUME_SCATTER;
    default:
      break;
  }

  return 0;
}

static
bool
gltf_texture_transform_has_values(AkTextureTransform * __restrict transform) {
  return transform
         && (!gltf_float_eq(transform->offset[0], 0.0f)
             || !gltf_float_eq(transform->offset[1], 0.0f)
             || !gltf_float_eq(transform->rotation, 0.0f)
             || !gltf_float_eq(transform->scale[0], 1.0f)
             || !gltf_float_eq(transform->scale[1], 1.0f));
}

static
bool
gltf_texture_transform_has_valid_slot(GLTFExpState    * __restrict st,
                                      AkMeshPrimitive * __restrict prim,
                                      AkTextureRef    * __restrict texref) {
  AkTextureTransform *transform;

  transform = texref ? texref->transform : NULL;
  return transform
         && transform->slot > -1
         && gltf_texcoord_source_set_valid(st, prim, transform->slot);
}

static
bool
gltf_texture_transform_used(GLTFExpState    * __restrict st,
                            AkMeshPrimitive * __restrict prim,
                            AkTextureRef    * __restrict texref) {
  return texref
         && texref->transform
         && (gltf_texture_transform_has_values(texref->transform)
             || gltf_texture_transform_has_valid_slot(st, prim, texref));
}

static
uint32_t
gltf_material_input_extension_mask(GLTFExpState          * __restrict st,
                                   AkMeshPrimitive       * __restrict prim,
                                   const AkMaterialInput * __restrict input) {
  return gltf_texture_transform_used(st,
                                     prim,
                                     gltf_material_writable_texture(st, input))
           ? GLTF_EXP_MAT_EXT_TEXTURE_TRANSFORM
           : 0;
}

static
uint32_t
gltf_material_feature_texture_extension_mask(
  GLTFExpState      * __restrict st,
  AkMeshPrimitive   * __restrict prim,
  AkMaterialFeature * __restrict feature) {
  uint32_t mask;

  mask = 0;
  switch (feature->type) {
    case AK_MATERIAL_FEATURE_CLEARCOAT: {
      AkMaterialClearcoatFeature *f;

      f = (AkMaterialClearcoatFeature *)feature;
      mask |= gltf_material_input_extension_mask(st, prim, f->factor);
      mask |= gltf_material_input_extension_mask(st, prim, f->roughness);
      mask |= gltf_material_input_extension_mask(st, prim, f->normal);
      break;
    }
    case AK_MATERIAL_FEATURE_SPECULAR: {
      AkMaterialSpecularFeature *f;

      f = (AkMaterialSpecularFeature *)feature;
      mask |= gltf_material_input_extension_mask(st, prim, f->factor);
      mask |= gltf_material_input_extension_mask(st, prim, f->color);
      break;
    }
    case AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS: {
      AkMaterialSpecularGlossinessFeature *f;

      f = (AkMaterialSpecularGlossinessFeature *)feature;
      mask |= gltf_material_input_extension_mask(st, prim, f->diffuse);
      mask |= gltf_material_input_extension_mask(st, prim, f->specular);
      mask |= gltf_material_input_extension_mask(st, prim, f->glossiness);
      break;
    }
    case AK_MATERIAL_FEATURE_TRANSMISSION: {
      AkMaterialTransmissionFeature *f;

      f = (AkMaterialTransmissionFeature *)feature;
      mask |= gltf_material_input_extension_mask(st, prim, f->factor);
      break;
    }
    case AK_MATERIAL_FEATURE_SHEEN: {
      AkMaterialSheenFeature *f;

      f = (AkMaterialSheenFeature *)feature;
      mask |= gltf_material_input_extension_mask(st, prim, f->color);
      mask |= gltf_material_input_extension_mask(st, prim, f->roughness);
      break;
    }
    case AK_MATERIAL_FEATURE_IRIDESCENCE: {
      AkMaterialIridescenceFeature *f;

      f = (AkMaterialIridescenceFeature *)feature;
      mask |= gltf_material_input_extension_mask(st, prim, f->factor);
      mask |= gltf_material_input_extension_mask(st, prim, f->thickness);
      break;
    }
    case AK_MATERIAL_FEATURE_VOLUME: {
      AkMaterialVolumeFeature *f;

      f = (AkMaterialVolumeFeature *)feature;
      mask |= gltf_material_input_extension_mask(st, prim, f->thickness);
      break;
    }
    case AK_MATERIAL_FEATURE_ANISOTROPY: {
      AkMaterialAnisotropyFeature *f;

      f = (AkMaterialAnisotropyFeature *)feature;
      mask |= gltf_material_input_extension_mask(st, prim, f->strength);
      break;
    }
    case AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION: {
      AkMaterialDiffuseTransmissionFeature *f;

      f = (AkMaterialDiffuseTransmissionFeature *)feature;
      mask |= gltf_material_input_extension_mask(st, prim, f->factor);
      mask |= gltf_material_input_extension_mask(st, prim, f->color);
      break;
    }
    default:
      break;
  }

  return mask;
}

static
uint32_t
gltf_material_surface_extension_mask(GLTFExpState     * __restrict st,
                                     AkMeshPrimitive   * __restrict prim,
                                     AkMaterialSurface * __restrict surface) {
  AkMaterialFeature *feature;
  uint32_t           mask;
  bool               unlit;
  float              ior;

  if (!surface)
    return 0;

  mask = 0;
  unlit = ak_materialUnlit(surface);
  if (unlit) {
    mask |= GLTF_EXP_MAT_EXT_UNLIT;
  } else {
    if (!gltf_float_eq(ak_materialEmissiveStrength(surface), 1.0f))
      mask |= GLTF_EXP_MAT_EXT_EMISSIVE_STRENGTH;

    ior = ak_materialIor(surface);
    if (ior > 0.0f
        && ((surface->flags & AK_MATERIAL_FLAG_HAS_IOR)
            || !gltf_float_eq(ior, 1.5f)))
      mask |= GLTF_EXP_MAT_EXT_IOR;

    for (feature = surface->features; feature; feature = feature->next) {
      mask |= gltf_material_feature_extension_mask(feature);
      mask |= gltf_material_feature_texture_extension_mask(st, prim, feature);
    }
  }

  mask |= gltf_material_input_extension_mask(st, prim, surface->baseColor);
  mask |= gltf_material_input_extension_mask(st, prim, surface->opacity);
  mask |= gltf_material_input_extension_mask(st, prim, surface->metallic);
  mask |= gltf_material_input_extension_mask(st, prim, surface->roughness);
  mask |= gltf_material_input_extension_mask(st, prim, surface->normal);
  mask |= gltf_material_input_extension_mask(st, prim, surface->occlusion);
  mask |= gltf_material_input_extension_mask(st, prim, surface->emissive);

  return mask;
}

uint32_t
gltf_material_extensions_mask(GLTFExpState * __restrict st) {
  AkMaterial *material;
  uint32_t    mask;
  size_t      i;

  mask = 0;
  for (i = 0; i < st->materials.count; i++) {
    GLTFExpMaterialOut *entry;

    entry    = &st->materials.items[i];
    material = entry->material;
    mask |= gltf_material_surface_extension_mask(st,
                                                 entry->primitive,
                                                 material
                                                   ? material->surface
                                                   : NULL);
  }

  return mask;
}

static
void
gltf_write_extension_name(GLTFExpWriter * __restrict w,
                          bool          * __restrict comma,
                          const char    * __restrict name,
                          size_t                     len) {
  if (*comma)
    gltf_w_ch(w, ',');
  gltf_w_qstr_len(w, name, len);
  *comma = true;
}

void
gltf_write_material_extensions_used(GLTFExpWriter * __restrict w,
                                    uint32_t                   mask,
                                    bool * __restrict          comma) {
  if (mask & GLTF_EXP_MAT_EXT_UNLIT)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_materials_unlit,
                              _s_gltf_KHR_materials_unlit_len);
  if (mask & GLTF_EXP_MAT_EXT_EMISSIVE_STRENGTH)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_materials_emissive_strength,
                              _s_gltf_KHR_materials_emissive_strength_len);
  if (mask & GLTF_EXP_MAT_EXT_IOR)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_materials_ior,
                              _s_gltf_KHR_materials_ior_len);
  if (mask & GLTF_EXP_MAT_EXT_CLEARCOAT)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_materials_clearcoat,
                              _s_gltf_KHR_materials_clearcoat_len);
  if (mask & GLTF_EXP_MAT_EXT_SPECULAR)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_materials_specular,
                              _s_gltf_KHR_materials_specular_len);
  if (mask & GLTF_EXP_MAT_EXT_SPECULAR_GLOSSINESS)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_materials_pbrSpecularGlossiness,
                              _s_gltf_KHR_materials_pbrSpecularGlossiness_len);
  if (mask & GLTF_EXP_MAT_EXT_TRANSMISSION)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_materials_transmission,
                              _s_gltf_KHR_materials_transmission_len);
  if (mask & GLTF_EXP_MAT_EXT_SHEEN)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_materials_sheen,
                              _s_gltf_KHR_materials_sheen_len);
  if (mask & GLTF_EXP_MAT_EXT_IRIDESCENCE)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_materials_iridescence,
                              _s_gltf_KHR_materials_iridescence_len);
  if (mask & GLTF_EXP_MAT_EXT_VOLUME)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_materials_volume,
                              _s_gltf_KHR_materials_volume_len);
  if (mask & GLTF_EXP_MAT_EXT_ANISOTROPY)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_materials_anisotropy,
                              _s_gltf_KHR_materials_anisotropy_len);
  if (mask & GLTF_EXP_MAT_EXT_DISPERSION)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_materials_dispersion,
                              _s_gltf_KHR_materials_dispersion_len);
  if (mask & GLTF_EXP_MAT_EXT_DIFFUSE_TRANSMISSION)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_materials_diffuse_transmission,
                              _s_gltf_KHR_materials_diffuse_transmission_len);
  if (mask & GLTF_EXP_MAT_EXT_VOLUME_SCATTER)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_materials_volume_scatter,
                              _s_gltf_KHR_materials_volume_scatter_len);
  if (mask & GLTF_EXP_MAT_EXT_TEXTURE_TRANSFORM)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_texture_transform,
                              _s_gltf_KHR_texture_transform_len);
}

uint32_t
gltf_texture_extensions_mask(GLTFExpState * __restrict st) {
  uint32_t mask;
  size_t   i;

  mask = 0;
  for (i = 0; i < st->textures.count; i++)
    mask |= gltf_texture_source_extension((AkTexture *)st->textures.items[i]);

  return mask;
}

void
gltf_write_texture_extensions_used(GLTFExpWriter * __restrict w,
                                   uint32_t                   mask,
                                   bool * __restrict          comma) {
  if (mask & GLTF_EXP_TEX_EXT_BASISU)
    gltf_write_extension_name(w, comma,
                              _s_gltf_KHR_texture_basisu,
                              _s_gltf_KHR_texture_basisu_len);
  if (mask & GLTF_EXP_TEX_EXT_WEBP)
    gltf_write_extension_name(w, comma,
                              _s_gltf_EXT_texture_webp,
                              _s_gltf_EXT_texture_webp_len);
}

bool
gltf_has_material_variants(GLTFExpState * __restrict st) {
  return st
         && st->doc
         && st->doc->materialVariants
         && st->materialVariantCount > 0
         && st->usesMaterialVariants;
}

void
gltf_write_material_variants_extension(GLTFExpWriter * __restrict w,
                                       GLTFExpState  * __restrict st) {
  AkMaterialVariant *variant;
  bool               comma;

  if (!gltf_has_material_variants(st))
    return;

  gltf_w_key(w,
             _s_gltf_KHR_materials_variants,
             _s_gltf_KHR_materials_variants_len);
  gltf_w_ch(w, '{');
  gltf_w_key(w, _s_gltf_variants, _s_gltf_variants_len);
  gltf_w_ch(w, '[');

  comma = false;
  for (variant = st->doc->materialVariants; variant; variant = variant->next) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_ch(w, '{');
    if (variant->name)
      gltf_w_key_str(w, _s_gltf_name, _s_gltf_name_len, variant->name);
    gltf_w_ch(w, '}');
    comma = true;
  }

  gltf_w_ch(w, ']');
  gltf_w_ch(w, '}');
}

static
void
gltf_write_texture_transform(GLTFExpWriter * __restrict w,
                             AkTextureRef  * __restrict texref,
                             int32_t                    slot) {
  AkTextureTransform *transform;
  bool                comma;

  transform = texref->transform;
  gltf_w_key(w,
             _s_gltf_KHR_texture_transform,
             _s_gltf_KHR_texture_transform_len);
  gltf_w_ch(w, '{');
  comma = false;

  if (!gltf_float_eq(transform->offset[0], 0.0f)
      || !gltf_float_eq(transform->offset[1], 0.0f)) {
    gltf_w_key(w, _s_gltf_offset, _s_gltf_offset_len);
    gltf_w_float_array(w, transform->offset, 2);
    comma = true;
  }

  if (!gltf_float_eq(transform->rotation, 0.0f)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_rotation, _s_gltf_rotation_len);
    gltf_w_float(w, transform->rotation);
    comma = true;
  }

  if (!gltf_float_eq(transform->scale[0], 1.0f)
      || !gltf_float_eq(transform->scale[1], 1.0f)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_scale, _s_gltf_scale_len);
    gltf_w_float_array(w, transform->scale, 2);
    comma = true;
  }

  if (slot > -1) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key_uint(w,
                    _s_gltf_texCoord,
                    _s_gltf_texCoord_len,
                    (size_t)slot);
  }

  gltf_w_ch(w, '}');
}

static
bool
gltf_write_texture_info_base(GLTFExpWriter      * __restrict w,
                             GLTFExpState       * __restrict st,
                             AkMeshPrimitive    * __restrict prim,
                             AkInstanceGeometry * __restrict inst,
                             AkTextureRef       * __restrict texref,
                             bool               * __restrict comma) {
  GLTFExpIndex texIndex;
  int32_t slot;
  int32_t transformSlot;

  if (!texref || !texref->texture) {
    w->result = AK_ERR;
    return false;
  }

  texIndex = gltf_texture_index(st, texref->texture);
  if (texIndex == GLTF_EXP_INDEX_NONE) {
    w->result = AK_ERR;
    return false;
  }

  gltf_w_key_uint(w, _s_gltf_index, _s_gltf_index_len, texIndex);
  *comma = true;

  slot = ak_materialTextureSlot(prim, inst, texref);
  if (slot < 0)
    slot = 0;
  slot = gltf_texcoord_export_set(st, prim, slot);
  if (slot > 0) {
    if (*comma)
      gltf_w_ch(w, ',');
    gltf_w_key_uint(w, _s_gltf_texCoord, _s_gltf_texCoord_len, (size_t)slot);
    *comma = true;
  }

  if (gltf_texture_transform_used(st, prim, texref)) {
    bool extensionComma;

    transformSlot = -1;
    if (gltf_texture_transform_has_valid_slot(st, prim, texref))
      transformSlot = gltf_texcoord_export_set(st,
                                               prim,
                                               texref->transform->slot);

    if (*comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_extensions, _s_gltf_extensions_len);
    gltf_w_ch(w, '{');
    gltf_write_texture_transform(w, texref, transformSlot);
    extensionComma = true;
    gltf_write_extra_extension_entries(w,
                                       ak_extra(texref),
                                       gltf_texture_info_core_extension_skip,
                                       NULL,
                                       &extensionComma);
    gltf_w_ch(w, '}');
    *comma = true;
  } else {
    gltf_write_extra_extensions_member(w,
                                       comma,
                                       ak_extra(texref),
                                       gltf_texture_info_core_extension_skip,
                                       NULL);
  }

  gltf_write_extras_member(w, comma, ak_extra(texref));

  return true;
}

static
void
gltf_write_texture_info(GLTFExpWriter      * __restrict w,
                        GLTFExpState       * __restrict st,
                        AkMeshPrimitive    * __restrict prim,
                        AkInstanceGeometry * __restrict inst,
                        AkTextureRef       * __restrict texref) {
  bool comma;

  comma = false;
  gltf_w_ch(w, '{');
  (void)gltf_write_texture_info_base(w, st, prim, inst, texref, &comma);
  gltf_w_ch(w, '}');
}

void
gltf_write_samplers(GLTFExpWriter * __restrict w,
                    GLTFExpState  * __restrict st) {
  size_t i;

  if (st->samplers.count == 0)
    return;

  gltf_w_key(w, _s_gltf_samplers, _s_gltf_samplers_len);
  gltf_w_ch(w, '[');

  for (i = 0; i < st->samplers.count; i++) {
    AkSampler *sampler;
    int        wrapS;
    int        wrapT;
    int        minFilter;
    int        magFilter;
    bool       comma;

    sampler = (AkSampler *)st->samplers.items[i];
    wrapS   = gltf_wrap_mode(sampler->wrapS);
    wrapT   = gltf_wrap_mode(sampler->wrapT);

    if (i > 0)
      gltf_w_ch(w, ',');

    comma = false;
    gltf_w_ch(w, '{');

    if (sampler->name) {
      gltf_w_key_str(w, _s_gltf_name, _s_gltf_name_len, sampler->name);
      comma = true;
    }

    if (wrapS != 10497) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_w_key_uint(w, _s_gltf_wrapS, _s_gltf_wrapS_len, (size_t)wrapS);
      comma = true;
    }

    if (wrapT != 10497) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_w_key_uint(w, _s_gltf_wrapT, _s_gltf_wrapT_len, (size_t)wrapT);
      comma = true;
    }

    minFilter = gltf_min_filter(sampler->minfilter, sampler->mipfilter);
    if (minFilter != 0) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_w_key_uint(w,
                      _s_gltf_minFilter,
                      _s_gltf_minFilter_len,
                      (size_t)minFilter);
      comma = true;
    }

    magFilter = gltf_mag_filter(sampler->magfilter);
    if (magFilter != 0) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_w_key_uint(w,
                      _s_gltf_magFilter,
                      _s_gltf_magFilter_len,
                      (size_t)magFilter);
      comma = true;
    }

    gltf_write_extra_extensions_member(w, &comma, ak_extra(sampler), NULL, NULL);
    gltf_write_extras_member(w, &comma, ak_extra(sampler));

    gltf_w_ch(w, '}');
  }

  gltf_w_ch(w, ']');
}

void
gltf_write_images(GLTFExpWriter * __restrict w,
                  GLTFExpState  * __restrict st) {
  size_t i;

  if (st->images.count == 0)
    return;

  gltf_w_key(w, _s_gltf_images, _s_gltf_images_len);
  gltf_w_ch(w, '[');

  for (i = 0; i < st->images.count; i++) {
    AkImage       *image;
    AkImageSource *source;
    bool           comma;

    image  = (AkImage *)st->images.items[i];
    source = gltf_image_source(image);

    if (i > 0)
      gltf_w_ch(w, ',');

    comma = false;
    gltf_w_ch(w, '{');

    if (image->name) {
      gltf_w_key_str(w, _s_gltf_name, _s_gltf_name_len, image->name);
      comma = true;
    }

    if (st->imageBufferViews
        && st->imageBufferViews[i] != GLTF_EXP_INDEX_NONE) {
      const char *mimeType;

      mimeType = st->imageMimeTypes ? st->imageMimeTypes[i] : source->mimeType;
      if (!mimeType) {
        w->result = AK_ERR;
        return;
      }

      if (comma)
        gltf_w_ch(w, ',');
      gltf_w_key_uint(w,
                      _s_gltf_bufferView,
                      _s_gltf_bufferView_len,
                      st->imageBufferViews[i]);
      gltf_w_ch(w, ',');
      gltf_w_key_str(w,
                     _s_gltf_mimeType,
                     _s_gltf_mimeType_len,
                     mimeType);
      comma = true;
    } else {
      if (!source) {
        w->result = AK_ERR;
        return;
      }

      switch (source->type) {
      case AK_IMAGE_SOURCE_URI:
        {
          const char *uri;

          uri = st->imageExportUris ? st->imageExportUris[i] : NULL;
          if (!uri || !gltf_image_copy_export_uri(st, source, uri)) {
            w->result = AK_ERR;
            return;
          }
          if (comma)
            gltf_w_ch(w, ',');
          gltf_w_key_str(w, _s_gltf_uri, _s_gltf_uri_len, uri);
          comma = true;

          if (source->mimeType) {
            gltf_w_ch(w, ',');
            gltf_w_key_str(w,
                           _s_gltf_mimeType,
                           _s_gltf_mimeType_len,
                           source->mimeType);
            comma = true;
          }
        }
        break;
      case AK_IMAGE_SOURCE_BUFFER:
      default:
        w->result = AK_ERR;
        return;
      }
    }

    gltf_write_extra_extensions_member(w, &comma, ak_extra(image), NULL, NULL);
    gltf_write_extras_member(w, &comma, ak_extra(image));

    gltf_w_ch(w, '}');
  }

  gltf_w_ch(w, ']');
}

void
gltf_write_textures(GLTFExpWriter * __restrict w,
                    GLTFExpState  * __restrict st) {
  size_t i;

  if (st->textures.count == 0)
    return;

  gltf_w_key(w, _s_gltf_textures, _s_gltf_textures_len);
  gltf_w_ch(w, '[');

  for (i = 0; i < st->textures.count; i++) {
    AkTexture *texture;
    GLTFExpIndex imageIndex;
    GLTFExpIndex samplerIndex;
    uint32_t   sourceExt;
    bool       comma;

    texture    = (AkTexture *)st->textures.items[i];
    imageIndex = gltf_ptrs_index(&st->images, texture->image);
    sourceExt  = gltf_texture_source_extension(texture);

    if (imageIndex == GLTF_EXP_INDEX_NONE) {
      w->result = AK_ERR;
      return;
    }

    if (i > 0)
      gltf_w_ch(w, ',');

    comma = false;
    gltf_w_ch(w, '{');

    if (texture->name) {
      gltf_w_key_str(w, _s_gltf_name, _s_gltf_name_len, texture->name);
      comma = true;
    }

    samplerIndex = texture->sampler
                   ? gltf_ptrs_index(&st->samplers, texture->sampler)
                   : GLTF_EXP_INDEX_NONE;
    if (samplerIndex != GLTF_EXP_INDEX_NONE) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_w_key_uint(w,
                      _s_gltf_sampler,
                      _s_gltf_sampler_len,
                      samplerIndex);
      comma = true;
    }

    if (comma)
      gltf_w_ch(w, ',');
    if (sourceExt == GLTF_EXP_TEX_EXT_BASISU) {
      bool extensionComma;

      gltf_w_key(w, _s_gltf_extensions, _s_gltf_extensions_len);
      gltf_w_ch(w, '{');
      gltf_w_key(w,
                 _s_gltf_KHR_texture_basisu,
                 _s_gltf_KHR_texture_basisu_len);
      gltf_w_ch(w, '{');
      gltf_w_key_uint(w, _s_gltf_source, _s_gltf_source_len, imageIndex);
      gltf_w_ch(w, '}');
      extensionComma = true;
      gltf_write_extra_extension_entries(w,
                                         ak_extra(texture),
                                         gltf_texture_core_extension_skip,
                                         NULL,
                                         &extensionComma);
      gltf_w_ch(w, '}');
      comma = true;
    } else if (sourceExt == GLTF_EXP_TEX_EXT_WEBP) {
      bool extensionComma;

      gltf_w_key(w, _s_gltf_extensions, _s_gltf_extensions_len);
      gltf_w_ch(w, '{');
      gltf_w_key(w,
                 _s_gltf_EXT_texture_webp,
                 _s_gltf_EXT_texture_webp_len);
      gltf_w_ch(w, '{');
      gltf_w_key_uint(w, _s_gltf_source, _s_gltf_source_len, imageIndex);
      gltf_w_ch(w, '}');
      extensionComma = true;
      gltf_write_extra_extension_entries(w,
                                         ak_extra(texture),
                                         gltf_texture_core_extension_skip,
                                         NULL,
                                         &extensionComma);
      gltf_w_ch(w, '}');
      comma = true;
    } else {
      gltf_w_key_uint(w, _s_gltf_source, _s_gltf_source_len, imageIndex);
      comma = true;
      gltf_write_extra_extensions_member(w,
                                         &comma,
                                         ak_extra(texture),
                                         gltf_texture_core_extension_skip,
                                         NULL);
    }

    gltf_write_extras_member(w, &comma, ak_extra(texture));

    gltf_w_ch(w, '}');
  }

  gltf_w_ch(w, ']');
}

static
bool
gltf_write_pbr(GLTFExpWriter      * __restrict w,
               GLTFExpState       * __restrict st,
               AkMeshPrimitive    * __restrict prim,
               AkInstanceGeometry * __restrict inst,
               AkMaterialSurface  * __restrict surface,
               bool               * __restrict outerComma) {
  AkTextureRef *baseTex;
  AkTextureRef *metalTex;
  AkTextureRef *roughTex;
  AkTextureRef *metalRoughTex;
  float         base[4];
  float         metallic;
  float         roughness;
  bool          any;
  bool          comma;

  if (!surface)
    return false;

  baseTex  = gltf_material_writable_texture(st, surface->baseColor);
  metalTex = gltf_material_writable_texture(st, surface->metallic);
  roughTex = gltf_material_writable_texture(st, surface->roughness);
  metalRoughTex = metalTex
                  && roughTex
                  && metalTex->texture == roughTex->texture
                    ? metalTex
                    : NULL;

  gltf_material_input_rgba(surface->baseColor, 1.0f, base);
  base[3] *= ak_materialOpacityFactor(surface);

  metallic  = ak_materialMetallicFactor(surface);
  roughness = ak_materialRoughnessFactor(surface);
  any       = baseTex
              || metalRoughTex
              || !gltf_rgba_default(base, 1.0f, 1.0f, 1.0f, 1.0f)
              || !gltf_float_eq(metallic, 1.0f)
              || !gltf_float_eq(roughness, 1.0f);
  if (!any)
    return false;

  if (*outerComma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_pbrMetalRough, _s_gltf_pbrMetalRough_len);
  gltf_w_ch(w, '{');
  comma = false;

  if (!gltf_rgba_default(base, 1.0f, 1.0f, 1.0f, 1.0f)) {
    gltf_w_key(w, _s_gltf_baseColor, _s_gltf_baseColor_len);
    gltf_w_float_array(w, base, 4);
    comma = true;
  }

  if (baseTex) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_baseColorTex, _s_gltf_baseColorTex_len);
    gltf_write_texture_info(w, st, prim, inst, baseTex);
    comma = true;
  }

  if (!gltf_float_eq(metallic, 1.0f)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_metalFac, _s_gltf_metalFac_len);
    gltf_w_float(w, metallic);
    comma = true;
  }

  if (!gltf_float_eq(roughness, 1.0f)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_roughFac, _s_gltf_roughFac_len);
    gltf_w_float(w, roughness);
    comma = true;
  }

  if (metalRoughTex) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_metalRoughTex, _s_gltf_metalRoughTex_len);
    gltf_write_texture_info(w, st, prim, inst, metalRoughTex);
  }

  gltf_w_ch(w, '}');
  *outerComma = true;

  return true;
}

static
bool
gltf_write_normal(GLTFExpWriter      * __restrict w,
                  GLTFExpState       * __restrict st,
                  AkMeshPrimitive    * __restrict prim,
                  AkInstanceGeometry * __restrict inst,
                  AkMaterialSurface  * __restrict surface,
                  bool               * __restrict outerComma) {
  AkTextureRef *texref;
  bool          comma;
  float         scale;

  texref = surface ? gltf_material_writable_texture(st, surface->normal) : NULL;
  if (!texref)
    return false;

  if (*outerComma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_normalTex, _s_gltf_normalTex_len);

  scale = ak_materialNormalScale(surface);
  if (gltf_float_eq(scale, 1.0f)) {
    gltf_write_texture_info(w, st, prim, inst, texref);
    *outerComma = true;
    return true;
  }

  comma = false;
  gltf_w_ch(w, '{');
  if (!gltf_write_texture_info_base(w, st, prim, inst, texref, &comma))
    return false;
  if (comma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_scale, _s_gltf_scale_len);
  gltf_w_float(w, scale);
  gltf_w_ch(w, '}');
  *outerComma = true;

  return true;
}

static
bool
gltf_write_occlusion(GLTFExpWriter      * __restrict w,
                     GLTFExpState       * __restrict st,
                     AkMeshPrimitive    * __restrict prim,
                     AkInstanceGeometry * __restrict inst,
                     AkMaterialSurface  * __restrict surface,
                     bool               * __restrict outerComma) {
  AkTextureRef *texref;
  bool          comma;
  float         strength;

  texref = surface
           ? gltf_material_writable_texture(st, surface->occlusion)
           : NULL;
  if (!texref)
    return false;

  if (*outerComma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_occlusionTex, _s_gltf_occlusionTex_len);

  strength = ak_materialOcclusionStrength(surface);
  if (gltf_float_eq(strength, 1.0f)) {
    gltf_write_texture_info(w, st, prim, inst, texref);
    *outerComma = true;
    return true;
  }

  comma = false;
  gltf_w_ch(w, '{');
  if (!gltf_write_texture_info_base(w, st, prim, inst, texref, &comma))
    return false;

  if (comma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_strength, _s_gltf_strength_len);
  gltf_w_float(w, strength);
  gltf_w_ch(w, '}');
  *outerComma = true;

  return true;
}

static
bool
gltf_write_emissive(GLTFExpWriter      * __restrict w,
                    GLTFExpState       * __restrict st,
                    AkMeshPrimitive    * __restrict prim,
                    AkInstanceGeometry * __restrict inst,
                    AkMaterialSurface  * __restrict surface,
                    bool               * __restrict comma) {
  AkTextureRef *texref;
  float         emissive[4];
  bool          wrote;

  if (!surface)
    return false;

  wrote = false;
  gltf_material_input_rgba(surface->emissive, 0.0f, emissive);
  if (!gltf_rgba_default(emissive, 0.0f, 0.0f, 0.0f, 1.0f)) {
    if (*comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_emissiveFac, _s_gltf_emissiveFac_len);
    gltf_w_float_array(w, emissive, 3);
    *comma = true;
    wrote  = true;
  }

  texref = gltf_material_writable_texture(st, surface->emissive);
  if (texref) {
    if (*comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_emissiveTex, _s_gltf_emissiveTex_len);
    gltf_write_texture_info(w, st, prim, inst, texref);
    *comma = true;
    wrote  = true;
  }

  return wrote;
}

static
bool
gltf_write_scalar_input(GLTFExpWriter          * __restrict w,
                        GLTFExpState           * __restrict st,
                        AkMeshPrimitive        * __restrict prim,
                        AkInstanceGeometry     * __restrict inst,
                        const AkMaterialInput  * __restrict input,
                        float                               fallback,
                        const char            * __restrict factorKey,
                        size_t                              factorKeyLen,
                        const char            * __restrict textureKey,
                        size_t                              textureKeyLen,
                        bool                  * __restrict comma) {
  AkTextureRef *texref;
  float         factor;
  bool          wrote;

  texref = gltf_material_writable_texture(st, input);
  factor = ak_materialInputScalar(input, fallback);
  wrote  = false;

  if (!gltf_float_eq(factor, fallback)) {
    if (*comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, factorKey, factorKeyLen);
    gltf_w_float(w, factor);
    *comma = true;
    wrote  = true;
  }

  if (texref) {
    if (*comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, textureKey, textureKeyLen);
    gltf_write_texture_info(w, st, prim, inst, texref);
    *comma = true;
    wrote  = true;
  }

  return wrote;
}

static
bool
gltf_write_color3_input(GLTFExpWriter          * __restrict w,
                        GLTFExpState           * __restrict st,
                        AkMeshPrimitive        * __restrict prim,
                        AkInstanceGeometry     * __restrict inst,
                        const AkMaterialInput  * __restrict input,
                        float                               fallback,
                        const char            * __restrict factorKey,
                        size_t                              factorKeyLen,
                        const char            * __restrict textureKey,
                        size_t                              textureKeyLen,
                        bool                  * __restrict comma) {
  AkTextureRef *texref;
  float         color[4];
  bool          wrote;

  texref = gltf_material_writable_texture(st, input);
  gltf_material_input_rgba(input, fallback, color);
  wrote = false;

  if (!gltf_float_eq(color[0], fallback)
      || !gltf_float_eq(color[1], fallback)
      || !gltf_float_eq(color[2], fallback)) {
    if (*comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, factorKey, factorKeyLen);
    gltf_w_float_array(w, color, 3);
    *comma = true;
    wrote  = true;
  }

  if (texref) {
    if (*comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, textureKey, textureKeyLen);
    gltf_write_texture_info(w, st, prim, inst, texref);
    *comma = true;
    wrote  = true;
  }

  return wrote;
}

static
void
gltf_write_normal_texture_info(GLTFExpWriter      * __restrict w,
                               GLTFExpState       * __restrict st,
                               AkMeshPrimitive    * __restrict prim,
                               AkInstanceGeometry * __restrict inst,
                               AkTextureRef       * __restrict texref,
                               float                           scale) {
  bool    comma;

  if (gltf_float_eq(scale, 1.0f)) {
    gltf_write_texture_info(w, st, prim, inst, texref);
    return;
  }

  comma = false;
  gltf_w_ch(w, '{');
  if (!gltf_write_texture_info_base(w, st, prim, inst, texref, &comma))
    return;

  if (comma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_scale, _s_gltf_scale_len);
  gltf_w_float(w, scale);
  gltf_w_ch(w, '}');
}

static
void
gltf_write_clearcoat_extension(GLTFExpWriter             * __restrict w,
                               GLTFExpState              * __restrict st,
                               AkMeshPrimitive           * __restrict prim,
                               AkInstanceGeometry        * __restrict inst,
                               AkMaterialClearcoatFeature * __restrict f) {
  AkTextureRef *normalTex;
  bool          comma;

  comma = false;
  gltf_w_ch(w, '{');
  gltf_write_scalar_input(w, st, prim, inst, f->factor, 0.0f,
                          _s_gltf_clearcoatFactor,
                          _s_gltf_clearcoatFactor_len,
                          _s_gltf_clearcoatTexture,
                          _s_gltf_clearcoatTexture_len,
                          &comma);
  gltf_write_scalar_input(w, st, prim, inst, f->roughness, 0.0f,
                          _s_gltf_clearcoatRoughnessFactor,
                          _s_gltf_clearcoatRoughnessFactor_len,
                          _s_gltf_clearcoatRoughnessTexture,
                          _s_gltf_clearcoatRoughnessTexture_len,
                          &comma);

  normalTex = gltf_material_writable_texture(st, f->normal);
  if (normalTex) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w,
               _s_gltf_clearcoatNormalTexture,
               _s_gltf_clearcoatNormalTexture_len);
    gltf_write_normal_texture_info(w, st, prim, inst,
                                   normalTex,
                                   f->normalScale);
  }
  gltf_w_ch(w, '}');
}

static
void
gltf_write_specular_extension(GLTFExpWriter             * __restrict w,
                              GLTFExpState              * __restrict st,
                              AkMeshPrimitive           * __restrict prim,
                              AkInstanceGeometry        * __restrict inst,
                              AkMaterialSpecularFeature * __restrict f) {
  bool comma;

  comma = false;
  gltf_w_ch(w, '{');
  gltf_write_scalar_input(w, st, prim, inst, f->factor, 1.0f,
                          _s_gltf_specularFactor,
                          _s_gltf_specularFactor_len,
                          _s_gltf_specularTexture,
                          _s_gltf_specularTexture_len,
                          &comma);
  gltf_write_color3_input(w, st, prim, inst, f->color, 1.0f,
                          _s_gltf_specularColorFactor,
                          _s_gltf_specularColorFactor_len,
                          _s_gltf_specularColorTexture,
                          _s_gltf_specularColorTexture_len,
                          &comma);
  gltf_w_ch(w, '}');
}

static
void
gltf_write_specular_glossiness_extension(
  GLTFExpWriter                        * __restrict w,
  GLTFExpState                         * __restrict st,
  AkMeshPrimitive                      * __restrict prim,
  AkInstanceGeometry                   * __restrict inst,
  AkMaterialSpecularGlossinessFeature  * __restrict f) {
  AkTextureRef *diffuseTex;
  AkTextureRef *specTex;
  AkTextureRef *glossTex;
  AkTextureRef *specGlossTex;
  float         diffuse[4];
  float         specular[4];
  float         glossiness;
  bool          comma;

  diffuseTex = gltf_material_writable_texture(st, f->diffuse);
  specTex    = gltf_material_writable_texture(st, f->specular);
  glossTex   = gltf_material_writable_texture(st, f->glossiness);
  specGlossTex = specTex ? specTex : glossTex;

  gltf_material_input_rgba(f->diffuse, 1.0f, diffuse);
  gltf_material_input_rgba(f->specular, 1.0f, specular);
  glossiness = ak_materialInputScalar(f->glossiness, 1.0f);

  comma = false;
  gltf_w_ch(w, '{');

  if (!gltf_rgba_default(diffuse, 1.0f, 1.0f, 1.0f, 1.0f)) {
    gltf_w_key(w, _s_gltf_diffuseFactor, _s_gltf_diffuseFactor_len);
    gltf_w_float_array(w, diffuse, 4);
    comma = true;
  }

  if (diffuseTex) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_diffuseTexture, _s_gltf_diffuseTexture_len);
    gltf_write_texture_info(w, st, prim, inst, diffuseTex);
    comma = true;
  }

  if (!gltf_float_eq(specular[0], 1.0f)
      || !gltf_float_eq(specular[1], 1.0f)
      || !gltf_float_eq(specular[2], 1.0f)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_specFactor, _s_gltf_specFactor_len);
    gltf_w_float_array(w, specular, 3);
    comma = true;
  }

  if (!gltf_float_eq(glossiness, 1.0f)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_glossFactor, _s_gltf_glossFactor_len);
    gltf_w_float(w, glossiness);
    comma = true;
  }

  if (specGlossTex) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_specGlossTex, _s_gltf_specGlossTex_len);
    gltf_write_texture_info(w, st, prim, inst, specGlossTex);
  }

  gltf_w_ch(w, '}');
}

static
void
gltf_write_transmission_extension(GLTFExpWriter                 * __restrict w,
                                  GLTFExpState                  * __restrict st,
                                  AkMeshPrimitive               * __restrict prim,
                                  AkInstanceGeometry            * __restrict inst,
                                  AkMaterialTransmissionFeature * __restrict f) {
  bool comma;

  comma = false;
  gltf_w_ch(w, '{');
  gltf_write_scalar_input(w, st, prim, inst, f->factor, 0.0f,
                          _s_gltf_transmissionFactor,
                          _s_gltf_transmissionFactor_len,
                          _s_gltf_transmissionTexture,
                          _s_gltf_transmissionTexture_len,
                          &comma);
  gltf_w_ch(w, '}');
}

static
void
gltf_write_sheen_extension(GLTFExpWriter         * __restrict w,
                           GLTFExpState          * __restrict st,
                           AkMeshPrimitive       * __restrict prim,
                           AkInstanceGeometry    * __restrict inst,
                           AkMaterialSheenFeature * __restrict f) {
  bool comma;

  comma = false;
  gltf_w_ch(w, '{');
  gltf_write_color3_input(w, st, prim, inst, f->color, 0.0f,
                          _s_gltf_sheenColorFactor,
                          _s_gltf_sheenColorFactor_len,
                          _s_gltf_sheenColorTexture,
                          _s_gltf_sheenColorTexture_len,
                          &comma);
  gltf_write_scalar_input(w, st, prim, inst, f->roughness, 0.0f,
                          _s_gltf_sheenRoughnessFactor,
                          _s_gltf_sheenRoughnessFactor_len,
                          _s_gltf_sheenRoughnessTexture,
                          _s_gltf_sheenRoughnessTexture_len,
                          &comma);
  gltf_w_ch(w, '}');
}

static
void
gltf_write_iridescence_extension(GLTFExpWriter                * __restrict w,
                                 GLTFExpState                 * __restrict st,
                                 AkMeshPrimitive              * __restrict prim,
                                 AkInstanceGeometry           * __restrict inst,
                                 AkMaterialIridescenceFeature * __restrict f) {
  AkTextureRef *thicknessTex;
  bool          comma;

  comma = false;
  gltf_w_ch(w, '{');
  gltf_write_scalar_input(w, st, prim, inst, f->factor, 0.0f,
                          _s_gltf_iridescenceFactor,
                          _s_gltf_iridescenceFactor_len,
                          _s_gltf_iridescenceTexture,
                          _s_gltf_iridescenceTexture_len,
                          &comma);
  if (!gltf_float_eq(f->ior, 1.3f)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_iridescenceIor, _s_gltf_iridescenceIor_len);
    gltf_w_float(w, f->ior);
    comma = true;
  }
  if (!gltf_float_eq(f->thicknessMinimum, 100.0f)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w,
               _s_gltf_iridescenceThicknessMinimum,
               _s_gltf_iridescenceThicknessMinimum_len);
    gltf_w_float(w, f->thicknessMinimum);
    comma = true;
  }
  if (!gltf_float_eq(f->thicknessMaximum, 400.0f)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w,
               _s_gltf_iridescenceThicknessMaximum,
               _s_gltf_iridescenceThicknessMaximum_len);
    gltf_w_float(w, f->thicknessMaximum);
    comma = true;
  }

  thicknessTex = gltf_material_writable_texture(st, f->thickness);
  if (thicknessTex) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w,
               _s_gltf_iridescenceThicknessTexture,
               _s_gltf_iridescenceThicknessTexture_len);
    gltf_write_texture_info(w, st, prim, inst, thicknessTex);
  }
  gltf_w_ch(w, '}');
}

static
void
gltf_write_volume_extension(GLTFExpWriter         * __restrict w,
                            GLTFExpState          * __restrict st,
                            AkMeshPrimitive       * __restrict prim,
                            AkInstanceGeometry    * __restrict inst,
                            AkMaterialVolumeFeature * __restrict f) {
  bool comma;

  comma = false;
  gltf_w_ch(w, '{');
  gltf_write_scalar_input(w, st, prim, inst, f->thickness, 0.0f,
                          _s_gltf_thicknessFactor,
                          _s_gltf_thicknessFactor_len,
                          _s_gltf_thicknessTexture,
                          _s_gltf_thicknessTexture_len,
                          &comma);
  if (isfinite(f->attenuationDistance)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w,
               _s_gltf_attenuationDistance,
               _s_gltf_attenuationDistance_len);
    gltf_w_float(w, f->attenuationDistance);
    comma = true;
  }
  if (!gltf_float_eq(f->attenuationColor.vec[0], 1.0f)
      || !gltf_float_eq(f->attenuationColor.vec[1], 1.0f)
      || !gltf_float_eq(f->attenuationColor.vec[2], 1.0f)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w,
               _s_gltf_attenuationColor,
               _s_gltf_attenuationColor_len);
    gltf_w_float_array(w, f->attenuationColor.vec, 3);
  }
  gltf_w_ch(w, '}');
}

static
void
gltf_write_anisotropy_extension(GLTFExpWriter              * __restrict w,
                                GLTFExpState               * __restrict st,
                                AkMeshPrimitive            * __restrict prim,
                                AkInstanceGeometry         * __restrict inst,
                                AkMaterialAnisotropyFeature * __restrict f) {
  bool comma;

  comma = false;
  gltf_w_ch(w, '{');
  gltf_write_scalar_input(w, st, prim, inst, f->strength, 0.0f,
                          _s_gltf_anisotropyStrength,
                          _s_gltf_anisotropyStrength_len,
                          _s_gltf_anisotropyTexture,
                          _s_gltf_anisotropyTexture_len,
                          &comma);
  if (!gltf_float_eq(ak_materialInputScalar(f->rotation, 0.0f), 0.0f)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_anisotropyRotation,
               _s_gltf_anisotropyRotation_len);
    gltf_w_float(w, ak_materialInputScalar(f->rotation, 0.0f));
  }
  gltf_w_ch(w, '}');
}

static
void
gltf_write_diffuse_transmission_extension(
  GLTFExpWriter                        * __restrict w,
  GLTFExpState                         * __restrict st,
  AkMeshPrimitive                      * __restrict prim,
  AkInstanceGeometry                   * __restrict inst,
  AkMaterialDiffuseTransmissionFeature * __restrict f) {
  bool comma;

  comma = false;
  gltf_w_ch(w, '{');
  gltf_write_scalar_input(w, st, prim, inst, f->factor, 0.0f,
                          _s_gltf_diffuseTransmissionFactor,
                          _s_gltf_diffuseTransmissionFactor_len,
                          _s_gltf_diffuseTransmissionTexture,
                          _s_gltf_diffuseTransmissionTexture_len,
                          &comma);
  gltf_write_color3_input(w, st, prim, inst, f->color, 1.0f,
                          _s_gltf_diffuseTransmissionColorFactor,
                          _s_gltf_diffuseTransmissionColorFactor_len,
                          _s_gltf_diffuseTransmissionColorTexture,
                          _s_gltf_diffuseTransmissionColorTexture_len,
                          &comma);
  gltf_w_ch(w, '}');
}

static
void
gltf_write_volume_scatter_extension(GLTFExpWriter                * __restrict w,
                                    AkMaterialSubsurfaceFeature  * __restrict f) {
  float color[4];
  bool  comma;

  comma = false;
  gltf_w_ch(w, '{');
  gltf_material_input_rgba(f->color, 0.0f, color);
  if (!gltf_float_eq(color[0], 0.0f)
      || !gltf_float_eq(color[1], 0.0f)
      || !gltf_float_eq(color[2], 0.0f)) {
    gltf_w_key(w,
               _s_gltf_multiscatterColorFactor,
               _s_gltf_multiscatterColorFactor_len);
    gltf_w_float_array(w, color, 3);
    comma = true;
  }

  if (!gltf_float_eq(f->anisotropy, 0.0f)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_scatterAnisotropy,
               _s_gltf_scatterAnisotropy_len);
    gltf_w_float(w, f->anisotropy);
  }
  gltf_w_ch(w, '}');
}

static
void
gltf_write_feature_extension(GLTFExpWriter      * __restrict w,
                             GLTFExpState       * __restrict st,
                             AkMeshPrimitive    * __restrict prim,
                             AkInstanceGeometry * __restrict inst,
                             AkMaterialFeature  * __restrict feature) {
  switch (feature->type) {
    case AK_MATERIAL_FEATURE_CLEARCOAT:
      gltf_write_clearcoat_extension(w, st, prim, inst, (void *)feature);
      break;
    case AK_MATERIAL_FEATURE_SPECULAR:
      gltf_write_specular_extension(w, st, prim, inst, (void *)feature);
      break;
    case AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS:
      gltf_write_specular_glossiness_extension(w, st, prim, inst,
                                               (void *)feature);
      break;
    case AK_MATERIAL_FEATURE_TRANSMISSION:
      gltf_write_transmission_extension(w, st, prim, inst, (void *)feature);
      break;
    case AK_MATERIAL_FEATURE_SHEEN:
      gltf_write_sheen_extension(w, st, prim, inst, (void *)feature);
      break;
    case AK_MATERIAL_FEATURE_IRIDESCENCE:
      gltf_write_iridescence_extension(w, st, prim, inst, (void *)feature);
      break;
    case AK_MATERIAL_FEATURE_VOLUME:
      gltf_write_volume_extension(w, st, prim, inst, (void *)feature);
      break;
    case AK_MATERIAL_FEATURE_ANISOTROPY:
      gltf_write_anisotropy_extension(w, st, prim, inst, (void *)feature);
      break;
    case AK_MATERIAL_FEATURE_DISPERSION: {
      AkMaterialDispersionFeature *f;

      f = (AkMaterialDispersionFeature *)feature;
      gltf_w_ch(w, '{');
      if (!gltf_float_eq(f->dispersion, 0.0f)) {
        gltf_w_key(w, _s_gltf_dispersion, _s_gltf_dispersion_len);
        gltf_w_float(w, f->dispersion);
      }
      gltf_w_ch(w, '}');
      break;
    }
    case AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION:
      gltf_write_diffuse_transmission_extension(w, st, prim, inst,
                                                (void *)feature);
      break;
    case AK_MATERIAL_FEATURE_SUBSURFACE:
      gltf_write_volume_scatter_extension(w, (void *)feature);
      break;
    default:
      gltf_w_ch(w, '{');
      gltf_w_ch(w, '}');
      break;
  }
}

static
void
gltf_write_material_extension_entry(GLTFExpWriter * __restrict w,
                                    bool          * __restrict comma,
                                    const char    * __restrict name,
                                    size_t                     len) {
  if (*comma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, name, len);
  *comma = true;
}

static
bool
gltf_write_material_extensions(GLTFExpWriter      * __restrict w,
                               GLTFExpState       * __restrict st,
                               AkMeshPrimitive    * __restrict prim,
                               AkInstanceGeometry * __restrict inst,
                               AkMaterialSurface  * __restrict surface,
                               AkTree             * __restrict extra,
                               bool               * __restrict outerComma) {
  AkMaterialFeature *feature;
  uint32_t           mask;
  bool               comma;
  bool               hasPreserved;

  mask = gltf_material_surface_extension_mask(st, prim, surface)
         & ~GLTF_EXP_MAT_EXT_TEXTURE_TRANSFORM;
  hasPreserved = gltf_extra_has_extensions(extra,
                                           gltf_material_core_extension_skip,
                                           NULL);
  if (mask == 0 && !hasPreserved)
    return false;

  if (*outerComma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_extensions, _s_gltf_extensions_len);
  gltf_w_ch(w, '{');
  comma = false;

  if (mask & GLTF_EXP_MAT_EXT_UNLIT) {
    gltf_write_material_extension_entry(w, &comma,
                                        _s_gltf_KHR_materials_unlit,
                                        _s_gltf_KHR_materials_unlit_len);
    gltf_w_ch(w, '{');
    gltf_w_ch(w, '}');
  }
  if (mask & GLTF_EXP_MAT_EXT_EMISSIVE_STRENGTH) {
    gltf_write_material_extension_entry(
      w, &comma,
      _s_gltf_KHR_materials_emissive_strength,
      _s_gltf_KHR_materials_emissive_strength_len);
    gltf_w_ch(w, '{');
    gltf_w_key(w, _s_gltf_emissiveStrength, _s_gltf_emissiveStrength_len);
    gltf_w_float(w, ak_materialEmissiveStrength(surface));
    gltf_w_ch(w, '}');
  }
  if (mask & GLTF_EXP_MAT_EXT_IOR) {
    gltf_write_material_extension_entry(w, &comma,
                                        _s_gltf_KHR_materials_ior,
                                        _s_gltf_KHR_materials_ior_len);
    gltf_w_ch(w, '{');
    gltf_w_key(w, _s_gltf_ior, _s_gltf_ior_len);
    gltf_w_float(w, ak_materialIor(surface));
    gltf_w_ch(w, '}');
  }

  for (feature = surface ? surface->features : NULL; feature; feature = feature->next) {
    uint32_t featureMask;

    featureMask = gltf_material_feature_extension_mask(feature);
    if (featureMask == 0)
      continue;

    switch (feature->type) {
      case AK_MATERIAL_FEATURE_CLEARCOAT:
        gltf_write_material_extension_entry(
          w, &comma,
          _s_gltf_KHR_materials_clearcoat,
          _s_gltf_KHR_materials_clearcoat_len);
        break;
      case AK_MATERIAL_FEATURE_SPECULAR:
        gltf_write_material_extension_entry(
          w, &comma,
          _s_gltf_KHR_materials_specular,
          _s_gltf_KHR_materials_specular_len);
        break;
      case AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS:
        gltf_write_material_extension_entry(
          w, &comma,
          _s_gltf_KHR_materials_pbrSpecularGlossiness,
          _s_gltf_KHR_materials_pbrSpecularGlossiness_len);
        break;
      case AK_MATERIAL_FEATURE_TRANSMISSION:
        gltf_write_material_extension_entry(
          w, &comma,
          _s_gltf_KHR_materials_transmission,
          _s_gltf_KHR_materials_transmission_len);
        break;
      case AK_MATERIAL_FEATURE_SHEEN:
        gltf_write_material_extension_entry(
          w, &comma,
          _s_gltf_KHR_materials_sheen,
          _s_gltf_KHR_materials_sheen_len);
        break;
      case AK_MATERIAL_FEATURE_IRIDESCENCE:
        gltf_write_material_extension_entry(
          w, &comma,
          _s_gltf_KHR_materials_iridescence,
          _s_gltf_KHR_materials_iridescence_len);
        break;
      case AK_MATERIAL_FEATURE_VOLUME:
        gltf_write_material_extension_entry(
          w, &comma,
          _s_gltf_KHR_materials_volume,
          _s_gltf_KHR_materials_volume_len);
        break;
      case AK_MATERIAL_FEATURE_ANISOTROPY:
        gltf_write_material_extension_entry(
          w, &comma,
          _s_gltf_KHR_materials_anisotropy,
          _s_gltf_KHR_materials_anisotropy_len);
        break;
      case AK_MATERIAL_FEATURE_DISPERSION:
        gltf_write_material_extension_entry(
          w, &comma,
          _s_gltf_KHR_materials_dispersion,
          _s_gltf_KHR_materials_dispersion_len);
        break;
      case AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION:
        gltf_write_material_extension_entry(
          w, &comma,
          _s_gltf_KHR_materials_diffuse_transmission,
          _s_gltf_KHR_materials_diffuse_transmission_len);
        break;
      case AK_MATERIAL_FEATURE_SUBSURFACE:
        gltf_write_material_extension_entry(
          w, &comma,
          _s_gltf_KHR_materials_volume_scatter,
          _s_gltf_KHR_materials_volume_scatter_len);
        break;
      default:
        break;
    }
    gltf_write_feature_extension(w, st, prim, inst, feature);
  }

  gltf_write_extra_extension_entries(w,
                                     extra,
                                     gltf_material_core_extension_skip,
                                     NULL,
                                     &comma);
  gltf_w_ch(w, '}');
  *outerComma = true;

  return true;
}

static
void
gltf_write_material(GLTFExpWriter      * __restrict w,
                    GLTFExpState       * __restrict st,
                    AkMaterial         * __restrict material,
                    AkMeshPrimitive    * __restrict prim,
                    AkInstanceGeometry * __restrict inst) {
  AkMaterialSurface *surface;
  bool               comma;

  surface = material ? material->surface : NULL;
  comma   = false;

  gltf_w_ch(w, '{');

  if (material && material->name) {
    gltf_w_key_str(w, _s_gltf_name, _s_gltf_name_len, material->name);
    comma = true;
  }

  gltf_write_pbr(w, st, prim, inst, surface, &comma);
  gltf_write_normal(w, st, prim, inst, surface, &comma);
  gltf_write_occlusion(w, st, prim, inst, surface, &comma);

  gltf_write_emissive(w, st, prim, inst, surface, &comma);
  gltf_write_material_extensions(w,
                                 st,
                                 prim,
                                 inst,
                                 surface,
                                 material ? ak_extra(material) : NULL,
                                 &comma);

  if (ak_materialAlphaBlend(surface) || ak_materialAlphaMask(surface)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_alphaMode, _s_gltf_alphaMode_len);
    if (ak_materialAlphaMask(surface))
      gltf_w_qstr_len(w, _s_gltf_MASK, _s_gltf_MASK_len);
    else
      gltf_w_qstr_len(w, _s_gltf_BLEND, _s_gltf_BLEND_len);
    comma = true;

    if (ak_materialAlphaMask(surface)) {
      gltf_w_ch(w, ',');
      gltf_w_key(w, _s_gltf_alphaCutoff, _s_gltf_alphaCutoff_len);
      gltf_w_float(w, ak_materialAlphaCutoff(surface));
    }
  }

  if (ak_materialDoubleSided(surface)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key_bool(w, _s_gltf_doubleSided, _s_gltf_doubleSided_len, true);
    comma = true;
  }

  if (material && gltf_extra_has_json_extras(ak_extra(material))) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_extras, _s_gltf_extras_len);
    gltf_write_extra_json_extras(w, ak_extra(material));
  }

  gltf_w_ch(w, '}');
}

void
gltf_write_materials(GLTFExpWriter * __restrict w,
                     GLTFExpState  * __restrict st) {
  size_t i;

  if (st->materials.count == 0)
    return;

  gltf_w_key(w, _s_gltf_materials, _s_gltf_materials_len);
  gltf_w_ch(w, '[');

  for (i = 0; i < st->materials.count; i++) {
    GLTFExpMaterialOut *entry;

    entry = &st->materials.items[i];
    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_write_material(w,
                        st,
                        entry->material,
                        entry->primitive,
                        entry->instance);
  }

  gltf_w_ch(w, ']');
}
