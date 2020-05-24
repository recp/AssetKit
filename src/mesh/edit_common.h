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

#ifndef ak_mesh_edit_common_h
#define ak_mesh_edit_common_h

#include "../common.h"

typedef struct AkPrimProxy {
  struct AkPrimProxy *next;
  AkMeshPrimitive    *orig;
  uint8_t            *flg;
  AkUIntArray        *ind;
  AkUIntArray        *newind;
  AkInput            *input;
  uint32_t           *inpi;
  uint32_t            chk_start;
  uint32_t            chk_end;
  uint32_t            vo;     /* vertOffset */
  uint32_t            st;
  uint32_t            count;
  uint32_t            icount;
  uint32_t            ccount; /* checked count */
} AkPrimProxy;

typedef struct AkInputDesc {
  struct AkInputDesc *next;
  const char         *semantic;
  AkURL              *source;
  AkInput            *input;
  AkPrimProxy        *pp;
  uint32_t            set;
  int32_t             index;
} AkInputDesc;

void
ak_meshFreeRsvBuff(RBTree *tree, RBNode *node);

#endif /* ak_meh_edit_common_h */
