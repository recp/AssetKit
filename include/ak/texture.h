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
#include "core-types.h"
#include "image.h"
#include "type.h"

typedef enum AkWrapMode {
  AK_WRAP_MODE_UNSPECIFIED = 0,
  AK_WRAP_MODE_WRAP        = 1,
  AK_WRAP_MODE_MIRROR      = 2,
  AK_WRAP_MODE_CLAMP       = 3,
  AK_WRAP_MODE_BORDER      = 4,
  AK_WRAP_MODE_MIRROR_ONCE = 5
} AkWrapMode;

typedef enum AkMinFilter {
  AK_MINFILTER_UNSPECIFIED           = 0,
  AK_MINFILTER_NONE                  = 1,
  AK_MINFILTER_NEAREST               = 2,
  AK_MINFILTER_LINEAR                = 3,
  AK_MINFILTER_ANISOTROPIC           = 4,
  AK_MINFILTER_NEAREST_MIPMAP_NEAREST = 5,
  AK_MINFILTER_LINEAR_MIPMAP_NEAREST  = 6,
  AK_MINFILTER_NEAREST_MIPMAP_LINEAR  = 7,
  AK_MINFILTER_LINEAR_MIPMAP_LINEAR   = 8,

  /* Source compatibility for the original unprefixed spellings. */
  AK_NEAREST_MIPMAP_NEAREST = AK_MINFILTER_NEAREST_MIPMAP_NEAREST,
  AK_LINEAR_MIPMAP_NEAREST  = AK_MINFILTER_LINEAR_MIPMAP_NEAREST,
  AK_NEAREST_MIPMAP_LINEAR  = AK_MINFILTER_NEAREST_MIPMAP_LINEAR,
  AK_LINEAR_MIPMAP_LINEAR   = AK_MINFILTER_LINEAR_MIPMAP_LINEAR
} AkMinFilter;

typedef enum AkMagFilter {
  AK_MAGFILTER_UNSPECIFIED = 0,
  AK_MAGFILTER_NONE        = 1,
  AK_MAGFILTER_NEAREST     = 2,
  AK_MAGFILTER_LINEAR      = 3
} AkMagFilter;

typedef enum AkMipFilter {
  AK_MIPFILTER_UNSPECIFIED = 0,
  AK_MIPFILTER_NONE        = 1,
  AK_MIPFILTER_NEAREST     = 2,
  AK_MIPFILTER_LINEAR      = 3
} AkMipFilter;

typedef enum AkTextureColorSpace {
  AK_TEXTURE_COLORSPACE_UNSPECIFIED = 0,
  AK_TEXTURE_COLORSPACE_LINEAR      = 1,
  AK_TEXTURE_COLORSPACE_SRGB        = 2
} AkTextureColorSpace;

typedef enum AkTextureChannels {
  AK_TEXTURE_CHANNEL_NONE = 0,
  AK_TEXTURE_CHANNEL_R    = 1 << 0,
  AK_TEXTURE_CHANNEL_G    = 1 << 1,
  AK_TEXTURE_CHANNEL_B    = 1 << 2,
  AK_TEXTURE_CHANNEL_A    = 1 << 3,
  AK_TEXTURE_CHANNEL_RGB  = AK_TEXTURE_CHANNEL_R
                            | AK_TEXTURE_CHANNEL_G
                            | AK_TEXTURE_CHANNEL_B,
  AK_TEXTURE_CHANNEL_RGBA = AK_TEXTURE_CHANNEL_RGB
                            | AK_TEXTURE_CHANNEL_A,
  AK_TEXTURE_CHANNEL_GB   = AK_TEXTURE_CHANNEL_G
                            | AK_TEXTURE_CHANNEL_B
} AkTextureChannels;

typedef struct AkSampler {
  struct AkSampler *next;
  const char       *uniformName;
  const char       *coordInputName;
  AkColor          *borderColor;
//  AkInstanceBase *instanceImage;
  AkTree           *extra;
  const char       *name;

  AkWrapMode        wrapS;
  AkWrapMode        wrapT;
  AkWrapMode        wrapP;

  AkMinFilter       minfilter;
  AkMagFilter       magfilter;
  AkMipFilter       mipfilter;

  uint32_t          maxAnisotropy;
  uint32_t          mipMaxLevel;
  uint32_t          mipMinLevel;
  float             mipBias;
} AkSampler;

typedef struct AkTexture {
  struct AkTexture *next;
  AkImage          *image;
  AkSampler        *sampler;
  const char       *name;
  AkTypeId          type;
} AkTexture;

typedef struct AkTextureTransform {
  AkFloat2    offset;
  float       rotation;
  AkFloat2    scale;
  int         slot;
  const char *coordInputName;
} AkTextureTransform;

typedef struct AkTextureRef {
  struct AkTexture   *texture;

  /* Source-side texcoord binding override, usually resolved from an object-
     or instance-level material binding. */
  const char         *texcoord;

  /* Canonical texture-coordinate binding. */
  const char         *coordInputName;
  int                 slot;

  AkTextureColorSpace colorSpace;
  AkTextureChannels   channels;

  /* Texture Transform */
  AkTextureTransform *transform;
} AkTextureRef;

/* KTX2/BasisU side-decoder ABI. Optional decoder shims and the glTF host
   include this public layout instead of duplicating it across libraries. */
typedef struct AkKTX2MipLevel {
  uint32_t width;
  uint32_t height;
  uint32_t byteOffset;
  uint32_t byteLength;
} AkKTX2MipLevel;

typedef struct AkKTX2DecodedImage {
  uint8_t        *data;
  size_t          dataLength;
  uint32_t        width;
  uint32_t        height;
  uint32_t        channels;
  uint32_t        mipCount;
  AkKTX2MipLevel *mips;
  uint32_t        padding[2];
} AkKTX2DecodedImage;

typedef int
(*AkKTX2DecodeFn)(const uint8_t      *data,
                  size_t              size,
                  AkKTX2DecodedImage *out);

typedef struct AkKTX2Decoder {
  void          *userdata;
  AkKTX2DecodeFn decode;
  void         (*close)(void *ud);
} AkKTX2Decoder;

typedef int
(*AkKTX2DecoderCreateFn)(AkKTX2Decoder *out);

AK_INLINE
AkTextureRef*
ak_texref_usage(AkTextureRef        *texref,
                AkTextureColorSpace  colorSpace,
                AkTextureChannels    channels) {
  if (texref) {
    texref->colorSpace = colorSpace;
    texref->channels   = channels;
  }
  return texref;
}

#ifdef __cglm__
AK_INLINE
mat4s
ak_textrans_mat4(AkTextureTransform *transform) {
  return glms_mat4_(textrans)(transform->scale[0],
                              transform->scale[1],
                              transform->rotation,
                              transform->offset[0],
                              transform->offset[1]);
}
#endif

#endif /* assetkit_texture_h */
