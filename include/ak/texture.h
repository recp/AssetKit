/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_texture_h
#define ak_texture_h

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

  unsigned long   maxAnisotropy;
  unsigned long   mipMaxLevel;
  unsigned long   mipMinLevel;
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
  /* struct AkInput   *coordInput; */
  const char       *coordInputName;
  int               slot;
  
//  /* TODO: WILL BE DELETED */
//  const char       *texcoord;
//  void             *extra;
} AkTextureRef;

#endif /* ak_texture_h */
