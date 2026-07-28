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
#include "core-types.h"

typedef enum AkChannelFormat {
  AK_CHANNEL_FORMAT_RGB  = 1,
  AK_CHANNEL_FORMAT_RGBA = 2,
  AK_CHANNEL_FORMAT_RGBE = 3,
  AK_CHANNEL_FORMAT_L    = 4,
  AK_CHANNEL_FORMAT_LA   = 5,
  AK_CHANNEL_FORMAT_D    = 6,
  AK_CHANNEL_FORMAT_XYZ  = 7,
  AK_CHANNEL_FORMAT_XYZW = 8
} AkChannelFormat;

typedef enum AkRangeFormat {
  AK_RANGE_FORMAT_SNORM = 1,
  AK_RANGE_FORMAT_UNORM = 2,
  AK_RANGE_FORMAT_SINT  = 3,
  AK_RANGE_FORMAT_UINT  = 4,
  AK_RANGE_FORMAT_FLOAT = 5
} AkRangeFormat;

typedef enum AkPrecisionFormat {
  AK_PRECISION_FORMAT_DEFAULT = 1,
  AK_PRECISION_FORMAT_LOW     = 2,
  AK_PRECISION_FORMAT_MID     = 3,
  AK_PRECISION_FORMAT_HIGHT   = 4,
  AK_PRECISION_FORMAT_MAX     = 5
} AkPrecisionFormat;

typedef enum AkImageType {
  AK_IMAGE_TYPE_1D   = 0,
  AK_IMAGE_TYPE_2D   = 1,
  AK_IMAGE_TYPE_3D   = 2,
  AK_IMAGE_TYPE_CUBE = 3
} AkImageType;

typedef enum AkFace {
  AK_FACE_UNSPECIFIED = 0,
  AK_FACE_POSITIVE_X  = 1,
  AK_FACE_NEGATIVE_X  = 2,
  AK_FACE_POSITIVE_Y  = 3,
  AK_FACE_NEGATIVE_Y  = 4,
  AK_FACE_POSITIVE_Z  = 5,
  AK_FACE_NEGATIVE_Z  = 6
} AkFace;

typedef struct AkImageData {
  void    *data;
  uint32_t width;
  uint32_t height;
  AkEnum   comp;
} AkImageData;

typedef struct AkHexData {
  const char *format;
  const char *hexval; /* hex value    */
  void       *data;   /* binary value */
} AkHexData;

typedef enum AkImageSourceType {
  AK_IMAGE_SOURCE_NONE   = 0,
  AK_IMAGE_SOURCE_URI    = 1,
  AK_IMAGE_SOURCE_BUFFER = 2,
  AK_IMAGE_SOURCE_HEX    = 3
} AkImageSourceType;

typedef struct AkImageSource {
  struct AkImageSource *next;
  const char           *uri;
  const char           *resolvedPath; /* derived/cache, not source selector */
  AkHexData            *hex;
  AkBuffer             *buffer;
  const char           *mimeType;

  /* Selects the authoritative source field:
     URI -> uri, BUFFER -> buffer, HEX -> hex. The remaining subresource
     fields annotate the source; they do not select where image data comes
     from. */
  AkImageSourceType     type;
  AkFace                face;
  AkUInt                mipIndex;
  AkUInt                depth;
  AkInt                 arrayIndex;
  AkBool                generateMips;
} AkImageSource;

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
  AkImageFormat *format;
  AkImageSource *source;
  long           arrayLen;
  AkImageType    type;
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
  const char     *name;
  AkImageSource  *source;
  AkImageBase    *image;
  AkImageData    *data;
  AkTree         *extra;
  struct AkImage *next;

  AkBool          renderable;
  AkBool          renderableShare;

  bool            flipOnLoad;
} AkImage;

/**
 * Resolves and caches the filesystem path for a URI-backed image without
 * decoding the image. Returns NULL for non-URI sources or unresolvable paths.
 */
AK_EXPORT
const char*
ak_imageResolvePath(AkImage * __restrict image);

AK_EXPORT
void
ak_imageLoad(AkImage * __restrict image);

/* Loader Configurator */
typedef AkImageData* (*AkImageLoadFromFileFn)(AkHeap     * __restrict heap,
                                              AkImage    * __restrict image,
                                              const char * __restrict path,
                                              bool                    flipVertically);

typedef AkImageData* (*AkImageLoadFromMemoryFn)(AkHeap   * __restrict heap,
                                                AkImage  * __restrict image,
                                                AkBuffer * __restrict buff,
                                                bool                  flipVertically);

typedef void  (*AkImageFlipVerticallyOnLoad)(bool flip);

AK_EXPORT
void
ak_imageInitLoader(AkImageLoadFromFileFn   fromFile,
                   AkImageLoadFromMemoryFn fromMemory);

#endif /* assetkit_image_h */
