/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_image_h
#define ak_image_h

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
  float width;
  float height;
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

void
ak_imageLoad(AkImage * __restrict image);

#endif /* ak_image_h */
