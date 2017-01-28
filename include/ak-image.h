/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_image_h
#define ak_image_h

typedef struct AkImageFormat {
  const char       *space;
  const char       *exact;
  AkChannelFormat   channel;
  AkRangeFormat     range;
  AkPrecisionFormat precision;
} AkImageFormat;

typedef struct AkImage2d {
  AkSizeExact   * sizeExact;
  AkSizeRatio   * sizeRatio;
  AkMips        * mips;
  const char    * unnormalized;
  AkImageFormat * format;
  AkInitFrom    * initFrom;
  long            arrayLen;
} AkImage2d;

typedef struct AkImage3d {
  struct {
    AkUInt width;
    AkUInt height;
    AkUInt depth;
  } size;

  AkMips          mips;
  long            arrayLen;
  AkImageFormat * format;
  AkInitFrom    * initFrom;
} AkImage3d;

typedef struct AkImageCube {
  AkUInt          width;
  AkMips          mips;
  long            arrayLen;
  AkImageFormat * format;
  AkInitFrom    * initFrom;
} AkImageCube;

typedef struct AkImage {
  ak_asset_base

  /* const char * id;  */
  /* const char * sid; */

  const char  * name;
  AkInitFrom  * initFrom;
  AkImage2d   * image2d;
  AkImage3d   * image3d;
  AkImageCube * cube;
  AkTree      * extra;

  struct AkImage * next;

  AkBool renderableShare;
} AkImage;

#endif /* ak_image_h */
