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

#include "anim.h"
#include "../strpool.h"

#define GLTF_EXP_TEX_XFORM_OFFSET   0u
#define GLTF_EXP_TEX_XFORM_SCALE    1u
#define GLTF_EXP_TEX_XFORM_ROTATION 2u

#define GLTF_EXP_TEX_ROLE_BASE_COLOR                  0u
#define GLTF_EXP_TEX_ROLE_METALLIC_ROUGHNESS          1u
#define GLTF_EXP_TEX_ROLE_OCCLUSION                   2u
#define GLTF_EXP_TEX_ROLE_NORMAL                      3u
#define GLTF_EXP_TEX_ROLE_EMISSIVE                    4u
#define GLTF_EXP_TEX_ROLE_SPECULAR                    6u
#define GLTF_EXP_TEX_ROLE_SPECULAR_COLOR              7u
#define GLTF_EXP_TEX_ROLE_CLEARCOAT                   8u
#define GLTF_EXP_TEX_ROLE_CLEARCOAT_ROUGHNESS         9u
#define GLTF_EXP_TEX_ROLE_CLEARCOAT_NORMAL           10u
#define GLTF_EXP_TEX_ROLE_TRANSMISSION               11u
#define GLTF_EXP_TEX_ROLE_SHEEN_COLOR                12u
#define GLTF_EXP_TEX_ROLE_SHEEN_ROUGHNESS            13u
#define GLTF_EXP_TEX_ROLE_IRIDESCENCE                14u
#define GLTF_EXP_TEX_ROLE_IRIDESCENCE_THICKNESS      15u
#define GLTF_EXP_TEX_ROLE_VOLUME_THICKNESS           16u
#define GLTF_EXP_TEX_ROLE_ANISOTROPY                 17u
#define GLTF_EXP_TEX_ROLE_DIFFUSE_TRANSMISSION       18u
#define GLTF_EXP_TEX_ROLE_DIFFUSE_TRANSMISSION_COLOR 19u

#define GLTF_EXP_MAT_PTR_BASE_COLOR                   1u
#define GLTF_EXP_MAT_PTR_METALLIC                     2u
#define GLTF_EXP_MAT_PTR_ROUGHNESS                    3u
#define GLTF_EXP_MAT_PTR_ALPHA_CUTOFF                 4u
#define GLTF_EXP_MAT_PTR_EMISSIVE_COLOR               5u
#define GLTF_EXP_MAT_PTR_EMISSIVE_STRENGTH            6u
#define GLTF_EXP_MAT_PTR_NORMAL_SCALE                 7u
#define GLTF_EXP_MAT_PTR_OCCLUSION_STRENGTH           8u
#define GLTF_EXP_MAT_PTR_IOR                          9u
#define GLTF_EXP_MAT_PTR_SPECULAR                     10u
#define GLTF_EXP_MAT_PTR_SPECULAR_COLOR               11u
#define GLTF_EXP_MAT_PTR_CLEARCOAT                    12u
#define GLTF_EXP_MAT_PTR_CLEARCOAT_ROUGHNESS          13u
#define GLTF_EXP_MAT_PTR_CLEARCOAT_NORMAL_SCALE       14u
#define GLTF_EXP_MAT_PTR_TRANSMISSION                 15u
#define GLTF_EXP_MAT_PTR_SHEEN_COLOR                  16u
#define GLTF_EXP_MAT_PTR_SHEEN_ROUGHNESS              17u
#define GLTF_EXP_MAT_PTR_IRIDESCENCE                  18u
#define GLTF_EXP_MAT_PTR_IRIDESCENCE_IOR              19u
#define GLTF_EXP_MAT_PTR_IRIDESCENCE_THICKNESS_MIN    20u
#define GLTF_EXP_MAT_PTR_IRIDESCENCE_THICKNESS_MAX    21u
#define GLTF_EXP_MAT_PTR_VOLUME_THICKNESS             22u
#define GLTF_EXP_MAT_PTR_VOLUME_ATTENUATION_DISTANCE  23u
#define GLTF_EXP_MAT_PTR_VOLUME_ATTENUATION_COLOR     24u
#define GLTF_EXP_MAT_PTR_ANISOTROPY                   25u
#define GLTF_EXP_MAT_PTR_ANISOTROPY_ROTATION          26u
#define GLTF_EXP_MAT_PTR_DISPERSION                   27u
#define GLTF_EXP_MAT_PTR_DIFFUSE_TRANSMISSION         28u
#define GLTF_EXP_MAT_PTR_DIFFUSE_TRANSMISSION_COLOR   29u

static
void
gltf_write_anim_path(GLTFExpWriter * __restrict w,
                     GLTFExpAnimPath            path) {
  switch (path) {
    case GLTF_EXP_ANIM_TRANSLATION:
      gltf_w_qstr_len(w, _s_gltf_translation, _s_gltf_translation_len);
      return;
    case GLTF_EXP_ANIM_ROTATION:
      gltf_w_qstr_len(w, _s_gltf_rotation, _s_gltf_rotation_len);
      return;
    case GLTF_EXP_ANIM_SCALE:
      gltf_w_qstr_len(w, _s_gltf_scale, _s_gltf_scale_len);
      return;
    case GLTF_EXP_ANIM_WEIGHTS:
      gltf_w_qstr_len(w, _s_gltf_weights, _s_gltf_weights_len);
      return;
    default:
      break;
  }

  w->result = AK_ERR;
}

static
void
gltf_write_anim_pointer_node_visible(GLTFExpWriter * __restrict w,
                                     GLTFExpIndex               nodeIndex) {
  gltf_w_ch(w, '"');
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_nodes, _s_gltf_nodes_len);
  gltf_w_ch(w, '/');
  gltf_w_uint(w, nodeIndex);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_extensions, _s_gltf_extensions_len);
  gltf_w_ch(w, '/');
  gltf_w_raw(w,
             _s_gltf_KHR_node_visibility,
             _s_gltf_KHR_node_visibility_len);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_visible, _s_gltf_visible_len);
  gltf_w_ch(w, '"');
}

static
void
gltf_write_anim_pointer_texture_prop(GLTFExpWriter * __restrict w,
                                     uint32_t                   prop) {
  switch (prop) {
    case GLTF_EXP_TEX_XFORM_OFFSET:
      gltf_w_raw(w, _s_gltf_offset, _s_gltf_offset_len);
      return;
    case GLTF_EXP_TEX_XFORM_SCALE:
      gltf_w_raw(w, _s_gltf_scale, _s_gltf_scale_len);
      return;
    case GLTF_EXP_TEX_XFORM_ROTATION:
      gltf_w_raw(w, _s_gltf_rotation, _s_gltf_rotation_len);
      return;
    default:
      break;
  }

  w->result = AK_ERR;
}

static
void
gltf_write_anim_pointer_texture_tail(GLTFExpWriter * __restrict w,
                                     uint32_t                   prop) {
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_extensions, _s_gltf_extensions_len);
  gltf_w_ch(w, '/');
  gltf_w_raw(w,
             _s_gltf_KHR_texture_transform,
             _s_gltf_KHR_texture_transform_len);
  gltf_w_ch(w, '/');
  gltf_write_anim_pointer_texture_prop(w, prop);
}

static
void
gltf_write_anim_pointer_material_tex_top(GLTFExpWriter * __restrict w,
                                         GLTFExpIndex               materialIndex,
                                         const char * __restrict    textureName,
                                         size_t                     textureNameLen,
                                         uint32_t                   prop) {
  gltf_w_ch(w, '"');
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_materials, _s_gltf_materials_len);
  gltf_w_ch(w, '/');
  gltf_w_uint(w, materialIndex);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, textureName, textureNameLen);
  gltf_write_anim_pointer_texture_tail(w, prop);
  gltf_w_ch(w, '"');
}

static
void
gltf_write_anim_pointer_material_tex_pbr(GLTFExpWriter * __restrict w,
                                         GLTFExpIndex               materialIndex,
                                         const char * __restrict    textureName,
                                         size_t                     textureNameLen,
                                         uint32_t                   prop) {
  gltf_w_ch(w, '"');
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_materials, _s_gltf_materials_len);
  gltf_w_ch(w, '/');
  gltf_w_uint(w, materialIndex);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_pbrMetalRough, _s_gltf_pbrMetalRough_len);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, textureName, textureNameLen);
  gltf_write_anim_pointer_texture_tail(w, prop);
  gltf_w_ch(w, '"');
}

static
void
gltf_write_anim_pointer_material_tex_ext(GLTFExpWriter * __restrict w,
                                         GLTFExpIndex               materialIndex,
                                         const char * __restrict    extName,
                                         size_t                     extNameLen,
                                         const char * __restrict    textureName,
                                         size_t                     textureNameLen,
                                         uint32_t                   prop) {
  gltf_w_ch(w, '"');
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_materials, _s_gltf_materials_len);
  gltf_w_ch(w, '/');
  gltf_w_uint(w, materialIndex);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_extensions, _s_gltf_extensions_len);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, extName, extNameLen);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, textureName, textureNameLen);
  gltf_write_anim_pointer_texture_tail(w, prop);
  gltf_w_ch(w, '"');
}

static
void
gltf_write_anim_pointer_texture_transform(GLTFExpWriter          * __restrict w,
                                          GLTFExpAnimChannelOut * __restrict channel) {
  switch (channel->pointerRole) {
    case GLTF_EXP_TEX_ROLE_BASE_COLOR:
      gltf_write_anim_pointer_material_tex_pbr(w,
                                               channel->nodeIndex,
                                               _s_gltf_baseColorTex,
                                               _s_gltf_baseColorTex_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_METALLIC_ROUGHNESS:
      gltf_write_anim_pointer_material_tex_pbr(w,
                                               channel->nodeIndex,
                                               _s_gltf_metalRoughTex,
                                               _s_gltf_metalRoughTex_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_OCCLUSION:
      gltf_write_anim_pointer_material_tex_top(w,
                                               channel->nodeIndex,
                                               _s_gltf_occlusionTex,
                                               _s_gltf_occlusionTex_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_NORMAL:
      gltf_write_anim_pointer_material_tex_top(w,
                                               channel->nodeIndex,
                                               _s_gltf_normalTex,
                                               _s_gltf_normalTex_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_EMISSIVE:
      gltf_write_anim_pointer_material_tex_top(w,
                                               channel->nodeIndex,
                                               _s_gltf_emissiveTex,
                                               _s_gltf_emissiveTex_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_SPECULAR:
      gltf_write_anim_pointer_material_tex_ext(w,
                                               channel->nodeIndex,
                                               _s_gltf_KHR_materials_specular,
                                               _s_gltf_KHR_materials_specular_len,
                                               _s_gltf_specularTexture,
                                               _s_gltf_specularTexture_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_SPECULAR_COLOR:
      gltf_write_anim_pointer_material_tex_ext(w,
                                               channel->nodeIndex,
                                               _s_gltf_KHR_materials_specular,
                                               _s_gltf_KHR_materials_specular_len,
                                               _s_gltf_specularColorTexture,
                                               _s_gltf_specularColorTexture_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_CLEARCOAT:
      gltf_write_anim_pointer_material_tex_ext(w,
                                               channel->nodeIndex,
                                               _s_gltf_KHR_materials_clearcoat,
                                               _s_gltf_KHR_materials_clearcoat_len,
                                               _s_gltf_clearcoatTexture,
                                               _s_gltf_clearcoatTexture_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_CLEARCOAT_ROUGHNESS:
      gltf_write_anim_pointer_material_tex_ext(w,
                                               channel->nodeIndex,
                                               _s_gltf_KHR_materials_clearcoat,
                                               _s_gltf_KHR_materials_clearcoat_len,
                                               _s_gltf_clearcoatRoughnessTexture,
                                               _s_gltf_clearcoatRoughnessTexture_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_CLEARCOAT_NORMAL:
      gltf_write_anim_pointer_material_tex_ext(w,
                                               channel->nodeIndex,
                                               _s_gltf_KHR_materials_clearcoat,
                                               _s_gltf_KHR_materials_clearcoat_len,
                                               _s_gltf_clearcoatNormalTexture,
                                               _s_gltf_clearcoatNormalTexture_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_TRANSMISSION:
      gltf_write_anim_pointer_material_tex_ext(w,
                                               channel->nodeIndex,
                                               _s_gltf_KHR_materials_transmission,
                                               _s_gltf_KHR_materials_transmission_len,
                                               _s_gltf_transmissionTexture,
                                               _s_gltf_transmissionTexture_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_SHEEN_COLOR:
      gltf_write_anim_pointer_material_tex_ext(w,
                                               channel->nodeIndex,
                                               _s_gltf_KHR_materials_sheen,
                                               _s_gltf_KHR_materials_sheen_len,
                                               _s_gltf_sheenColorTexture,
                                               _s_gltf_sheenColorTexture_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_SHEEN_ROUGHNESS:
      gltf_write_anim_pointer_material_tex_ext(w,
                                               channel->nodeIndex,
                                               _s_gltf_KHR_materials_sheen,
                                               _s_gltf_KHR_materials_sheen_len,
                                               _s_gltf_sheenRoughnessTexture,
                                               _s_gltf_sheenRoughnessTexture_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_IRIDESCENCE:
      gltf_write_anim_pointer_material_tex_ext(w,
                                               channel->nodeIndex,
                                               _s_gltf_KHR_materials_iridescence,
                                               _s_gltf_KHR_materials_iridescence_len,
                                               _s_gltf_iridescenceTexture,
                                               _s_gltf_iridescenceTexture_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_IRIDESCENCE_THICKNESS:
      gltf_write_anim_pointer_material_tex_ext(w,
                                               channel->nodeIndex,
                                               _s_gltf_KHR_materials_iridescence,
                                               _s_gltf_KHR_materials_iridescence_len,
                                               _s_gltf_iridescenceThicknessTexture,
                                               _s_gltf_iridescenceThicknessTexture_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_VOLUME_THICKNESS:
      gltf_write_anim_pointer_material_tex_ext(w,
                                               channel->nodeIndex,
                                               _s_gltf_KHR_materials_volume,
                                               _s_gltf_KHR_materials_volume_len,
                                               _s_gltf_thicknessTexture,
                                               _s_gltf_thicknessTexture_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_ANISOTROPY:
      gltf_write_anim_pointer_material_tex_ext(w,
                                               channel->nodeIndex,
                                               _s_gltf_KHR_materials_anisotropy,
                                               _s_gltf_KHR_materials_anisotropy_len,
                                               _s_gltf_anisotropyTexture,
                                               _s_gltf_anisotropyTexture_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_DIFFUSE_TRANSMISSION:
      gltf_write_anim_pointer_material_tex_ext(w,
                                               channel->nodeIndex,
                                               _s_gltf_KHR_materials_diffuse_transmission,
                                               _s_gltf_KHR_materials_diffuse_transmission_len,
                                               _s_gltf_diffuseTransmissionTexture,
                                               _s_gltf_diffuseTransmissionTexture_len,
                                               channel->pointerProp);
      return;
    case GLTF_EXP_TEX_ROLE_DIFFUSE_TRANSMISSION_COLOR:
      gltf_write_anim_pointer_material_tex_ext(w,
                                               channel->nodeIndex,
                                               _s_gltf_KHR_materials_diffuse_transmission,
                                               _s_gltf_KHR_materials_diffuse_transmission_len,
                                               _s_gltf_diffuseTransmissionColorTexture,
                                               _s_gltf_diffuseTransmissionColorTexture_len,
                                               channel->pointerProp);
      return;
    default:
      break;
  }

  w->result = AK_ERR;
}

static
void
gltf_write_anim_pointer_material_texinfo_top(GLTFExpWriter * __restrict w,
                                             GLTFExpIndex               materialIndex,
                                             const char * __restrict    textureName,
                                             size_t                     textureNameLen,
                                             const char * __restrict    propName,
                                             size_t                     propNameLen) {
  gltf_w_ch(w, '"');
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_materials, _s_gltf_materials_len);
  gltf_w_ch(w, '/');
  gltf_w_uint(w, materialIndex);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, textureName, textureNameLen);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, propName, propNameLen);
  gltf_w_ch(w, '"');
}

static
void
gltf_write_anim_pointer_material_texinfo_ext(GLTFExpWriter * __restrict w,
                                             GLTFExpIndex               materialIndex,
                                             const char * __restrict    extName,
                                             size_t                     extNameLen,
                                             const char * __restrict    textureName,
                                             size_t                     textureNameLen,
                                             const char * __restrict    propName,
                                             size_t                     propNameLen) {
  gltf_w_ch(w, '"');
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_materials, _s_gltf_materials_len);
  gltf_w_ch(w, '/');
  gltf_w_uint(w, materialIndex);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_extensions, _s_gltf_extensions_len);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, extName, extNameLen);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, textureName, textureNameLen);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, propName, propNameLen);
  gltf_w_ch(w, '"');
}

static
void
gltf_write_anim_pointer_material_prop_top(GLTFExpWriter * __restrict w,
                                          GLTFExpIndex               materialIndex,
                                          const char * __restrict    propName,
                                          size_t                     propNameLen) {
  gltf_w_ch(w, '"');
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_materials, _s_gltf_materials_len);
  gltf_w_ch(w, '/');
  gltf_w_uint(w, materialIndex);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, propName, propNameLen);
  gltf_w_ch(w, '"');
}

static
void
gltf_write_anim_pointer_material_prop_pbr(GLTFExpWriter * __restrict w,
                                          GLTFExpIndex               materialIndex,
                                          const char * __restrict    propName,
                                          size_t                     propNameLen) {
  gltf_w_ch(w, '"');
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_materials, _s_gltf_materials_len);
  gltf_w_ch(w, '/');
  gltf_w_uint(w, materialIndex);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_pbrMetalRough, _s_gltf_pbrMetalRough_len);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, propName, propNameLen);
  gltf_w_ch(w, '"');
}

static
void
gltf_write_anim_pointer_material_prop_ext(GLTFExpWriter * __restrict w,
                                          GLTFExpIndex               materialIndex,
                                          const char * __restrict    extName,
                                          size_t                     extNameLen,
                                          const char * __restrict    propName,
                                          size_t                     propNameLen) {
  gltf_w_ch(w, '"');
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_materials, _s_gltf_materials_len);
  gltf_w_ch(w, '/');
  gltf_w_uint(w, materialIndex);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, _s_gltf_extensions, _s_gltf_extensions_len);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, extName, extNameLen);
  gltf_w_ch(w, '/');
  gltf_w_raw(w, propName, propNameLen);
  gltf_w_ch(w, '"');
}

static
void
gltf_write_anim_pointer_material_value(GLTFExpWriter          * __restrict w,
                                       GLTFExpAnimChannelOut * __restrict channel) {
  switch (channel->pointerProp) {
    case GLTF_EXP_MAT_PTR_BASE_COLOR:
      gltf_write_anim_pointer_material_prop_pbr(w,
                                                channel->nodeIndex,
                                                _s_gltf_baseColor,
                                                _s_gltf_baseColor_len);
      return;
    case GLTF_EXP_MAT_PTR_METALLIC:
      gltf_write_anim_pointer_material_prop_pbr(w,
                                                channel->nodeIndex,
                                                _s_gltf_metalFac,
                                                _s_gltf_metalFac_len);
      return;
    case GLTF_EXP_MAT_PTR_ROUGHNESS:
      gltf_write_anim_pointer_material_prop_pbr(w,
                                                channel->nodeIndex,
                                                _s_gltf_roughFac,
                                                _s_gltf_roughFac_len);
      return;
    case GLTF_EXP_MAT_PTR_ALPHA_CUTOFF:
      gltf_write_anim_pointer_material_prop_top(w,
                                                channel->nodeIndex,
                                                _s_gltf_alphaCutoff,
                                                _s_gltf_alphaCutoff_len);
      return;
    case GLTF_EXP_MAT_PTR_EMISSIVE_COLOR:
      gltf_write_anim_pointer_material_prop_top(w,
                                                channel->nodeIndex,
                                                _s_gltf_emissiveFac,
                                                _s_gltf_emissiveFac_len);
      return;
    case GLTF_EXP_MAT_PTR_EMISSIVE_STRENGTH:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_emissive_strength,
                                                _s_gltf_KHR_materials_emissive_strength_len,
                                                _s_gltf_emissiveStrength,
                                                _s_gltf_emissiveStrength_len);
      return;
    case GLTF_EXP_MAT_PTR_NORMAL_SCALE:
      gltf_write_anim_pointer_material_texinfo_top(w,
                                                   channel->nodeIndex,
                                                   _s_gltf_normalTex,
                                                   _s_gltf_normalTex_len,
                                                   _s_gltf_scale,
                                                   _s_gltf_scale_len);
      return;
    case GLTF_EXP_MAT_PTR_OCCLUSION_STRENGTH:
      gltf_write_anim_pointer_material_texinfo_top(w,
                                                   channel->nodeIndex,
                                                   _s_gltf_occlusionTex,
                                                   _s_gltf_occlusionTex_len,
                                                   _s_gltf_strength,
                                                   _s_gltf_strength_len);
      return;
    case GLTF_EXP_MAT_PTR_IOR:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_ior,
                                                _s_gltf_KHR_materials_ior_len,
                                                _s_gltf_ior,
                                                _s_gltf_ior_len);
      return;
    case GLTF_EXP_MAT_PTR_SPECULAR:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_specular,
                                                _s_gltf_KHR_materials_specular_len,
                                                _s_gltf_specularFactor,
                                                _s_gltf_specularFactor_len);
      return;
    case GLTF_EXP_MAT_PTR_SPECULAR_COLOR:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_specular,
                                                _s_gltf_KHR_materials_specular_len,
                                                _s_gltf_specularColorFactor,
                                                _s_gltf_specularColorFactor_len);
      return;
    case GLTF_EXP_MAT_PTR_CLEARCOAT:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_clearcoat,
                                                _s_gltf_KHR_materials_clearcoat_len,
                                                _s_gltf_clearcoatFactor,
                                                _s_gltf_clearcoatFactor_len);
      return;
    case GLTF_EXP_MAT_PTR_CLEARCOAT_ROUGHNESS:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_clearcoat,
                                                _s_gltf_KHR_materials_clearcoat_len,
                                                _s_gltf_clearcoatRoughnessFactor,
                                                _s_gltf_clearcoatRoughnessFactor_len);
      return;
    case GLTF_EXP_MAT_PTR_CLEARCOAT_NORMAL_SCALE:
      gltf_write_anim_pointer_material_texinfo_ext(w,
                                                   channel->nodeIndex,
                                                   _s_gltf_KHR_materials_clearcoat,
                                                   _s_gltf_KHR_materials_clearcoat_len,
                                                   _s_gltf_clearcoatNormalTexture,
                                                   _s_gltf_clearcoatNormalTexture_len,
                                                   _s_gltf_scale,
                                                   _s_gltf_scale_len);
      return;
    case GLTF_EXP_MAT_PTR_TRANSMISSION:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_transmission,
                                                _s_gltf_KHR_materials_transmission_len,
                                                _s_gltf_transmissionFactor,
                                                _s_gltf_transmissionFactor_len);
      return;
    case GLTF_EXP_MAT_PTR_SHEEN_COLOR:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_sheen,
                                                _s_gltf_KHR_materials_sheen_len,
                                                _s_gltf_sheenColorFactor,
                                                _s_gltf_sheenColorFactor_len);
      return;
    case GLTF_EXP_MAT_PTR_SHEEN_ROUGHNESS:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_sheen,
                                                _s_gltf_KHR_materials_sheen_len,
                                                _s_gltf_sheenRoughnessFactor,
                                                _s_gltf_sheenRoughnessFactor_len);
      return;
    case GLTF_EXP_MAT_PTR_IRIDESCENCE:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_iridescence,
                                                _s_gltf_KHR_materials_iridescence_len,
                                                _s_gltf_iridescenceFactor,
                                                _s_gltf_iridescenceFactor_len);
      return;
    case GLTF_EXP_MAT_PTR_IRIDESCENCE_IOR:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_iridescence,
                                                _s_gltf_KHR_materials_iridescence_len,
                                                _s_gltf_iridescenceIor,
                                                _s_gltf_iridescenceIor_len);
      return;
    case GLTF_EXP_MAT_PTR_IRIDESCENCE_THICKNESS_MIN:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_iridescence,
                                                _s_gltf_KHR_materials_iridescence_len,
                                                _s_gltf_iridescenceThicknessMinimum,
                                                _s_gltf_iridescenceThicknessMinimum_len);
      return;
    case GLTF_EXP_MAT_PTR_IRIDESCENCE_THICKNESS_MAX:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_iridescence,
                                                _s_gltf_KHR_materials_iridescence_len,
                                                _s_gltf_iridescenceThicknessMaximum,
                                                _s_gltf_iridescenceThicknessMaximum_len);
      return;
    case GLTF_EXP_MAT_PTR_VOLUME_THICKNESS:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_volume,
                                                _s_gltf_KHR_materials_volume_len,
                                                _s_gltf_thicknessFactor,
                                                _s_gltf_thicknessFactor_len);
      return;
    case GLTF_EXP_MAT_PTR_VOLUME_ATTENUATION_DISTANCE:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_volume,
                                                _s_gltf_KHR_materials_volume_len,
                                                _s_gltf_attenuationDistance,
                                                _s_gltf_attenuationDistance_len);
      return;
    case GLTF_EXP_MAT_PTR_VOLUME_ATTENUATION_COLOR:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_volume,
                                                _s_gltf_KHR_materials_volume_len,
                                                _s_gltf_attenuationColor,
                                                _s_gltf_attenuationColor_len);
      return;
    case GLTF_EXP_MAT_PTR_ANISOTROPY:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_anisotropy,
                                                _s_gltf_KHR_materials_anisotropy_len,
                                                _s_gltf_anisotropyStrength,
                                                _s_gltf_anisotropyStrength_len);
      return;
    case GLTF_EXP_MAT_PTR_ANISOTROPY_ROTATION:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_anisotropy,
                                                _s_gltf_KHR_materials_anisotropy_len,
                                                _s_gltf_anisotropyRotation,
                                                _s_gltf_anisotropyRotation_len);
      return;
    case GLTF_EXP_MAT_PTR_DISPERSION:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_dispersion,
                                                _s_gltf_KHR_materials_dispersion_len,
                                                _s_gltf_dispersion,
                                                _s_gltf_dispersion_len);
      return;
    case GLTF_EXP_MAT_PTR_DIFFUSE_TRANSMISSION:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_diffuse_transmission,
                                                _s_gltf_KHR_materials_diffuse_transmission_len,
                                                _s_gltf_diffuseTransmissionFactor,
                                                _s_gltf_diffuseTransmissionFactor_len);
      return;
    case GLTF_EXP_MAT_PTR_DIFFUSE_TRANSMISSION_COLOR:
      gltf_write_anim_pointer_material_prop_ext(w,
                                                channel->nodeIndex,
                                                _s_gltf_KHR_materials_diffuse_transmission,
                                                _s_gltf_KHR_materials_diffuse_transmission_len,
                                                _s_gltf_diffuseTransmissionColorFactor,
                                                _s_gltf_diffuseTransmissionColorFactor_len);
      return;
    default:
      break;
  }

  w->result = AK_ERR;
}

static
void
gltf_write_anim_pointer_target(GLTFExpWriter          * __restrict w,
                               GLTFExpAnimChannelOut * __restrict channel) {
  gltf_w_ch(w, '{');
  gltf_w_key(w, _s_gltf_path, _s_gltf_path_len);
  gltf_w_qstr_len(w, _s_gltf_pointer, _s_gltf_pointer_len);
  gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_extensions, _s_gltf_extensions_len);
  gltf_w_ch(w, '{');
  gltf_w_key(w,
             _s_gltf_KHR_animation_pointer,
             _s_gltf_KHR_animation_pointer_len);
  gltf_w_ch(w, '{');
  gltf_w_key(w, _s_gltf_pointer, _s_gltf_pointer_len);

  switch (channel->path) {
    case GLTF_EXP_ANIM_POINTER_NODE_VISIBLE:
      gltf_write_anim_pointer_node_visible(w, channel->nodeIndex);
      break;
    case GLTF_EXP_ANIM_POINTER_MATERIAL_VALUE:
      gltf_write_anim_pointer_material_value(w, channel);
      break;
    case GLTF_EXP_ANIM_POINTER_TEXTURE_TRANSFORM:
      gltf_write_anim_pointer_texture_transform(w, channel);
      break;
    default:
      w->result = AK_ERR;
      break;
  }

  gltf_w_ch(w, '}');
  gltf_w_ch(w, '}');
  gltf_w_ch(w, '}');
}

static
void
gltf_write_anim_sampler(GLTFExpWriter          * __restrict w,
                        GLTFExpAnimSamplerOut * __restrict sampler) {
  gltf_w_ch(w, '{');
  gltf_w_key_uint(w, _s_gltf_input, _s_gltf_input_len,
                  sampler->inputAccessorIndex);
  gltf_w_ch(w, ',');
  gltf_w_key_uint(w, _s_gltf_output, _s_gltf_output_len,
                  sampler->outputAccessorIndex);

  if (sampler->interpolation == AK_INTERPOLATION_STEP) {
    gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_interpolation, _s_gltf_interpolation_len);
    gltf_w_qstr_len(w, _s_gltf_STEP, _s_gltf_STEP_len);
  } else if (sampler->interpolation == AK_INTERPOLATION_HERMITE) {
    gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_interpolation, _s_gltf_interpolation_len);
    gltf_w_qstr_len(w, _s_gltf_CUBICSPLINE, _s_gltf_CUBICSPLINE_len);
  }

  gltf_w_ch(w, '}');
}

static
void
gltf_write_anim_channel(GLTFExpWriter          * __restrict w,
                        GLTFExpAnimChannelOut * __restrict channel,
                        size_t                             samplerOffset) {
  gltf_w_ch(w, '{');
  gltf_w_key_uint(w, _s_gltf_sampler, _s_gltf_sampler_len,
                  channel->samplerIndex - samplerOffset);
  gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_target, _s_gltf_target_len);

  if (channel->path == GLTF_EXP_ANIM_POINTER_NODE_VISIBLE
      || channel->path == GLTF_EXP_ANIM_POINTER_MATERIAL_VALUE
      || channel->path == GLTF_EXP_ANIM_POINTER_TEXTURE_TRANSFORM) {
    gltf_write_anim_pointer_target(w, channel);
    gltf_w_ch(w, '}');
    return;
  }

  gltf_w_ch(w, '{');
  gltf_w_key_uint(w, _s_gltf_node, _s_gltf_node_len, channel->nodeIndex);
  gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_path, _s_gltf_path_len);
  gltf_write_anim_path(w, channel->path);
  gltf_w_ch(w, '}');
  gltf_w_ch(w, '}');
}

static
void
gltf_write_animation(GLTFExpWriter * __restrict w,
                     GLTFExpState  * __restrict st,
                     GLTFExpAnimOut * __restrict anim) {
  uint32_t i;
  bool     comma;

  comma = false;
  gltf_w_ch(w, '{');

  if (anim->name) {
    gltf_w_key_str(w, _s_gltf_name, _s_gltf_name_len, anim->name);
    comma = true;
  }

  if (comma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_samplers, _s_gltf_samplers_len);
  gltf_w_ch(w, '[');
  for (i = 0; i < anim->samplerCount; i++) {
    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_write_anim_sampler(w,
                            &st->animSamplers.items[anim->samplerOffset + i]);
  }
  gltf_w_ch(w, ']');

  gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_channels, _s_gltf_channels_len);
  gltf_w_ch(w, '[');
  for (i = 0; i < anim->channelCount; i++) {
    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_write_anim_channel(w,
                            &st->animChannels.items[anim->channelOffset + i],
                            anim->samplerOffset);
  }
  gltf_w_ch(w, ']');

  gltf_w_ch(w, '}');
}

void
gltf_write_animations(GLTFExpWriter * __restrict w,
                      GLTFExpState  * __restrict st) {
  size_t i;

  gltf_w_key(w, _s_gltf_animations, _s_gltf_animations_len);
  gltf_w_ch(w, '[');
  for (i = 0; i < st->animations.count; i++) {
    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_write_animation(w, st, &st->animations.items[i]);
  }
  gltf_w_ch(w, ']');
}
