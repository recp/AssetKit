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

#include "internal.h"

AK_HIDE
bool
gltf_plan_image(GLTFExpState * __restrict st,
                AkImage      * __restrict image) {
  AkImageSource *source;

  if (!image)
    return false;

  source = gltf_image_source(image);
  if (!source && !image->data)
    return false;

  return gltf_ptrs_add(&st->images, image)
         && gltf_plan_extra_extensions(st, ak_extra(image), NULL, NULL);
}

AK_HIDE
bool
gltf_plan_texture_ref(GLTFExpState * __restrict st,
                      AkTextureRef * __restrict texref) {
  AkTexture *texture;

  texture = texref ? texref->texture : NULL;
  if (!texture)
    return true;

  if (!texture->image
      || !gltf_image_exportable(st, texture->image))
    return true;

  if (!gltf_plan_image(st, texture->image))
    return false;

  if (texture->sampler
      && (!gltf_ptrs_add(&st->samplers, texture->sampler)
          || !gltf_plan_extra_extensions(st,
                                         ak_extra(texture->sampler),
                                         NULL,
                                         NULL)))
    return false;

  return gltf_ptrs_add(&st->textures, texture)
         && gltf_plan_extra_extensions(st,
                                       ak_extra(texture),
                                       gltf_plan_skip_texture_core_extension,
                                       NULL)
         && gltf_plan_extra_extensions(st,
                                       ak_extra(texref),
                                       gltf_plan_skip_texture_info_core_extension,
                                       NULL);
}

AK_HIDE
AkTextureRef*
gltf_plan_texture_ref_if_exportable(GLTFExpState * __restrict st,
                                    AkTextureRef * __restrict texref) {
  AkTexture *texture;

  texture = texref ? texref->texture : NULL;
  if (!texture || !texture->image)
    return NULL;

  return gltf_image_exportable(st, texture->image) ? texref : NULL;
}

static
bool
gltf_plan_material_input_texture_channels_compatible(
  const AkMaterialInput * __restrict input,
  const AkTextureRef    * __restrict texref) {
  AkTextureChannels inputChannels;
  AkTextureChannels texrefChannels;

  inputChannels  = input ? input->channels : AK_TEXTURE_CHANNEL_NONE;
  texrefChannels = texref ? texref->channels : AK_TEXTURE_CHANNEL_NONE;
  return inputChannels == AK_TEXTURE_CHANNEL_NONE
         || texrefChannels == AK_TEXTURE_CHANNEL_NONE
         || (texrefChannels & inputChannels) == inputChannels;
}

static
AkTextureRef*
gltf_plan_material_input_texture_ref_if_exportable(
  GLTFExpState          * __restrict st,
  const AkMaterialInput * __restrict input) {
  AkTextureRef *texref;

  texref = ak_materialInputTexture(input);
  if (!gltf_plan_material_input_texture_channels_compatible(input, texref))
    return NULL;

  return gltf_plan_texture_ref_if_exportable(st, texref);
}

AK_HIDE
bool
gltf_plan_material_input(GLTFExpState          * __restrict st,
                         const AkMaterialInput * __restrict input) {
  AkTextureRef *texref;

  texref = ak_materialInputTexture(input);
  if (!gltf_plan_material_input_texture_channels_compatible(input, texref))
    return true;

  return gltf_plan_texture_ref(st, texref);
}

AK_HIDE
bool
gltf_material_input_empty(const AkMaterialInput * __restrict input) {
  return !input
         || (input->source == AK_MATERIAL_INPUT_NONE
             && input->valueType == AK_MATERIAL_VALUE_NONE
             && !input->texture);
}

AK_HIDE
bool
gltf_plan_material_feature(GLTFExpState      * __restrict st,
                           AkMaterialFeature * __restrict feature) {
  switch (feature->type) {
    case AK_MATERIAL_FEATURE_CLEARCOAT: {
      AkMaterialClearcoatFeature *f;

      f = (AkMaterialClearcoatFeature *)feature;
      return gltf_plan_material_input(st, f->factor)
             && gltf_plan_material_input(st, f->roughness)
             && gltf_plan_material_input(st, f->normal);
    }
    case AK_MATERIAL_FEATURE_SPECULAR: {
      AkMaterialSpecularFeature *f;

      f = (AkMaterialSpecularFeature *)feature;
      return gltf_plan_material_input(st, f->factor)
             && gltf_plan_material_input(st, f->color);
    }
    case AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS: {
      AkMaterialSpecularGlossinessFeature *f;
      AkTextureRef                        *specTex;
      AkTextureRef                        *glossTex;

      f = (AkMaterialSpecularGlossinessFeature *)feature;
      specTex  = gltf_plan_material_input_texture_ref_if_exportable(st,
                                                                    f->specular);
      glossTex = gltf_plan_material_input_texture_ref_if_exportable(st,
                                                                    f->glossiness);
      if (specTex
          && glossTex
          && specTex->texture != glossTex->texture)
        glossTex = NULL;

      return gltf_plan_material_input(st, f->diffuse)
             && (!specTex || gltf_plan_texture_ref(st, specTex))
             && (!glossTex || gltf_plan_texture_ref(st, glossTex));
    }
    case AK_MATERIAL_FEATURE_TRANSMISSION: {
      AkMaterialTransmissionFeature *f;

      f = (AkMaterialTransmissionFeature *)feature;
      return gltf_plan_material_input(st, f->factor);
    }
    case AK_MATERIAL_FEATURE_SHEEN: {
      AkMaterialSheenFeature *f;

      f = (AkMaterialSheenFeature *)feature;
      return gltf_plan_material_input(st, f->color)
             && gltf_plan_material_input(st, f->roughness);
    }
    case AK_MATERIAL_FEATURE_IRIDESCENCE: {
      AkMaterialIridescenceFeature *f;

      f = (AkMaterialIridescenceFeature *)feature;
      return gltf_plan_material_input(st, f->factor)
             && gltf_plan_material_input(st, f->thickness);
    }
    case AK_MATERIAL_FEATURE_VOLUME: {
      AkMaterialVolumeFeature *f;

      f = (AkMaterialVolumeFeature *)feature;
      return gltf_plan_material_input(st, f->thickness);
    }
    case AK_MATERIAL_FEATURE_ANISOTROPY: {
      AkMaterialAnisotropyFeature *f;

      f = (AkMaterialAnisotropyFeature *)feature;
      return gltf_plan_material_input(st, f->strength);
    }
    case AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION: {
      AkMaterialDiffuseTransmissionFeature *f;

      f = (AkMaterialDiffuseTransmissionFeature *)feature;
      return gltf_plan_material_input(st, f->factor)
             && gltf_plan_material_input(st, f->color);
    }
    case AK_MATERIAL_FEATURE_SUBSURFACE: {
      AkMaterialSubsurfaceFeature *f;

      f = (AkMaterialSubsurfaceFeature *)feature;
      return gltf_material_input_empty(f->weight)
             && gltf_material_input_empty(f->radius)
             && !ak_materialInputTexture(f->color);
    }
    default:
      break;
  }

  return true;
}

AK_HIDE
bool
gltf_plan_material_features(GLTFExpState       * __restrict st,
                            AkMaterialSurface  * __restrict surface) {
  AkMaterialFeature *feature;

  for (feature = surface->features; feature; feature = feature->next) {
    if (!gltf_plan_material_feature(st, feature))
      return false;
  }

  return true;
}

AK_HIDE
bool
gltf_plan_material(GLTFExpState * __restrict st,
                   AkMaterial   * __restrict material,
                   AkMeshPrimitive * __restrict prim,
                   AkInstanceGeometry * __restrict inst) {
  AkMaterialSurface *surface;
  AkTextureRef      *baseTex;
  AkTextureRef      *opacityTex;
  AkTextureRef      *metalTex;
  AkTextureRef      *roughTex;

  if (!material || gltf_material_is_default_noop(material))
    return true;

  if (gltf_material_index(st, material, prim, inst) != GLTF_EXP_INDEX_NONE)
    return true;

  if (!gltf_materials_add(&st->materials, material, prim, inst))
    return false;

  if (!gltf_plan_extra_extensions(st,
                                  ak_extra(material),
                                  gltf_plan_skip_material_core_extension,
                                  NULL))
    return false;

  surface = material->surface;
  if (!surface)
    return true;

  baseTex    = gltf_plan_material_input_texture_ref_if_exportable(st,
                                                                  surface->baseColor);
  opacityTex = gltf_plan_material_input_texture_ref_if_exportable(st,
                                                                  surface->opacity);
  metalTex   = gltf_plan_material_input_texture_ref_if_exportable(st,
                                                                  surface->metallic);
  roughTex   = gltf_plan_material_input_texture_ref_if_exportable(st,
                                                                  surface->roughness);

  if (opacityTex
      && (!baseTex || opacityTex->texture != baseTex->texture))
    opacityTex = NULL;

  if (!metalTex
      || !roughTex
      || metalTex->texture != roughTex->texture) {
    metalTex = NULL;
    roughTex = NULL;
  }

  return gltf_plan_material_input(st, surface->baseColor)
         && (!opacityTex || gltf_plan_texture_ref(st, opacityTex))
         && (!metalTex || gltf_plan_texture_ref(st, metalTex))
         && (!roughTex || gltf_plan_texture_ref(st, roughTex))
         && gltf_plan_material_input(st, surface->normal)
         && gltf_plan_material_input(st, surface->occlusion)
         && gltf_plan_material_input(st, surface->emissive)
         && gltf_plan_material_features(st, surface);
}
