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

#ifndef assetkit_texture_h
#define assetkit_texture_h

#include "common.h"
#include "image.h"

typedef enum AkWrapMode {
  AK_WRAP_MODE_WRAP        = 1,
  AK_WRAP_MODE_MIRROR      = 2,
  AK_WRAP_MODE_CLAMP       = 3,
  AK_WRAP_MODE_BORDER      = 4,
  AK_WRAP_MODE_MIRROR_ONCE = 5
} AkWrapMode;

typedef enum AkMinFilter {
  AK_MINFILTER_LINEAR       = 0,
  AK_MINFILTER_NEAREST      = 1,
  AK_MINFILTER_ANISOTROPIC  = 2,

  AK_LINEAR_MIPMAP_NEAREST  = 2,
  AK_LINEAR_MIPMAP_LINEAR   = 3,
  AK_NEAREST_MIPMAP_NEAREST = 4,
  AK_NEAREST_MIPMAP_LINEAR  = 5
} AkMinFilter;

typedef enum AkMagFilter {
  AK_MAGFILTER_LINEAR       = 0,
  AK_MAGFILTER_NEAREST      = 1
} AkMagFilter;

typedef enum AkMipFilter {
  AK_MIPFILTER_LINEAR  = 0,
  AK_MIPFILTER_NONE    = 1,
  AK_MIPFILTER_NEAREST = 2
} AkMipFilter;

typedef struct AkSampler {
  const char     *uniformName;
  const char     *coordInputName;
  AkColor        *borderColor;
//  AkInstanceBase *instanceImage;
  AkTree         *extra;
  const char     *name;

  AkWrapMode      wrapS;
  AkWrapMode      wrapT;
  AkWrapMode      wrapP;

  AkMinFilter     minfilter;
  AkMagFilter     magfilter;
  AkMipFilter     mipfilter;

  uint32_t        maxAnisotropy;
  uint32_t        mipMaxLevel;
  uint32_t        mipMinLevel;
  float           mipBias;
} AkSampler;

typedef struct AkTexture {
  struct AkTexture *next;
  AkImage          *image;
  AkSampler        *sampler;
  const char       *name;
  AkTypeId          type;
} AkTexture;

typedef struct AkTextureRef {
  struct AkTexture *texture;
  const char       *texcoord; /* to bind texture to input coord dynamically */
  const char       *coordInputName;
  int               slot;
} AkTextureRef;

#endif /* assetkit_texture_h */
