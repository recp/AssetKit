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
#include "image.h"

#include <string.h>

static AkTextureRef*
dae_material_base_texture(AkMaterialSurface * __restrict surface);

static AkTextureRef*
dae_material_input_texture(const AkMaterialInput * __restrict input);

static AkMaterial*
dae_resolve_prim_material(AkMeshPrimitive    * __restrict prim,
                          AkInstanceGeometry * __restrict inst);

AK_HIDE
bool
dae_prepare_material_dependencies(DAEExpState * __restrict st,
                                  AkMaterial  * __restrict mat) {
  AkMaterialSurface *surface;
  AkMaterialClassicFeature *classic;
  AkMaterialSpecularFeature *specular;

  if (!mat)
    return true;

  surface = mat->surface;
  classic = (AkMaterialClassicFeature *)ak_materialFeature(
              surface, AK_MATERIAL_FEATURE_CLASSIC);
  specular = (AkMaterialSpecularFeature *)ak_materialFeature(
               surface, AK_MATERIAL_FEATURE_SPECULAR);

  return dae_prepare_texture_image(st, dae_material_base_texture(surface))
         && dae_prepare_texture_image(
              st,
              dae_material_input_texture(surface ? surface->emissive : NULL))
         && dae_prepare_texture_image(
              st,
              dae_material_input_texture(surface ? surface->normal : NULL))
         && dae_prepare_texture_image(
              st,
              dae_material_input_texture(classic ? classic->specular : NULL))
         && dae_prepare_texture_image(
              st,
              dae_material_input_texture(specular ? specular->factor : NULL))
         && dae_prepare_texture_image(
              st,
              dae_material_input_texture(surface ? surface->opacity : NULL))
         && dae_prepare_texture_image(
              st,
              dae_material_input_texture(classic ? classic->transparency : NULL));
}

AK_HIDE
bool
dae_prepare_extra_material(DAEExpState * __restrict st,
                           AkMaterial  * __restrict mat) {
  if (!mat)
    return true;

  if (!dae_prepare_extra_object(st->materials,
                                &st->materialCount,
                                &st->extraMaterials,
                                &st->lastExtraMaterial,
                                mat))
    return false;

  return dae_prepare_material_dependencies(st, mat);
}

AK_HIDE
bool
dae_prepare_instance_materials(DAEExpState        * __restrict st,
                               AkGeometry         * __restrict geom,
                               AkInstanceGeometry * __restrict inst) {
  AkMesh          *mesh;
  AkMeshPrimitive *prim;

  if (!geom || !geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
    return true;

  mesh = ak_objGet(geom->gdata);
  if (!mesh)
    return true;

  for (prim = mesh->primitive; prim; prim = prim->next) {
    if (!dae_prepare_extra_material(st, dae_resolve_prim_material(prim, inst)))
      return false;
  }

  return true;
}

static
AkTextureRef*
dae_material_base_texture(AkMaterialSurface * __restrict surface) {
  return ak_materialInputTexture(surface ? surface->baseColor : NULL);
}

static
void
dae_material_base_color(AkMaterialSurface * __restrict surface,
                        AkColor           * __restrict color) {
  const AkMaterialInput *input;

  color->rgba.R = 1.0f;
  color->rgba.G = 1.0f;
  color->rgba.B = 1.0f;
  color->rgba.A = ak_materialOpacityFactor(surface);

  input = surface ? surface->baseColor : NULL;
  if (!input)
    return;

  switch (input->valueType) {
    case AK_MATERIAL_VALUE_COLOR:
    case AK_MATERIAL_VALUE_FLOAT4:
      color->rgba.R = input->color.rgba.R;
      color->rgba.G = input->color.rgba.G;
      color->rgba.B = input->color.rgba.B;
      color->rgba.A = input->color.rgba.A * ak_materialOpacityFactor(surface);
      break;
    case AK_MATERIAL_VALUE_FLOAT3:
      color->rgba.R = input->value[0];
      color->rgba.G = input->value[1];
      color->rgba.B = input->value[2];
      break;
    default:
      break;
  }
}

static
bool
dae_material_input_color(const AkMaterialInput * __restrict input,
                         float                              strength,
                         AkColor             * __restrict  color) {
  if (!input)
    return false;

  color->rgba.R = 1.0f;
  color->rgba.G = 1.0f;
  color->rgba.B = 1.0f;
  color->rgba.A = 1.0f;

  switch (input->valueType) {
    case AK_MATERIAL_VALUE_COLOR:
    case AK_MATERIAL_VALUE_FLOAT4:
      color->rgba.R = input->color.rgba.R * strength;
      color->rgba.G = input->color.rgba.G * strength;
      color->rgba.B = input->color.rgba.B * strength;
      color->rgba.A = input->color.rgba.A;
      return true;
    case AK_MATERIAL_VALUE_FLOAT3:
      color->rgba.R = input->value[0] * strength;
      color->rgba.G = input->value[1] * strength;
      color->rgba.B = input->value[2] * strength;
      return true;
    case AK_MATERIAL_VALUE_FLOAT:
      color->rgba.R = input->value[0] * strength;
      color->rgba.G = input->value[0] * strength;
      color->rgba.B = input->value[0] * strength;
      return true;
    default:
      return false;
  }
}

static
void
dae_write_color_value(DAEExpWriter * __restrict w,
                      AkColor      * __restrict color) {
  dae_w_lit(w, "<color>");
  dae_w_float_fast(w, color->rgba.R);
  dae_w_ch(w, ' ');
  dae_w_float_fast(w, color->rgba.G);
  dae_w_ch(w, ' ');
  dae_w_float_fast(w, color->rgba.B);
  dae_w_ch(w, ' ');
  dae_w_float_fast(w, color->rgba.A);
  dae_w_lit(w, "</color>");
}

static
void
dae_write_material_color_tag(DAEExpWriter * __restrict w,
                             DAEExpName                 tag,
                             AkColor      * __restrict  color) {
  dae_w_ch(w, '<');
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
  dae_write_color_value(w, color);
  dae_w_lit(w, "</");
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
}

static
float
dae_clamp01(float v) {
  if (v < 0.0f)
    return 0.0f;
  if (v > 1.0f)
    return 1.0f;
  return v;
}

static
bool
dae_material_pbr_like(AkMaterialSurface * __restrict surface) {
  if (!surface)
    return false;

  switch (surface->type) {
    case AK_MATERIAL_TYPE_PBR:
    case AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS:
    case AK_MATERIAL_TYPE_PBR_SPECULAR_GLOSSINESS:
      return true;
    case AK_MATERIAL_TYPE_NONE:
      return surface->metallic || surface->roughness;
    default:
      return false;
  }
}

static
float
dae_material_roughness_approx(AkMaterialSurface * __restrict surface) {
  float roughness;

  roughness = dae_clamp01(ak_materialRoughnessFactor(surface));
  if (surface
      && surface->roughness
      && surface->roughness->texture
      && roughness > 0.75f) {
    /* COLLADA common profile cannot carry metallic-roughness channel
       packing. Bias texture-only roughness slightly glossy so PBR assets do
       not collapse into a flat matte classic material. */
    roughness = 0.35f;
  }

  return roughness;
}

static
float
dae_material_metallic_approx(AkMaterialSurface * __restrict surface) {
  return dae_clamp01(ak_materialMetallicFactor(surface));
}

static
bool
dae_material_pbr_matte_lambert(AkMaterialSurface * __restrict surface) {
  if (!dae_material_pbr_like(surface))
    return false;

  if (surface->type == AK_MATERIAL_TYPE_PBR_SPECULAR_GLOSSINESS)
    return false;

  if (ak_materialInputTexture(surface->metallic)
      || ak_materialInputTexture(surface->roughness))
    return false;

  if (ak_materialFeature(surface, AK_MATERIAL_FEATURE_SPECULAR)
      || ak_materialFeature(surface, AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS))
    return false;

  return dae_material_metallic_approx(surface) <= 0.001f
         && dae_material_roughness_approx(surface) >= 0.999f;
}

static
bool
dae_material_pbr_specular_color(AkMaterialSurface * __restrict surface,
                                AkColor           * __restrict color) {
  AkMaterialSpecularFeature *specular;
  AkColor                   base;
  float                     metallic;
  float                     dielectric;

  if (!dae_material_pbr_like(surface))
    return false;

  specular = (AkMaterialSpecularFeature *)ak_materialFeature(
               surface, AK_MATERIAL_FEATURE_SPECULAR);
  if (specular && specular->color
      && dae_material_input_color(specular->color, 1.0f, color)) {
    float factor;

    factor = specular->factor
             ? dae_clamp01(ak_materialInputScalar(specular->factor, 1.0f))
             : 1.0f;
    color->rgba.R *= factor;
    color->rgba.G *= factor;
    color->rgba.B *= factor;
    color->rgba.A = 1.0f;
    return true;
  }

  dae_material_base_color(surface, &base);
  metallic  = dae_material_metallic_approx(surface);
  dielectric = 0.5f * (1.0f - metallic);

  color->rgba.R = dielectric + base.rgba.R * metallic;
  color->rgba.G = dielectric + base.rgba.G * metallic;
  color->rgba.B = dielectric + base.rgba.B * metallic;
  color->rgba.A = 1.0f;
  return true;
}

static
float
dae_material_pbr_shininess(AkMaterialSurface * __restrict surface) {
  float roughness;
  float shininess;

  roughness = dae_material_roughness_approx(surface);
  if (roughness <= 0.045f)
    return 1024.0f;

  shininess = 2.0f / (roughness * roughness) - 2.0f;
  if (shininess < 0.0f)
    return 0.0f;
  if (shininess > 1024.0f)
    return 1024.0f;
  return shininess;
}

static
float
dae_material_pbr_reflectivity(AkMaterialSurface * __restrict surface) {
  float metallic;
  float roughness;

  metallic  = dae_material_metallic_approx(surface);
  roughness = dae_material_roughness_approx(surface);
  return dae_clamp01((0.04f + 0.96f * metallic) * (1.0f - roughness));
}

static
bool
dae_material_ior(AkMaterialSurface        * __restrict surface,
                 AkMaterialClassicFeature * __restrict classic,
                 float                    * __restrict ior) {
  if (classic && classic->ior > 0.0f) {
    *ior = classic->ior;
    return true;
  }

  if (surface && (surface->flags & AK_MATERIAL_FLAG_HAS_IOR)) {
    *ior = ak_materialIor(surface);
    return *ior > 0.0f;
  }

  return false;
}

static
void
dae_write_material_float_tag(DAEExpWriter * __restrict w,
                             DAEExpName                 tag,
                             float                      value) {
  dae_w_ch(w, '<');
  dae_w_name(w, tag);
  dae_w_lit(w, "><float>");
  dae_w_float_fast(w, value);
  dae_w_lit(w, "</float></");
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
}

static
uint32_t
dae_texture_image_index(DAEExpState * __restrict st,
                        AkTextureRef * __restrict texref) {
  AkTexture *tex;

  tex = texref ? texref->texture : NULL;
  return tex && tex->image
         ? dae_map_index(st->images, tex->image)
         : UINT32_MAX;
}

static
bool
dae_texture_ref_mapped(DAEExpState * __restrict st,
                       AkTextureRef * __restrict texref) {
  AkTexture *tex;

  if (!texref)
    return true;

  tex = texref->texture;
  if (!tex || !tex->image)
    return true;

  return dae_map_index(st->images, tex->image) != UINT32_MAX;
}

static
AkTextureRef*
dae_material_input_texture(const AkMaterialInput * __restrict input) {
  return ak_materialInputTexture(input);
}

static
int32_t
dae_texcoord_slot(AkTextureRef * __restrict texref,
                  int32_t                   slotOverride) {
  int32_t slot;

  slot = slotOverride >= 0 ? slotOverride : (texref ? texref->slot : 0);
  if (texref && texref->transform && texref->transform->slot >= 0)
    slot = texref->transform->slot;
  return slot >= 0 ? slot : 0;
}

static
void
dae_w_texcoord(DAEExpWriter * __restrict w,
               AkTextureRef * __restrict texref,
               int32_t                   slotOverride) {
  int32_t slot;

  if (texref && texref->texcoord && texref->texcoord[0]) {
    dae_w_xml(w, texref->texcoord, true);
    return;
  }

  slot = dae_texcoord_slot(texref, slotOverride);
  dae_w_name(w, DAE_EXP_NAME(TEXCOORD));
  dae_w_uint_fast(w, (uint32_t)slot);
}

static
void
dae_write_sampler_value(DAEExpWriter * __restrict w,
                        DAEExpName                tag,
                        DAEExpName                val) {
  dae_w_ch(w, '<');
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
  dae_w_name(w, val);
  dae_w_lit(w, "</");
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
}

static
bool
dae_wrap_name(AkWrapMode wrap, DAEExpName * __restrict name) {
  switch (wrap) {
    case AK_WRAP_MODE_WRAP:        *name = DAE_EXP_NAME(WRAP); return true;
    case AK_WRAP_MODE_CLAMP:       *name = DAE_EXP_NAME(CLAMP); return true;
    case AK_WRAP_MODE_BORDER:      *name = DAE_EXP_NAME(BORDER); return true;
    case AK_WRAP_MODE_MIRROR:      *name = DAE_EXP_NAME(MIRROR); return true;
    case AK_WRAP_MODE_MIRROR_ONCE: *name = DAE_EXP_NAME(MIRROR_ONCE); return true;
    default:                       return false;
  }
}

static
bool
dae_min_filter_name(AkMinFilter filter, DAEExpName * __restrict name) {
  switch (filter) {
    case AK_MINFILTER_NONE:        *name = DAE_EXP_NAME(NONE); return true;
    case AK_MINFILTER_LINEAR:      *name = DAE_EXP_NAME(LINEAR); return true;
    case AK_MINFILTER_NEAREST:     *name = DAE_EXP_NAME(NEAREST); return true;
    case AK_MINFILTER_NEAREST_MIPMAP_NEAREST:
      *name = DAE_EXP_NAME(NEAREST_MIPMAP_NEAREST); return true;
    case AK_MINFILTER_LINEAR_MIPMAP_NEAREST:
      *name = DAE_EXP_NAME(LINEAR_MIPMAP_NEAREST); return true;
    case AK_MINFILTER_NEAREST_MIPMAP_LINEAR:
      *name = DAE_EXP_NAME(NEAREST_MIPMAP_LINEAR); return true;
    case AK_MINFILTER_LINEAR_MIPMAP_LINEAR:
      *name = DAE_EXP_NAME(LINEAR_MIPMAP_LINEAR); return true;
    case AK_MINFILTER_ANISOTROPIC: *name = DAE_EXP_NAME(ANISOTROPIC); return true;
    default:                      return false;
  }
}

static
bool
dae_mag_filter_name(AkMagFilter filter, DAEExpName * __restrict name) {
  switch (filter) {
    case AK_MAGFILTER_NONE:    *name = DAE_EXP_NAME(NONE); return true;
    case AK_MAGFILTER_LINEAR:  *name = DAE_EXP_NAME(LINEAR); return true;
    case AK_MAGFILTER_NEAREST: *name = DAE_EXP_NAME(NEAREST); return true;
    default:                   return false;
  }
}

static
bool
dae_mip_filter_name(AkMipFilter filter, DAEExpName * __restrict name) {
  switch (filter) {
    case AK_MIPFILTER_LINEAR:  *name = DAE_EXP_NAME(LINEAR); return true;
    case AK_MIPFILTER_NONE:    *name = DAE_EXP_NAME(NONE); return true;
    case AK_MIPFILTER_NEAREST: *name = DAE_EXP_NAME(NEAREST); return true;
    default:                   return false;
  }
}

static
void
dae_write_sampler_state(DAEExpWriter * __restrict w,
                        AkSampler    * __restrict sampler) {
  DAEExpName val;

  if (!sampler)
    return;

  if (dae_wrap_name(sampler->wrapS, &val))
    dae_write_sampler_value(w, DAE_EXP_NAME(wrap_s), val);
  if (dae_wrap_name(sampler->wrapT, &val))
    dae_write_sampler_value(w, DAE_EXP_NAME(wrap_t), val);
  if (dae_wrap_name(sampler->wrapP, &val))
    dae_write_sampler_value(w, DAE_EXP_NAME(wrap_p), val);
  if (dae_min_filter_name(sampler->minfilter, &val))
    dae_write_sampler_value(w, DAE_EXP_NAME(minfilter), val);
  if (dae_mag_filter_name(sampler->magfilter, &val))
    dae_write_sampler_value(w, DAE_EXP_NAME(magfilter), val);
  if (dae_mip_filter_name(sampler->mipfilter, &val))
    dae_write_sampler_value(w, DAE_EXP_NAME(mipfilter), val);
}

static
void
dae_write_texture_newparam(DAEExpState * __restrict st,
                           uint32_t                 matIdx,
                           DAEExpName               suffix,
                           AkTextureRef * __restrict texref) {
  DAEExpWriter *w;
  uint32_t      imageIdx;

  imageIdx = dae_texture_image_index(st, texref);
  if (imageIdx == UINT32_MAX)
    return;

  w = &st->w;
  if (st->useCollada150) {
    dae_w_lit(w, "<newparam sid=\"sampler_");
    dae_w_uint_fast(w, matIdx);
    dae_w_name(w, suffix);
    dae_w_lit(w, "\"><sampler2D><instance_image url=\"#");
    dae_w_id(w, DAE_EXP_NAME(image), imageIdx);
    dae_w_lit(w, "\"/>");
    dae_write_sampler_state(w,
                            texref && texref->texture
                              ? texref->texture->sampler
                              : NULL);
    dae_w_lit(w, "</sampler2D></newparam>");
    return;
  }

  dae_w_lit(w, "<newparam sid=\"surface_");
  dae_w_uint_fast(w, matIdx);
  dae_w_name(w, suffix);
  dae_w_lit(w, "\"><surface type=\"2D\"><init_from>");
  dae_w_id(w, DAE_EXP_NAME(image), imageIdx);
  dae_w_lit(w, "</init_from></surface></newparam>");
  dae_w_lit(w, "<newparam sid=\"sampler_");
  dae_w_uint_fast(w, matIdx);
  dae_w_name(w, suffix);
  dae_w_lit(w, "\"><sampler2D><source>surface_");
  dae_w_uint_fast(w, matIdx);
  dae_w_name(w, suffix);
  dae_w_lit(w, "</source>");
  dae_write_sampler_state(w,
                          texref && texref->texture
                            ? texref->texture->sampler
                            : NULL);
  dae_w_lit(w, "</sampler2D></newparam>");
}

static
void
dae_write_vendor_float_tag(DAEExpWriter * __restrict w,
                           DAEExpName                 tag,
                           float                      value) {
  dae_w_ch(w, '<');
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
  dae_w_float_fast(w, value);
  dae_w_lit(w, "</");
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
}

static
bool
dae_texture_has_vendor_extra(AkTextureRef * __restrict texref,
                             bool                       forceWeight) {
  return texref
         && (forceWeight
             || texref->transform);
}

static
void
dae_write_texture_vendor_extra(DAEExpWriter * __restrict w,
                               AkTextureRef * __restrict texref,
                               bool                       forceWeight,
                               float                      weight) {
  AkTextureTransform *transform;

  if (!dae_texture_has_vendor_extra(texref, forceWeight))
    return;

  transform = texref->transform;
  dae_w_lit(w, "<extra>");
  if (transform) {
    dae_w_lit(w, "<technique profile=\"MAYA\">");
    dae_write_vendor_float_tag(w, DAE_EXP_NAME_LIT("repeatU"),
                               transform->scale[0]);
    dae_write_vendor_float_tag(w, DAE_EXP_NAME_LIT("repeatV"),
                               transform->scale[1]);
    dae_write_vendor_float_tag(w, DAE_EXP_NAME_LIT("offsetU"),
                               transform->offset[0]);
    dae_write_vendor_float_tag(w, DAE_EXP_NAME_LIT("offsetV"),
                               transform->offset[1]);
    dae_write_vendor_float_tag(w, DAE_EXP_NAME_LIT("rotateUV"),
                               transform->rotation);
    dae_w_lit(w, "</technique>");
  }
  if (forceWeight) {
    dae_w_lit(w, "<technique profile=\"MAX3D\"><amount>");
    dae_w_float_fast(w, weight);
    dae_w_lit(w, "</amount></technique>");
  }
  dae_w_lit(w, "</extra>");
}

static
void
dae_write_texture_value_weight(DAEExpWriter * __restrict w,
                               uint32_t                  matIdx,
                               DAEExpName                suffix,
                               AkTextureRef * __restrict texref,
                               bool                      forceWeight,
                               float                     weight) {
  dae_w_lit(w, "<texture texture=\"sampler_");
  dae_w_uint_fast(w, matIdx);
  dae_w_name(w, suffix);
  dae_w_lit(w, "\" texcoord=\"");
  dae_w_texcoord(w, texref, -1);
  dae_w_ch(w, '"');
  if (!dae_texture_has_vendor_extra(texref, forceWeight)) {
    dae_w_lit(w, "/>");
    return;
  }
  dae_w_ch(w, '>');
  dae_write_texture_vendor_extra(w, texref, forceWeight, weight);
  dae_w_lit(w, "</texture>");
}

static
void
dae_write_texture_value(DAEExpWriter * __restrict w,
                        uint32_t                  matIdx,
                        DAEExpName                suffix,
                        AkTextureRef * __restrict texref) {
  dae_write_texture_value_weight(w, matIdx, suffix, texref, false, 0.0f);
}

static
void
dae_write_material_texture_tag(DAEExpWriter * __restrict w,
                               DAEExpName                tag,
                               uint32_t                  matIdx,
                               DAEExpName                suffix,
                               AkTextureRef * __restrict texref) {
  dae_w_ch(w, '<');
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
  dae_write_texture_value(w, matIdx, suffix, texref);
  dae_w_lit(w, "</");
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
}

static
DAEExpName
dae_material_technique_tag(AkMaterialSurface * __restrict surface) {
  if (!surface)
    return DAE_EXP_NAME(phong);

  if (dae_material_pbr_matte_lambert(surface))
    return DAE_EXP_NAME(lambert);

  switch (surface->type) {
    case AK_MATERIAL_TYPE_CONSTANT: return DAE_EXP_NAME(constant);
    case AK_MATERIAL_TYPE_LAMBERT:  return DAE_EXP_NAME(lambert);
    case AK_MATERIAL_TYPE_BLINN:    return DAE_EXP_NAME(blinn);
    case AK_MATERIAL_TYPE_PHONG:    return DAE_EXP_NAME(phong);
    default:                        return DAE_EXP_NAME(phong);
  }
}

AK_HIDE
void
dae_write_effect(DAEExpState * __restrict st,
                 AkMaterial  * __restrict mat,
                 uint32_t                 matIdx) {
  DAEExpWriter      *w;
  AkMaterialSurface *surface;
  AkMaterialClassicFeature *classic;
  AkMaterialSpecularFeature *specular;
  AkTextureRef      *baseTex;
  AkTextureRef      *emissiveTex;
  AkTextureRef      *normalTex;
  AkTextureRef      *specularTex;
  AkTextureRef      *specularLevelTex;
  AkTextureRef      *opacityTex;
  AkTextureRef      *transparentTex;
  AkTextureRef      *alphaTex;
  DAEExpName         techniqueTag;
  AkColor            color;
  float              opacity;
  float              ior;
  bool               useConstant;
  bool               useLambert;
  bool               useSpecular;
  bool               wroteSpecular;

  w        = &st->w;
  surface  = mat ? mat->surface : NULL;
  classic  = (AkMaterialClassicFeature *)ak_materialFeature(surface,
                                                            AK_MATERIAL_FEATURE_CLASSIC);
  specular = (AkMaterialSpecularFeature *)ak_materialFeature(
               surface, AK_MATERIAL_FEATURE_SPECULAR);
  baseTex  = dae_material_base_texture(surface);
  emissiveTex = dae_material_input_texture(surface ? surface->emissive : NULL);
  normalTex = dae_material_input_texture(surface ? surface->normal : NULL);
  specularTex = dae_material_input_texture(classic ? classic->specular : NULL);
  specularLevelTex =
    dae_material_input_texture(specular ? specular->factor : NULL);
  opacityTex  = dae_material_input_texture(surface ? surface->opacity : NULL);
  transparentTex =
    dae_material_input_texture(classic ? classic->transparency : NULL);
  alphaTex = opacityTex ? opacityTex : transparentTex;
  if (!alphaTex
      && baseTex
      && (ak_materialAlphaBlend(surface) || ak_materialAlphaMask(surface)))
    alphaTex = baseTex;
  opacity  = ak_materialOpacityFactor(surface);

  if (!dae_texture_ref_mapped(st, baseTex)
      || !dae_texture_ref_mapped(st, emissiveTex)
      || !dae_texture_ref_mapped(st, normalTex)
      || !dae_texture_ref_mapped(st, specularTex)
      || !dae_texture_ref_mapped(st, specularLevelTex)
      || !dae_texture_ref_mapped(st, opacityTex)
      || !dae_texture_ref_mapped(st, transparentTex)) {
    w->result = AK_EINVAL;
    return;
  }

  dae_w_lit(w, "<effect id=\"");
  dae_w_id(w, DAE_EXP_NAME(effect), matIdx);
  dae_w_lit(w, "\"><profile_COMMON>");

  dae_write_texture_newparam(st, matIdx, DAE_EXP_NAME_LIT(""), baseTex);
  dae_write_texture_newparam(st, matIdx, DAE_EXP_NAME_LIT("_emission"), emissiveTex);
  dae_write_texture_newparam(st, matIdx, DAE_EXP_NAME_LIT("_normal"), normalTex);
  dae_write_texture_newparam(st, matIdx, DAE_EXP_NAME_LIT("_specular"), specularTex);
  dae_write_texture_newparam(st, matIdx, DAE_EXP_NAME_LIT("_specular_level"),
                             specularLevelTex);
  dae_write_texture_newparam(st, matIdx, DAE_EXP_NAME_LIT("_transparent"),
                             alphaTex);

  techniqueTag = dae_material_technique_tag(surface);
  useConstant  = surface && surface->type == AK_MATERIAL_TYPE_CONSTANT;
  useLambert   = surface && (surface->type == AK_MATERIAL_TYPE_LAMBERT
                             || dae_material_pbr_matte_lambert(surface));
  useSpecular  = !surface
                 || (surface->type != AK_MATERIAL_TYPE_CONSTANT
                     && !useLambert);
  dae_w_lit(w, "<technique sid=\"common\"><");
  dae_w_name(w, techniqueTag);
  dae_w_ch(w, '>');

  if (useConstant) {
    dae_w_lit(w, "<emission>");
    if (dae_texture_image_index(st, baseTex) != UINT32_MAX) {
      dae_write_texture_value(w, matIdx, DAE_EXP_NAME_LIT(""), baseTex);
    } else {
      dae_material_base_color(surface, &color);
      dae_write_color_value(w, &color);
    }
    dae_w_lit(w, "</emission>");
  } else if (dae_texture_image_index(st, emissiveTex) != UINT32_MAX) {
    dae_write_material_texture_tag(w, DAE_EXP_NAME(emission), matIdx,
                                   DAE_EXP_NAME_LIT("_emission"), emissiveTex);
  } else if (dae_material_input_color(surface ? surface->emissive : NULL,
                                      ak_materialEmissiveStrength(surface),
                                      &color)) {
    dae_write_material_color_tag(w, DAE_EXP_NAME(emission), &color);
  }

  if (!useConstant) {
    dae_w_lit(w, "<diffuse>");
    if (dae_texture_image_index(st, baseTex) != UINT32_MAX) {
      dae_write_texture_value(w, matIdx, DAE_EXP_NAME_LIT(""), baseTex);
    } else {
      dae_material_base_color(surface, &color);
      dae_write_color_value(w, &color);
    }
    dae_w_lit(w, "</diffuse>");
  }

  wroteSpecular = false;
  if (useSpecular && dae_texture_image_index(st, specularTex) != UINT32_MAX) {
    dae_write_material_texture_tag(w, DAE_EXP_NAME(specular), matIdx,
                                   DAE_EXP_NAME_LIT("_specular"), specularTex);
    wroteSpecular = true;
  } else if (useSpecular
             && classic
             && dae_material_input_color(classic->specular, 1.0f, &color)) {
    dae_write_material_color_tag(w, DAE_EXP_NAME(specular), &color);
    wroteSpecular = true;
  } else if (useSpecular
             && dae_material_pbr_specular_color(surface, &color)) {
    dae_write_material_color_tag(w, DAE_EXP_NAME(specular), &color);
    wroteSpecular = true;
  }

  if (useSpecular && classic && classic->shininess > 0.0f) {
    dae_write_material_float_tag(w, DAE_EXP_NAME(shininess), classic->shininess);
  } else if (wroteSpecular && dae_material_pbr_like(surface)) {
    dae_write_material_float_tag(w,
                                 DAE_EXP_NAME(shininess),
                                 dae_material_pbr_shininess(surface));
  }

  if (useSpecular && classic && classic->reflective
      && dae_material_input_color(classic->reflective, 1.0f, &color)) {
    dae_write_material_color_tag(w, DAE_EXP_NAME(reflective), &color);
    if (classic->reflectivity > 0.0f) {
      dae_write_material_float_tag(w,
                                   DAE_EXP_NAME(reflectivity),
                                   classic->reflectivity);
    }
  } else if (wroteSpecular && dae_material_pbr_like(surface)) {
    dae_write_material_color_tag(w, DAE_EXP_NAME(reflective), &color);
    dae_write_material_float_tag(w,
                                 DAE_EXP_NAME(reflectivity),
                                 dae_material_pbr_reflectivity(surface));
  }

  if (dae_material_ior(surface, classic, &ior))
    dae_write_material_float_tag(w, DAE_EXP_NAME(index_of_refraction), ior);

  if (dae_texture_image_index(st, alphaTex) != UINT32_MAX) {
    dae_w_lit(w, "<transparent opaque=\"A_ONE\">");
    dae_write_texture_value(w, matIdx, DAE_EXP_NAME_LIT("_transparent"),
                            alphaTex);
    dae_w_lit(w, "</transparent><transparency><float>");
    dae_w_float_fast(w, opacity);
    dae_w_lit(w, "</float></transparency>");
  } else if (opacity < 0.999f
             || ak_materialAlphaBlend(surface)
             || ak_materialAlphaMask(surface)) {
    color.rgba.R = 1.0f;
    color.rgba.G = 1.0f;
    color.rgba.B = 1.0f;
    color.rgba.A = opacity;
    dae_w_lit(w, "<transparent opaque=\"A_ONE\">");
    dae_write_color_value(w, &color);
    dae_w_lit(w, "</transparent><transparency><float>1</float></transparency>");
  }

  dae_w_lit(w, "</");
  dae_w_name(w, techniqueTag);
  dae_w_ch(w, '>');

  if (dae_texture_image_index(st, normalTex) != UINT32_MAX
      || dae_texture_image_index(st, specularLevelTex) != UINT32_MAX) {
    dae_w_lit(w, "<extra><technique profile=\"OpenCOLLADA3dsMax\">");
    if (dae_texture_image_index(st, specularLevelTex) != UINT32_MAX) {
      dae_w_lit(w, "<specularLevel>");
      dae_write_texture_value_weight(
        w, matIdx, DAE_EXP_NAME_LIT("_specular_level"), specularLevelTex,
        specular && specular->factor, specular && specular->factor
          ? specular->factor->value[0] : 1.0f);
      dae_w_lit(w, "</specularLevel>");
    }
    if (dae_texture_image_index(st, normalTex) != UINT32_MAX) {
      dae_w_lit(w, "<bump bumptype=\"");
      if (surface && surface->normal
          && (surface->normal->flags & AK_MATERIAL_INPUT_FLAG_HEIGHT))
        dae_w_lit(w, "HEIGHTFIELD");
      else
        dae_w_lit(w, "NORMALMAP");
      dae_w_lit(w, "\">");
      dae_write_texture_value_weight(
        w, matIdx, DAE_EXP_NAME_LIT("_normal"), normalTex,
        surface && surface->normal, surface && surface->normal
          ? surface->normal->value[0] : 1.0f);
      dae_w_lit(w, "</bump>");
    }
    dae_w_lit(w, "</technique></extra>");
  }

  dae_w_lit(w, "</technique></profile_COMMON>");
  if (surface && (surface->flags & AK_MATERIAL_FLAG_DOUBLE_SIDED)) {
    dae_w_lit(w, "<extra><technique profile=\"MAX3D\">");
    dae_w_lit(w, "<double_sided>1</double_sided>");
    dae_w_lit(w, "</technique></extra>");
  }
  dae_w_lit(w, "</effect>");
}

AK_HIDE
void
dae_write_material(DAEExpState * __restrict st,
                   AkMaterial  * __restrict mat,
                   uint32_t                 matIdx) {
  DAEExpWriter *w;

  w = &st->w;
  dae_w_lit(w, "<material id=\"");
  dae_w_id(w, DAE_EXP_NAME(material), matIdx);
  if (mat && mat->name) {
    dae_w_lit(w, "\" name=\"");
    dae_w_xml(w, mat->name, true);
  }
  dae_w_lit(w, "\"><instance_effect url=\"#");
  dae_w_id(w, DAE_EXP_NAME(effect), matIdx);
  dae_w_lit(w, "\"/>");
  dae_write_extra(w, mat ? mat->extra : NULL);
  dae_w_lit(w, "</material>");
}

static
AkMaterial*
dae_resolve_prim_material(AkMeshPrimitive    * __restrict prim,
                          AkInstanceGeometry * __restrict inst) {
  AkResolvedMaterial resolved;

  memset(&resolved, 0, sizeof(resolved));
  if (ak_materialResolve(prim, inst, UINT32_MAX, &resolved))
    return resolved.material;

  return prim ? prim->material : NULL;
}

static
bool
dae_instance_texcoord_slot_seen(const int32_t * __restrict slots,
                                uint32_t                   slotCount,
                                int32_t                    slot) {
  uint32_t i;

  for (i = 0; i < slotCount; i++) {
    if (slots[i] == slot)
      return true;
  }

  return false;
}

static
uint32_t
dae_write_instance_texcoord_binding(DAEExpState        * __restrict st,
                                    AkMeshPrimitive    * __restrict prim,
                                    AkInstanceGeometry * __restrict inst,
                                    AkTextureRef       * __restrict texref,
                                    int32_t            * __restrict slots,
                                    uint32_t                         slotCount) {
  DAEExpWriter *w;
  int32_t       slot;

  if (!texref || dae_texture_image_index(st, texref) == UINT32_MAX)
    return slotCount;

  slot = ak_materialTextureSlot(prim, inst, texref);
  if (!texref->texcoord
      && dae_instance_texcoord_slot_seen(slots, slotCount, slot))
    return slotCount;

  w = &st->w;
  dae_w_lit(w, "<bind_vertex_input semantic=\"");
  dae_w_texcoord(w, texref, slot);
  dae_w_lit(w, "\" input_semantic=\"TEXCOORD\" input_set=\"");
  dae_w_uint_fast(w, (uint32_t)dae_texcoord_slot(texref, slot));
  dae_w_lit(w, "\"/>");

  if (!texref->texcoord && slotCount < 8u)
    slots[slotCount++] = slot;

  return slotCount;
}

AK_HIDE
void
dae_write_instance_materials(DAEExpState        * __restrict st,
                             AkGeometry         * __restrict geom,
                             AkInstanceGeometry * __restrict inst) {
  DAEExpWriter    *w;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  uint32_t         primIdx;
  bool             any;

  if (!geom || !geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
    return;

  mesh = ak_objGet(geom->gdata);
  if (!mesh || !mesh->primitive)
    return;

  w   = &st->w;
  any = false;
  for (prim = mesh->primitive, primIdx = 0;
       prim;
       prim = prim->next, primIdx++) {
    AkMaterial *mat;
    uint32_t    matIdx;

    mat    = dae_resolve_prim_material(prim, inst);
    matIdx = mat ? dae_map_index(st->materials, mat) : UINT32_MAX;
    if (matIdx == UINT32_MAX)
      continue;

    if (!any) {
      dae_w_lit(w, "<bind_material><technique_common>");
      any = true;
    }

    dae_w_lit(w, "<instance_material symbol=\"");
    dae_w_prim_material_symbol(w, primIdx);
    dae_w_lit(w, "\" target=\"#");
    dae_w_id(w, DAE_EXP_NAME(material), matIdx);
    dae_w_lit(w, "\">");

    {
      AkMaterialSurface *surface;
      AkMaterialClassicFeature *classic;
      AkMaterialSpecularFeature *specular;
      int32_t       slots[8];
      uint32_t      slotCount;

      surface   = mat->surface;
      classic   = (AkMaterialClassicFeature *)ak_materialFeature(
                    surface, AK_MATERIAL_FEATURE_CLASSIC);
      specular  = (AkMaterialSpecularFeature *)ak_materialFeature(
                    surface, AK_MATERIAL_FEATURE_SPECULAR);
      slotCount = 0;
      slotCount = dae_write_instance_texcoord_binding(
                    st, prim, inst, dae_material_base_texture(surface),
                    slots, slotCount);
      slotCount = dae_write_instance_texcoord_binding(
                    st, prim, inst,
                    dae_material_input_texture(surface ? surface->emissive : NULL),
                    slots, slotCount);
      slotCount = dae_write_instance_texcoord_binding(
                    st, prim, inst,
                    dae_material_input_texture(surface ? surface->normal : NULL),
                    slots, slotCount);
      slotCount = dae_write_instance_texcoord_binding(
                    st, prim, inst,
                    dae_material_input_texture(classic ? classic->specular : NULL),
                    slots, slotCount);
      slotCount = dae_write_instance_texcoord_binding(
                    st, prim, inst,
                    dae_material_input_texture(specular ? specular->factor : NULL),
                    slots, slotCount);
      slotCount = dae_write_instance_texcoord_binding(
                    st, prim, inst,
                    dae_material_input_texture(surface ? surface->opacity : NULL),
                    slots, slotCount);
      (void)dae_write_instance_texcoord_binding(
        st, prim, inst,
        dae_material_input_texture(classic ? classic->transparency : NULL),
        slots, slotCount);
    }

    dae_w_lit(w, "</instance_material>");
  }

  if (any)
    dae_w_lit(w, "</technique_common></bind_material>");
}
