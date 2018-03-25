/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae14_h
#define dae14_h

#include "../../../include/ak/assetkit.h"
#include "../dae_common.h"

typedef enum AkDae14LoadJobType {
  AK_DAE14_LOADJOB_SURFACE = 1
} AkDae14LoadJobType;

typedef struct AkDae14LoadJob {
  struct AkDae14LoadJob *prev;
  struct AkDae14LoadJob *next;
  void                  *parent;
  void                  *value;
  AkDae14LoadJobType     type;
} AkDae14LoadJob;

typedef struct AkDae14SurfaceFrom {
  const char *image;
  AkUInt      mip;
  AkUInt      slice;
  AkFace      face;
} AkDae14SurfaceFrom;

typedef struct AkDae14Surface {
  AkInstanceBase *instanceImage;
  AkTree         *extra;
  const char     *format;
  AkImageFormat   formatHint;
  AkImageSize     size;
  float           viewportRatio[2];
  int             mipLevels;
  bool            mipmapGenerate;

  /* initializers */
  bool                  initAsNull;
  bool                  initAsTarget;
  AkDae14SurfaceFrom   *initFrom;
} AkDae14Surface;

void
_assetkit_hide
ak_dae14_loadjobs_add(AkXmlState * __restrict  xst,
                      void       *  __restrict parent,
                      void       * __restrict  value,
                      AkDae14LoadJobType       type);

void
_assetkit_hide
ak_dae14_loadjobs_finish(AkXmlState * __restrict xst);

#endif /* dae14_h */
