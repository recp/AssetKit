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

#ifndef assetkit_gltf_exp_material_h
#define assetkit_gltf_exp_material_h

#include "common.h"
#include "writer.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum GLTFExpMaterialExtensionMask {
  GLTF_EXP_MAT_EXT_UNLIT                = 1u << 0,
  GLTF_EXP_MAT_EXT_EMISSIVE_STRENGTH    = 1u << 1,
  GLTF_EXP_MAT_EXT_IOR                  = 1u << 2,
  GLTF_EXP_MAT_EXT_CLEARCOAT            = 1u << 3,
  GLTF_EXP_MAT_EXT_SPECULAR             = 1u << 4,
  GLTF_EXP_MAT_EXT_TRANSMISSION         = 1u << 5,
  GLTF_EXP_MAT_EXT_SHEEN                = 1u << 6,
  GLTF_EXP_MAT_EXT_IRIDESCENCE          = 1u << 7,
  GLTF_EXP_MAT_EXT_VOLUME               = 1u << 8,
  GLTF_EXP_MAT_EXT_ANISOTROPY           = 1u << 9,
  GLTF_EXP_MAT_EXT_DISPERSION           = 1u << 10,
  GLTF_EXP_MAT_EXT_DIFFUSE_TRANSMISSION = 1u << 11,
  GLTF_EXP_MAT_EXT_TEXTURE_TRANSFORM    = 1u << 12,
  GLTF_EXP_MAT_EXT_SPECULAR_GLOSSINESS  = 1u << 13,
  GLTF_EXP_MAT_EXT_VOLUME_SCATTER       = 1u << 14
} GLTFExpMaterialExtensionMask;

typedef enum GLTFExpTextureExtensionMask {
  GLTF_EXP_TEX_EXT_BASISU = 1u << 0,
  GLTF_EXP_TEX_EXT_WEBP   = 1u << 1
} GLTFExpTextureExtensionMask;

GLTFExpIndex
gltf_material_index(GLTFExpState * __restrict st,
                    AkMaterial   * __restrict material,
                    AkMeshPrimitive * __restrict prim,
                    AkInstanceGeometry * __restrict inst);

bool
gltf_material_is_default_noop(AkMaterial * __restrict material);

uint32_t
gltf_material_extensions_mask(GLTFExpState * __restrict st);

void
gltf_write_material_extensions_used(GLTFExpWriter * __restrict w,
                                    uint32_t                   mask,
                                    bool * __restrict          comma);

uint32_t
gltf_texture_extensions_mask(GLTFExpState * __restrict st);

void
gltf_write_texture_extensions_used(GLTFExpWriter * __restrict w,
                                   uint32_t                   mask,
                                   bool * __restrict          comma);

bool
gltf_has_material_variants(GLTFExpState * __restrict st);

void
gltf_write_material_variants_extension(GLTFExpWriter * __restrict w,
                                       GLTFExpState  * __restrict st);

void
gltf_write_samplers(GLTFExpWriter * __restrict w,
                    GLTFExpState  * __restrict st);

void
gltf_write_images(GLTFExpWriter * __restrict w,
                  GLTFExpState  * __restrict st);

void
gltf_write_textures(GLTFExpWriter * __restrict w,
                    GLTFExpState  * __restrict st);

void
gltf_write_materials(GLTFExpWriter * __restrict w,
                     GLTFExpState  * __restrict st);

#endif /* assetkit_gltf_exp_material_h */
