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

#ifndef assetkit_image_h
#define assetkit_image_h

#include "common.h"

struct AkInitFrom;

typedef enum AkImageType {
  AK_IMAGE_TYPE_1D   = 0,
  AK_IMAGE_TYPE_2D   = 1,
  AK_IMAGE_TYPE_3D   = 2,
  AK_IMAGE_TYPE_CUBE = 3
} AkImageType;

typedef struct AkImageData {
  void    *data;
  uint32_t width;
  uint32_t height;
  AkEnum   comp;
} AkImageData;

typedef struct AkSizeExact {
  uint32_t width;
  uint32_t height;
} AkSizeExact;

typedef struct AkSizeRatio {
  float width;
  float height;
} AkSizeRatio;

typedef struct AkMips {
  uint32_t levels;
  bool     autoGenerate;
} AkMips;

typedef struct AkImageFormat {
  const char       *space;
  const char       *exact;
  AkChannelFormat   channel;
  AkRangeFormat     range;
  AkPrecisionFormat precision;
} AkImageFormat;

typedef struct AkImageSize {
  AkUInt width;
  AkUInt height;
  AkUInt depth;
} AkImageSize;

typedef struct AkImageBase {
  AkImageFormat     *format;
  struct AkInitFrom *initFrom;
  long               arrayLen;
  AkImageType        type;
} AkImageBase;

typedef struct AkImage2d {
  AkImageBase  base;
  AkSizeExact *sizeExact;
  AkSizeRatio *sizeRatio;
  AkMips      *mips;
  const char  *unnormalized;
} AkImage2d;

typedef struct AkImage3d {
  AkImageBase base;
  AkImageSize size;
  AkMips      mips;
} AkImage3d;

typedef struct AkImageCube {
  AkImageBase base;
  uint32_t    width;
  AkMips      mips;
} AkImageCube;

typedef struct AkImage {
  /* const char * id;  */
  /* const char * sid; */
  const char        *name;
  struct AkInitFrom *initFrom;
  AkImageBase       *image;
  AkImageData       *data;
  AkTree            *extra;
  struct AkImage    *next;

  AkBool          renderable;
  AkBool          renderableShare;
} AkImage;

AK_EXPORT
void
ak_imageLoad(AkImage * __restrict image);

/* Loader Configurator */
typedef void* (*AkImageLoadFromFileFn)(const char * __restrict path,
                                       int        * __restrict width,
                                       int        * __restrict height,
                                       int        * __restrict components);
typedef void* (*AkImageLoadFromMemoryFn)(const char * __restrict data,
                                         size_t                  len,
                                         int        * __restrict width,
                                         int        * __restrict height,
                                         int        * __restrict components);
typedef void (*AkImageFlipVerticallyOnLoad)(bool flip);

AK_EXPORT
void
ak_imageInitLoader(AkImageLoadFromFileFn       fromFile,
                   AkImageLoadFromMemoryFn     fromMemory,
                   AkImageFlipVerticallyOnLoad flipper);

#endif /* assetkit_image_h */
