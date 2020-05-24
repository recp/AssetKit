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

#ifndef dae14_h
#define dae14_h

#include "../common.h"

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
dae14_loadjobs_add(DAEState   * __restrict  dst,
                   void       *  __restrict parent,
                   void       * __restrict  value,
                   AkDae14LoadJobType       type);

void
_assetkit_hide
dae14_loadjobs_finish(DAEState * __restrict dst);

#endif /* dae14_h */
