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

#include "gltf.h"
#include "../../common/binary.h"
#include "../../common/path.h"
#include "accessor.h"
#include "anim.h"
#include "bin.h"
#include "camera.h"
#include "common.h"
#include "extra.h"
#include "light.h"
#include "material.h"
#include "mesh.h"
#include "plan.h"
#include "scene.h"
#include "skin.h"
#include "writer.h"
#include "../strpool.h"
#include "../../../image/export.h"
#include "../../../../include/ak/options.h"
#include "../../../../include/ak/version.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define GLTF_EXP_FILE_BUFFER_SIZE (1024u * 1024u)

enum {
  GLTF_GLB_MAGIC = 0x46546c67u,
  GLTF_GLB_JSON  = 0x4e4f534au,
  GLTF_GLB_BIN   = 0x004e4942u,
  GLTF_GLB_VER   = 2u
};

static
void
gltf_configure_file_buffer(FILE * __restrict file) {
  if (file)
    (void)setvbuf(file, NULL, _IOFBF, GLTF_EXP_FILE_BUFFER_SIZE);
}

static
void
gltf_state_destroy(GLTFExpState * __restrict st) {
  size_t i;

  if (st->nodeStack)               rb_destroy(st->nodeStack);
  if (st->nodeMap)                 rb_destroy(st->nodeMap);
  if (st->meshes.map)              rb_destroy(st->meshes.map);
  if (st->textures.map)            rb_destroy(st->textures.map);
  if (st->images.map)              rb_destroy(st->images.map);
  if (st->samplers.map)            rb_destroy(st->samplers.map);
  if (st->cameras.map)             rb_destroy(st->cameras.map);
  if (st->lights.map)              rb_destroy(st->lights.map);
  if (st->skins.map)               rb_destroy(st->skins.map);
  if (st->skinJointRoots.map)      rb_destroy(st->skinJointRoots.map);
  if (st->sceneSkinJointRoots.map) rb_destroy(st->sceneSkinJointRoots.map);
  if (st->accessors.accessorMap)   rb_destroy(st->accessors.accessorMap);
  if (st->accessors.primitiveMap)  rb_destroy(st->accessors.primitiveMap);
  if (st->accessors.rawMap)        rb_destroy(st->accessors.rawMap);

  for (i = 0; i < st->accessors.count; i++)
    free(st->accessors.items[i].rawPath);

  if (st->imagePayloads) {
    for (i = 0; i < st->images.count; i++)
      ak_imageExportPayloadRelease(&st->imagePayloads[i]);
  }

  if (st->imageExportUris) {
    for (i = 0; i < st->images.count; i++)
      free(st->imageExportUris[i]);
  }

  for (i = 0; i < st->skinAttrs.count; i++)
    free(st->skinAttrs.items[i].data);

  for (i = 0; i < st->morphAttrs.count; i++) {
    free(st->morphAttrs.items[i].positionData);
    free(st->morphAttrs.items[i].normalData);
    free(st->morphAttrs.items[i].tangentData);
  }
  for (i = 0; i < st->positionAttrs.count; i++)
    free(st->positionAttrs.items[i].data);
  for (i = 0; i < st->bakedAttrs.count; i++) {
    free(st->bakedAttrs.items[i].positionData);
    free(st->bakedAttrs.items[i].normalData);
  }

  free(st->nodes.items);
  free(st->nodeChildren.items);
  free(st->scenes.items);
  free(st->sceneRoots.items);
  free(st->meshes.items);
  free(st->materials.items);
  free(st->textures.items);
  free(st->images.items);
  free(st->imageBufferViews);
  free(st->imageMimeTypes);
  free(st->imagePayloads);
  free(st->imageExportUris);
  free(st->samplers.items);
  free(st->cameras.items);
  free(st->lights.items);
  free(st->skins.items);
  free(st->skinJointRoots.items);
  free(st->sceneSkinJointRoots.items);
  free(st->skinAttrs.items);
  free(st->morphAttrs.items);
  free(st->positionAttrs.items);
  free(st->bakedAttrs.items);
  free(st->animations.items);
  free(st->animSamplers.items);
  free(st->animChannels.items);
  free(st->accessors.items);
  free(st->preservedExtensions.items);
  free(st->outDir);
  free(st->binPath);
  free(st->binUri);
}

static
bool
gltf_root_ext_name_eq(const char * __restrict name,
                      size_t                  nameLen,
                      const char * __restrict expected,
                      size_t                  expectedLen) {
  return name
         && nameLen == expectedLen
         && memcmp(name, expected, expectedLen) == 0;
}

static
bool
gltf_root_ext_is_core_payload(const char * __restrict name,
                              size_t                  nameLen) {
  return gltf_root_ext_name_eq(name,
                               nameLen,
                               _s_gltf_KHR_lights_punctual,
                               _s_gltf_KHR_lights_punctual_len)
         || gltf_root_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_materials_variants,
                                  _s_gltf_KHR_materials_variants_len);
}

static
bool
gltf_root_ext_material_used(uint32_t               mask,
                            const char * __restrict name,
                            size_t                  nameLen) {
#define GLTF_EXP_MAT_USED(MASK, NAME)                                        \
  (((mask) & (MASK))                                                         \
   && gltf_root_ext_name_eq(name, nameLen, _s_gltf_ ## NAME,                 \
                            _s_gltf_ ## NAME ## _len))

  return GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_UNLIT,
                           KHR_materials_unlit)
         || GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_EMISSIVE_STRENGTH,
                              KHR_materials_emissive_strength)
         || GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_IOR,
                              KHR_materials_ior)
         || GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_CLEARCOAT,
                              KHR_materials_clearcoat)
         || GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_SPECULAR,
                              KHR_materials_specular)
         || GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_SPECULAR_GLOSSINESS,
                              KHR_materials_pbrSpecularGlossiness)
         || GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_TRANSMISSION,
                              KHR_materials_transmission)
         || GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_SHEEN,
                              KHR_materials_sheen)
         || GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_IRIDESCENCE,
                              KHR_materials_iridescence)
         || GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_VOLUME,
                              KHR_materials_volume)
         || GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_ANISOTROPY,
                              KHR_materials_anisotropy)
         || GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_DISPERSION,
                              KHR_materials_dispersion)
         || GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_DIFFUSE_TRANSMISSION,
                              KHR_materials_diffuse_transmission)
         || GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_VOLUME_SCATTER,
                              KHR_materials_volume_scatter)
         || GLTF_EXP_MAT_USED(GLTF_EXP_MAT_EXT_TEXTURE_TRANSFORM,
                              KHR_texture_transform);
#undef GLTF_EXP_MAT_USED
}

static
bool
gltf_root_ext_core_used(GLTFExpState * __restrict st,
                        uint32_t                  materialMask,
                        uint32_t                  textureMask,
                        const char * __restrict   name,
                        size_t                    nameLen) {
  return (st->lights.count > 0
          && gltf_root_ext_name_eq(name,
                                   nameLen,
                                   _s_gltf_KHR_lights_punctual,
                                   _s_gltf_KHR_lights_punctual_len))
         || gltf_root_ext_material_used(materialMask, name, nameLen)
         || ((textureMask & GLTF_EXP_TEX_EXT_BASISU)
             && gltf_root_ext_name_eq(name,
                                      nameLen,
                                      _s_gltf_KHR_texture_basisu,
                                      _s_gltf_KHR_texture_basisu_len))
         || ((textureMask & GLTF_EXP_TEX_EXT_WEBP)
             && gltf_root_ext_name_eq(name,
                                      nameLen,
                                      _s_gltf_EXT_texture_webp,
                                      _s_gltf_EXT_texture_webp_len))
         || (gltf_has_material_variants(st)
             && gltf_root_ext_name_eq(name,
                                      nameLen,
                                      _s_gltf_KHR_materials_variants,
                                      _s_gltf_KHR_materials_variants_len))
         || (st->usesNodeVisibility
             && gltf_root_ext_name_eq(name,
                                      nameLen,
                                      _s_gltf_KHR_node_visibility,
                                      _s_gltf_KHR_node_visibility_len))
         || (st->usesGpuInstancing
             && gltf_root_ext_name_eq(name,
                                      nameLen,
                                      _s_gltf_EXT_mesh_gpu_instancing,
                                      _s_gltf_EXT_mesh_gpu_instancing_len))
         || (st->usesMeshQuantization
             && gltf_root_ext_name_eq(name,
                                      nameLen,
                                      _s_gltf_KHR_mesh_quantization,
                                      _s_gltf_KHR_mesh_quantization_len))
         || (st->usesAnimationPointer
             && gltf_root_ext_name_eq(name,
                                      nameLen,
                                      _s_gltf_KHR_animation_pointer,
                                      _s_gltf_KHR_animation_pointer_len));
}

static
bool
gltf_root_ext_core_required(uint32_t                textureMask,
                            GLTFExpState * __restrict st,
                            const char * __restrict name,
                            size_t                  nameLen) {
  return ((textureMask & GLTF_EXP_TEX_EXT_BASISU)
          && gltf_root_ext_name_eq(name,
                                   nameLen,
                                   _s_gltf_KHR_texture_basisu,
                                   _s_gltf_KHR_texture_basisu_len))
         || ((textureMask & GLTF_EXP_TEX_EXT_WEBP)
             && gltf_root_ext_name_eq(name,
                                      nameLen,
                                      _s_gltf_EXT_texture_webp,
                                      _s_gltf_EXT_texture_webp_len))
         || (st->usesMeshQuantization
             && gltf_root_ext_name_eq(name,
                                      nameLen,
                                      _s_gltf_KHR_mesh_quantization,
                                      _s_gltf_KHR_mesh_quantization_len));
}

static
bool
gltf_has_preserved_root_extension(GLTFExpState * __restrict st) {
  AkTreeNode *root;
  AkTreeNode *child;

  root = gltf_extra_root_extensions_node(st);
  for (child = root ? root->chld : NULL; child; child = child->next) {
    if (child->name && !gltf_root_ext_is_core_payload(child->name,
                                                      strlen(child->name)))
      return true;
  }

  return false;
}

static
bool
gltf_has_preserved_required_root_extension(GLTFExpState * __restrict st,
                                           uint32_t                  textureMask) {
  AkTreeNode *root;
  AkTreeNode *child;

  root = gltf_extra_root_extensions_node(st);
  for (child = root ? root->chld : NULL; child; child = child->next) {
    size_t nameLen;

    if (!child->name)
      continue;

    nameLen = strlen(child->name);
    if (gltf_extra_root_extension_required(st, child->name, nameLen)
        && !gltf_root_ext_core_required(textureMask, st, child->name, nameLen))
      return true;
  }

  return false;
}

static
bool
gltf_has_preserved_required_object_extension(GLTFExpState * __restrict st,
                                             uint32_t                  textureMask) {
  size_t i;

  for (i = 0; i < st->preservedExtensions.count; i++) {
    GLTFExpStringOut *ext;

    ext = &st->preservedExtensions.items[i];
    if (gltf_extra_has_root_extension(st, ext->name, ext->nameLen))
      continue;
    if (gltf_extra_root_extension_required(st, ext->name, ext->nameLen)
        && !gltf_root_ext_core_required(textureMask, st, ext->name, ext->nameLen))
      return true;
  }

  return false;
}

static
bool
gltf_count_material_variants(AkDoc * __restrict doc,
                             GLTFExpIndex * __restrict outCount) {
  AkMaterialVariant *variant;
  GLTFExpIndex       count;

  count = 0;
  for (variant = doc ? doc->materialVariants : NULL;
       variant;
       variant = variant->next) {
    if (count == GLTF_EXP_INDEX_NONE)
      return false;
    count++;
  }

  *outCount = count;

  return true;
}

static
bool
gltf_state_init(GLTFExpState * __restrict st, AkDoc * __restrict doc) {
  memset(st, 0, sizeof(*st));
  st->doc                    = doc;
  if (!gltf_count_material_variants(doc, &st->materialVariantCount))
    return false;
  st->nodeStack              = rb_newtree_ptr();
  st->nodeMap                = rb_newtree_ptr();
  st->meshes.map             = rb_newtree_ptr();
  st->textures.map           = rb_newtree_ptr();
  st->images.map             = rb_newtree_ptr();
  st->samplers.map           = rb_newtree_ptr();
  st->cameras.map            = rb_newtree_ptr();
  st->lights.map             = rb_newtree_ptr();
  st->skins.map              = rb_newtree_ptr();
  st->skinJointRoots.map     = rb_newtree_ptr();
  st->sceneSkinJointRoots.map = rb_newtree_ptr();
  st->accessors.accessorMap  = rb_newtree_ptr();
  st->accessors.primitiveMap = rb_newtree_ptr();
  st->accessors.rawMap       = rb_newtree_ptr();

  if (st->nodeStack && st->nodeMap && st->meshes.map
      && st->textures.map
      && st->images.map && st->samplers.map && st->cameras.map
      && st->lights.map && st->skins.map && st->skinJointRoots.map
      && st->sceneSkinJointRoots.map
      && st->accessors.accessorMap && st->accessors.primitiveMap
      && st->accessors.rawMap)
    return true;

  gltf_state_destroy(st);

  return false;
}

static
void
gltf_write_root_extension_name(GLTFExpWriter * __restrict w,
                               bool          * __restrict comma,
                               const char    * __restrict name,
                               size_t                     nameLen) {
  if (*comma)
    gltf_w_ch(w, ',');
  gltf_w_qstr_len(w, name, nameLen);
  *comma = true;
}

static
void
gltf_write_preserved_root_extensions_used(GLTFExpWriter * __restrict w,
                                          GLTFExpState  * __restrict st,
                                          uint32_t                   materialMask,
                                          uint32_t                   textureMask,
                                          bool          * __restrict comma) {
  AkTreeNode *root;
  AkTreeNode *child;

  root = gltf_extra_root_extensions_node(st);
  for (child = root ? root->chld : NULL; child; child = child->next) {
    size_t nameLen;

    if (!child->name)
      continue;

    nameLen = strlen(child->name);
    if (gltf_root_ext_is_core_payload(child->name, nameLen))
      continue;

    if (gltf_root_ext_core_used(st,
                                materialMask,
                                textureMask,
                                child->name,
                                nameLen))
      continue;

    gltf_write_root_extension_name(w, comma, child->name, nameLen);
  }
}

static
void
gltf_write_preserved_object_extensions_used(GLTFExpWriter * __restrict w,
                                            GLTFExpState  * __restrict st,
                                            uint32_t                   materialMask,
                                            uint32_t                   textureMask,
                                            bool          * __restrict comma) {
  size_t i;

  for (i = 0; i < st->preservedExtensions.count; i++) {
    GLTFExpStringOut *ext;

    ext = &st->preservedExtensions.items[i];
    if (gltf_extra_has_root_extension(st, ext->name, ext->nameLen))
      continue;
    if (gltf_root_ext_core_used(st,
                                materialMask,
                                textureMask,
                                ext->name,
                                ext->nameLen))
      continue;

    gltf_write_root_extension_name(w, comma, ext->name, ext->nameLen);
  }
}

static
void
gltf_write_preserved_root_extensions_required(GLTFExpWriter * __restrict w,
                                              GLTFExpState  * __restrict st,
                                              uint32_t                   textureMask,
                                              bool          * __restrict comma) {
  AkTreeNode *root;
  AkTreeNode *child;

  root = gltf_extra_root_extensions_node(st);
  for (child = root ? root->chld : NULL; child; child = child->next) {
    size_t nameLen;

    if (!child->name)
      continue;

    nameLen = strlen(child->name);
    if (gltf_root_ext_is_core_payload(child->name, nameLen))
      continue;

    if (!gltf_extra_root_extension_required(st, child->name, nameLen)
        || gltf_root_ext_core_required(textureMask, st, child->name, nameLen))
      continue;

    gltf_write_root_extension_name(w, comma, child->name, nameLen);
  }
}

static
void
gltf_write_preserved_object_extensions_required(GLTFExpWriter * __restrict w,
                                                GLTFExpState  * __restrict st,
                                                uint32_t                   textureMask,
                                                bool          * __restrict comma) {
  size_t i;

  for (i = 0; i < st->preservedExtensions.count; i++) {
    GLTFExpStringOut *ext;

    ext = &st->preservedExtensions.items[i];
    if (gltf_extra_has_root_extension(st, ext->name, ext->nameLen))
      continue;
    if (!gltf_extra_root_extension_required(st, ext->name, ext->nameLen))
      continue;
    if (gltf_root_ext_core_required(textureMask, st, ext->name, ext->nameLen))
      continue;

    gltf_write_root_extension_name(w, comma, ext->name, ext->nameLen);
  }
}

static
void
gltf_write_preserved_root_extensions(GLTFExpWriter * __restrict w,
                                     GLTFExpState  * __restrict st,
                                     bool          * __restrict comma) {
  AkTreeNode *root;
  AkTreeNode *child;

  root = gltf_extra_root_extensions_node(st);
  for (child = root ? root->chld : NULL; child; child = child->next) {
    size_t nameLen;

    if (!child->name)
      continue;

    nameLen = strlen(child->name);
    if (gltf_root_ext_is_core_payload(child->name, nameLen))
      continue;

    if (*comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, child->name, nameLen);
    gltf_write_extra_json_value(w, child);
    *comma = true;
  }
}

static
void
gltf_write_root_extensions_used(GLTFExpWriter * __restrict w,
                                GLTFExpState  * __restrict st,
                                uint32_t                   materialMask,
                                uint32_t                   textureMask) {
  bool comma;

  comma = false;
  gltf_w_key(w, _s_gltf_extensionsUsed, _s_gltf_extensionsUsed_len);
  gltf_w_ch(w, '[');
  if (st->lights.count > 0) {
    gltf_w_qstr_len(w,
                    _s_gltf_KHR_lights_punctual,
                    _s_gltf_KHR_lights_punctual_len);
    comma = true;
  }
  gltf_write_material_extensions_used(w, materialMask, &comma);
  gltf_write_texture_extensions_used(w, textureMask, &comma);
  if (gltf_has_material_variants(st)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_qstr_len(w,
                    _s_gltf_KHR_materials_variants,
                    _s_gltf_KHR_materials_variants_len);
    comma = true;
  }
  if (st->usesNodeVisibility) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_qstr_len(w,
                    _s_gltf_KHR_node_visibility,
                    _s_gltf_KHR_node_visibility_len);
    comma = true;
  }
  if (st->usesGpuInstancing) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_qstr_len(w,
                    _s_gltf_EXT_mesh_gpu_instancing,
                    _s_gltf_EXT_mesh_gpu_instancing_len);
    comma = true;
  }
  if (st->usesMeshQuantization) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_qstr_len(w,
                    _s_gltf_KHR_mesh_quantization,
                    _s_gltf_KHR_mesh_quantization_len);
    comma = true;
  }
  if (st->usesAnimationPointer) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_qstr_len(w,
                    _s_gltf_KHR_animation_pointer,
                    _s_gltf_KHR_animation_pointer_len);
    comma = true;
  }
  gltf_write_preserved_root_extensions_used(w,
                                            st,
                                            materialMask,
                                            textureMask,
                                            &comma);
  gltf_write_preserved_object_extensions_used(w,
                                              st,
                                              materialMask,
                                              textureMask,
                                              &comma);
  gltf_w_ch(w, ']');
}

static
void
gltf_write_root_extensions_required(GLTFExpWriter * __restrict w,
                                    GLTFExpState  * __restrict st,
                                    uint32_t                   textureMask) {
  bool comma;

  comma = false;
  gltf_w_key(w, _s_gltf_extensionsRequired, _s_gltf_extensionsRequired_len);
  gltf_w_ch(w, '[');

  if (textureMask & GLTF_EXP_TEX_EXT_BASISU) {
    gltf_w_qstr_len(w,
                    _s_gltf_KHR_texture_basisu,
                    _s_gltf_KHR_texture_basisu_len);
    comma = true;
  }

  if (textureMask & GLTF_EXP_TEX_EXT_WEBP) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_qstr_len(w,
                    _s_gltf_EXT_texture_webp,
                    _s_gltf_EXT_texture_webp_len);
    comma = true;
  }

  if (st->usesMeshQuantization) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_qstr_len(w,
                    _s_gltf_KHR_mesh_quantization,
                    _s_gltf_KHR_mesh_quantization_len);
    comma = true;
  }

  gltf_write_preserved_root_extensions_required(w, st, textureMask, &comma);
  gltf_write_preserved_object_extensions_required(w, st, textureMask, &comma);
  gltf_w_ch(w, ']');
}

static
bool
gltf_has_required_extensions(GLTFExpState * __restrict st,
                             uint32_t                  textureMask) {
  return st->usesMeshQuantization
         || (textureMask & GLTF_EXP_TEX_EXT_BASISU) != 0
         || (textureMask & GLTF_EXP_TEX_EXT_WEBP) != 0
         || gltf_has_preserved_required_root_extension(st, textureMask)
         || gltf_has_preserved_required_object_extension(st, textureMask);
}

static
void
gltf_write_root_extensions(GLTFExpWriter * __restrict w,
                           GLTFExpState  * __restrict st) {
  bool comma;

  comma = false;
  gltf_w_key(w, _s_gltf_extensions, _s_gltf_extensions_len);
  gltf_w_ch(w, '{');

  if (st->lights.count > 0) {
    gltf_write_lights_punctual_extension(w, st);
    comma = true;
  }

  if (gltf_has_material_variants(st)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_write_material_variants_extension(w, st);
    comma = true;
  }

  gltf_write_preserved_root_extensions(w, st, &comma);
  gltf_w_ch(w, '}');
}

static
const char*
gltf_asset_version(size_t * __restrict len) {
  switch ((AkGltfExportVersion)ak_opt_get(AK_OPT_GLTF_EXPORT_VERSION)) {
    case AK_GLTF_EXPORT_VERSION_2_1:
      *len = _s_gltf_version21_len;
      return _s_gltf_version21;
    case AK_GLTF_EXPORT_VERSION_AUTO:
    case AK_GLTF_EXPORT_VERSION_2_0:
    default:
      *len = _s_gltf_version2_len;
      return _s_gltf_version2;
  }
}

static
void
gltf_write_json_payload(GLTFExpState  * __restrict st,
                        GLTFExpWriter * __restrict w) {
  uint32_t materialExtensions;
  uint32_t textureExtensions;
  const char *generator;
  const char *version;
  size_t   versionLen;
  bool     comma;

  materialExtensions = gltf_material_extensions_mask(st);
  textureExtensions  = gltf_texture_extensions_mask(st);
  version            = gltf_asset_version(&versionLen);
  gltf_w_ch(w, '{');

  gltf_w_key(w, _s_gltf_asset, _s_gltf_asset_len);
  gltf_w_ch(w, '{');
  gltf_w_key(w, _s_gltf_version, _s_gltf_version_len);
  gltf_w_qstr_len(w, version, versionLen);
  gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_generator, _s_gltf_generator_len);
  generator = (const char *)ak_opt_get(AK_OPT_EXPORT_AUTHORING_TOOL);
  if (!generator || !generator[0])
    generator = AK_AUTHORING_TOOL;
  gltf_w_qstr(w, generator);
  gltf_w_ch(w, '}');
  comma = true;

  if (st->lights.count > 0
      || materialExtensions != 0
      || textureExtensions != 0
      || gltf_has_material_variants(st)
      || st->usesNodeVisibility
      || st->usesGpuInstancing
      || st->usesMeshQuantization
      || st->usesAnimationPointer
      || gltf_has_preserved_root_extension(st)
      || st->preservedExtensions.count > 0) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_write_root_extensions_used(w,
                                    st,
                                    materialExtensions,
                                    textureExtensions);
    comma = true;
  }

  if (gltf_has_required_extensions(st, textureExtensions)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_write_root_extensions_required(w, st, textureExtensions);
    comma = true;
  }

  if (st->accessors.count > 0) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_write_buffers(w, st);
    comma = true;

    gltf_w_ch(w, ',');
    gltf_write_buffer_views(w, st);

    if (gltf_has_accessors(st)) {
      gltf_w_ch(w, ',');
      gltf_write_accessors(w, st);
    }
  }

  if (st->meshes.count > 0) {
    if (st->samplers.count > 0) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_write_samplers(w, st);
      comma = true;
    }

    if (st->images.count > 0) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_write_images(w, st);
      comma = true;
    }

    if (st->textures.count > 0) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_write_textures(w, st);
      comma = true;
    }

    if (st->materials.count > 0) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_write_materials(w, st);
      comma = true;
    }

    if (comma)
      gltf_w_ch(w, ',');
    gltf_write_meshes(w, st);
    comma = true;
  }

  if (st->cameras.count > 0) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_write_cameras(w, st);
    comma = true;
  }

  if (st->skins.count > 0) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_write_skins(w, st);
    comma = true;
  }

  if (st->lights.count > 0
      || gltf_has_material_variants(st)
      || gltf_has_preserved_root_extension(st)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_write_root_extensions(w, st);
    comma = true;
  }

  if (st->animations.count > 0) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_write_animations(w, st);
    comma = true;
  }

  if (comma)
    gltf_w_ch(w, ',');
  gltf_write_scenes(w, st);
  comma = true;

  if (st->nodes.count > 0) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_write_nodes(w, st);
  }

  gltf_w_ch(w, '}');
  gltf_w_ch(w, '\n');
}

static
AkResult
gltf_write_json(GLTFExpState * __restrict st,
                const char   * __restrict filepath) {
  GLTFExpWriter w;

  memset(&w, 0, sizeof(w));
  w.result = AK_OK;
  w.file   = fopen(filepath, "wb");
  if (!w.file)
    return AK_EBADF;
  gltf_configure_file_buffer(w.file);

  gltf_write_json_payload(st, &w);
  gltf_w_flush(&w);

  if (fclose(w.file) != 0 && w.result == AK_OK)
    w.result = AK_ERR;

  if (w.result != AK_OK)
    remove(filepath);

  return w.result;
}

static
bool
gltf_align4_checked(size_t len, size_t * __restrict aligned) {
  if (len > (size_t)-1 - 3u)
    return false;

  *aligned = (len + 3u) & ~(size_t)3u;

  return true;
}

static
bool
gltf_write_padding(FILE * __restrict file, unsigned char pad, size_t count) {
  unsigned char bytes[4] = {0, 0, 0, 0};

  if (count == 0)
    return true;

  bytes[0] = pad;
  bytes[1] = pad;
  bytes[2] = pad;
  bytes[3] = pad;

  return fwrite(bytes, 1, count, file) == count;
}

static
AkResult
gltf_write_glb(GLTFExpState * __restrict st,
               const char   * __restrict filepath) {
  GLTFExpWriter w;
  FILE         *file;
  unsigned char header[20];
  size_t        jsonLen;
  size_t        jsonAligned;
  size_t        jsonPadLen;
  size_t        binLen;
  size_t        binAligned;
  size_t        binPadLen;
  size_t        totalLen;
  bool          ok;

  if (!gltf_w_init_memory(&w, GLTF_EXP_WRITER_CAP))
    return AK_ERR;

  gltf_write_json_payload(st, &w);
  gltf_w_flush(&w);
  if (w.result != AK_OK) {
    gltf_w_free(&w);
    return w.result;
  }

  jsonLen = w.memLen;
  binLen  = st->binByteLength;
  if (!gltf_align4_checked(jsonLen, &jsonAligned)
      || !gltf_align4_checked(binLen, &binAligned)
      || jsonAligned > UINT32_MAX
      || binAligned > UINT32_MAX) {
    gltf_w_free(&w);
    return AK_ERR;
  }

  jsonPadLen = jsonAligned - jsonLen;
  binPadLen  = binLen > 0 ? binAligned - binLen : 0;

  if (jsonAligned > (size_t)-1 - 20u) {
    gltf_w_free(&w);
    return AK_ERR;
  }
  totalLen = 20u + jsonAligned;

  if (binLen > 0) {
    if (binAligned > (size_t)-1 - totalLen - 8u) {
      gltf_w_free(&w);
      return AK_ERR;
    }
    totalLen += 8u + binAligned;
  }

  if (totalLen > UINT32_MAX) {
    gltf_w_free(&w);
    return AK_ERR;
  }

  file = fopen(filepath, "wb");
  if (!file) {
    gltf_w_free(&w);
    return AK_EBADF;
  }
  gltf_configure_file_buffer(file);

  io_store_u32le(header + 0, GLTF_GLB_MAGIC);
  io_store_u32le(header + 4, GLTF_GLB_VER);
  io_store_u32le(header + 8, (uint32_t)totalLen);
  io_store_u32le(header + 12, (uint32_t)jsonAligned);
  io_store_u32le(header + 16, GLTF_GLB_JSON);

  ok = fwrite(header, 1, sizeof(header), file) == sizeof(header)
       && fwrite(w.mem, 1, jsonLen, file) == jsonLen
       && gltf_write_padding(file, 0x20u, jsonPadLen);

  if (ok && binLen > 0) {
    unsigned char binHeader[8];

    io_store_u32le(binHeader + 0, (uint32_t)binAligned);
    io_store_u32le(binHeader + 4, GLTF_GLB_BIN);
    ok = fwrite(binHeader, 1, sizeof(binHeader), file) == sizeof(binHeader)
         && gltf_write_bin_payload(st, file)
         && gltf_write_padding(file, 0u, binPadLen);
  }

  gltf_w_free(&w);

  if (fclose(file) != 0)
    ok = false;

  if (!ok) {
    remove(filepath);
    return AK_ERR;
  }

  return AK_OK;
}

static
AkResult
gltf_failed_result(GLTFExpState * __restrict st) {
  return st->failResult != AK_OK ? st->failResult : AK_ERR;
}

AK_HIDE
AkResult
gltf_export(AkDoc * __restrict doc, const char * __restrict filepath) {
  GLTFExpState st;
  AkResult     result;

  if (!gltf_state_init(&st, doc))
    return AK_ERR;

  st.outDir = io_path_output_dir_dup(filepath);
  if (!st.outDir) {
    gltf_state_destroy(&st);
    return AK_ERR;
  }

  gltf_plan(&st);

  if (st.failed)
    result = gltf_failed_result(&st);
  else if ((result = gltf_write_bin(&st, filepath)) == AK_OK) {
    result = gltf_write_json(&st, filepath);
    if (result != AK_OK && st.binPath)
      remove(st.binPath);
  }

  gltf_state_destroy(&st);

  return result;
}

AK_HIDE
AkResult
gltf_export_glb(AkDoc * __restrict doc, const char * __restrict filepath) {
  GLTFExpState st;
  AkResult     result;

  if (!gltf_state_init(&st, doc))
    return AK_ERR;

  st.outDir = io_path_output_dir_dup(filepath);
  if (!st.outDir) {
    gltf_state_destroy(&st);
    return AK_ERR;
  }

  st.glb = true;
  gltf_plan(&st);

  if (st.failed)
    result = gltf_failed_result(&st);
  else if ((result = gltf_prepare_bin(&st)) == AK_OK)
    result = gltf_write_glb(&st, filepath);

  gltf_state_destroy(&st);

  return result;
}
